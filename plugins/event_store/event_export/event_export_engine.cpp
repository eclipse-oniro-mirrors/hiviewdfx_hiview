/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "event_export_engine.h"

#include <chrono>

#include "event_expire_task.h"
#include "event_export_task.h"
#include "ffrt.h"
#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "setting_observer_manager.h"
#include "sys_event_sequence_mgr.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportEngine");
namespace {
constexpr char SYS_EVENT_EXPORT_DIR_NAME[] = "sys_event_export";
constexpr int REGISTER_RETRY_CNT = 100;
constexpr int REGISTER_LOOP_DURATION = 6;
std::string GetExportDir(HiviewContext::DirectoryType type)
{
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        HIVIEW_LOGW("faield to get export directory");
        return "";
    }
    std::string configDir = context->GetHiViewDirectory(type);
    return FileUtil::IncludeTrailingPathDelimiter(configDir.append(SYS_EVENT_EXPORT_DIR_NAME));
}
}

EventExportEngine& EventExportEngine::GetInstance()
{
    static EventExportEngine instance;
    return instance;
}

EventExportEngine::EventExportEngine()
{
}

EventExportEngine::~EventExportEngine()
{
    for (auto& config : configs_) {
        SettingObserverManager::GetInstance()->UnregisterObserver(config->exportSwitchParam.name);
    }
}

void EventExportEngine::Start()
{
    std::lock_guard<std::mutex> lock(mgrMutex_);
    if (isTaskRunning_) {
        HIVIEW_LOGW("tasks have been started.");
        return;
    }
    isTaskRunning_ = true;
    auto initTaskHandle = ffrt::submit_h([this] () {
            Init();
        }, {}, {}, ffrt::task_attr().name("dft_export_init").qos(ffrt::qos_default));
    ffrt::submit([this] () {
            InitAndRunTasks();
        }, { initTaskHandle }, {}, ffrt::task_attr().name("dft_export_start").qos(ffrt::qos_default));
}

void EventExportEngine::Stop()
{
    std::lock_guard<std::mutex> lock(mgrMutex_);
    if (!isTaskRunning_) {
        HIVIEW_LOGW("tasks have been stopped");
        return;
    }
    isTaskRunning_ = false;
    HIVIEW_LOGE("succeed to stop all tasks");
}

void EventExportEngine::Init()
{
    // init ExportConfigManager
    std::string configFileStoreDir = GetExportDir(HiviewContext::DirectoryType::CONFIG_DIRECTORY);
    HIVIEW_LOGI("directory for export config file to store: %{public}s", configFileStoreDir.c_str());
    ExportConfigManager configMgr(configFileStoreDir);

    // init ExportDbManager
    std::string dbStoreDir = GetExportDir(HiviewContext::DirectoryType::WORK_DIRECTORY);
    HIVIEW_LOGI("directory for export db to store: %{public}s", dbStoreDir.c_str());
    dbMgr_ = std::make_shared<ExportDbManager>(dbStoreDir);

    // build tasks for all modules
    configMgr.GetExportConfigs(configs_);
    HIVIEW_LOGD("count of configuration: %{public}zu", configs_.size());
}

void EventExportEngine::InitAndRunTasks()
{
    HIVIEW_LOGI("total count of module is %{public}zu", configs_.size());
    for (const auto& config : configs_) {
        auto task = std::bind(&EventExportEngine::InitAndRunTask, this, config);
        ffrt::submit(task, {}, {}, ffrt::task_attr().name("dft_event_export").qos(ffrt::qos_default));
    }
}

bool EventExportEngine::RegistSettingObserver(std::shared_ptr<ExportConfig> config)
{
    SettingObserver::ObserverCallback callback =
        [this, &config] (const std::string& paramKey) {
            std::string val = SettingObserverManager::GetInstance()->GetStringValue(paramKey);
            HIVIEW_LOGI("value of param key[%{public}s] is %{public}s", paramKey.c_str(), val.c_str());
            if (val == config->exportSwitchParam.enabledVal) {
                HandleExportSwitchOn(config->moduleName);
            } else {
                HandleExportSwitchOff(config->moduleName);
            }
        };
    bool regRet = false;
    int retryCount = REGISTER_RETRY_CNT;
    while (!regRet && retryCount > 0) {
        regRet = SettingObserverManager::GetInstance()->RegisterObserver(config->exportSwitchParam.name,
            callback);
        if (regRet) {
            break;
        }
        retryCount--;
        ffrt::this_task::sleep_for(std::chrono::seconds(REGISTER_LOOP_DURATION));
    }
    if (!regRet) {
        HIVIEW_LOGW("failed to regist setting db observer for module %{public}s", config->moduleName.c_str());
        return regRet;
    }
    if (dbMgr_->IsUnrecordedModule(config->moduleName)) { // first time to export event for current module
        auto upgradeParam = config->sysUpgradeParam;
        if (!upgradeParam.name.empty() &&
            SettingObserverManager::GetInstance()->GetStringValue(upgradeParam.name) == upgradeParam.enabledVal) {
            HIVIEW_LOGI("reset enabled sequence to 0 for moudle %{public}s", config->moduleName.c_str());
            dbMgr_->HandleExportSwitchChanged(config->moduleName, 0);
        }
    }
    HIVIEW_LOGI("succeed to regist setting db observer for module %{public}s", config->moduleName.c_str());
    return regRet;
}

void EventExportEngine::InitAndRunTask(std::shared_ptr<ExportConfig> config)
{
    // reg setting db observer
    auto regRet = RegistSettingObserver(config);
    if (!regRet) {
        return;
    }
    // init tasks of current config then run them
    auto expireTask = std::make_shared<EventExpireTask>(config, dbMgr_);
    auto exportTask = std::make_shared<EventExportTask>(config, dbMgr_);
    while (isTaskRunning_) {
        expireTask->Run();
        exportTask->Run();
        // sleep for a task cycle
        ffrt::this_task::sleep_for(exportTask->GetExecutingCycle());
    }
}

void EventExportEngine::HandleExportSwitchOn(const std::string& moduleName)
{
    int64_t enabledSeq = dbMgr_->GetExportEnabledSeq(moduleName);
    if (enabledSeq == 0) {
        // expport enabled sequence has been reset to 0, no need to update
        return;
    }
    auto curEventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    HIVIEW_LOGI("update enabled seq:%{public}" PRId64 " for moudle %{public}s", curEventSeq, moduleName.c_str());
    dbMgr_->HandleExportSwitchChanged(moduleName, curEventSeq);
}

void EventExportEngine::HandleExportSwitchOff(const std::string& moduleName)
{
    dbMgr_->HandleExportSwitchChanged(moduleName, INVALID_SEQ_VAL);
}
} // HiviewDFX
} // OHOS