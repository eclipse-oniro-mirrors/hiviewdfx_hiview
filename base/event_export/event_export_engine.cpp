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
#include "sys_event_service_adapter.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportEngine");
namespace {
constexpr char SYS_EVENT_EXPORT_DIR_NAME[] = "sys_event_export";
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
    Init();
}

EventExportEngine::~EventExportEngine()
{
    UnregistSettingsObservers();
}

void EventExportEngine::Start()
{
    std::lock_guard<std::mutex> lock(mgrMutex_);
    if (isTaskRunning_) {
        HIVIEW_LOGW("tasks have been started.");
        return;
    }
    isTaskRunning_ = true;
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
    // register setting db param observer
    auto task = std::bind(&EventExportEngine::RegistSettingsObservers, this);
    ffrt::submit(task, {}, {}, ffrt::task_attr().name("dft_hiview_reg").qos(ffrt::qos_default));

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
    for (auto& config : configs_) {
        auto expireTask = std::make_shared<EventExpireTask>(config, dbMgr_);
        tasks_.emplace_back(expireTask);
        auto exportTask = std::make_shared<EventExportTask>(config, dbMgr_);
        tasks_.emplace_back(exportTask);
    }
}

void EventExportEngine::RegistSettingsObservers()
{
    for (auto& config : configs_) {
        dbMgr_->HandleExportModuleInit(config->moduleName, SysEventServiceAdapter::GetCurrentEventSeq());
        SettingObserver::ObserverCallback callback =
            [this, &config] (const std::string& paramKey) {
                auto setVal = SettingObserverManager::GetInstance()->GetStringValue(paramKey);
                HIVIEW_LOGI("value of param key[%{public}s] is set to be %{public}s", paramKey.c_str(), setVal.c_str());
                if (setVal == config->settingDbParam.enabledVal) {
                    this->HandleExportSwitchOn(config->moduleName);
                } else if (setVal == config->settingDbParam.disabledVal) {
                    this->HandleExportSwitchOff(config->moduleName);
                }
            };
        SettingObserverManager::GetInstance()->RegisterObserver(config->settingDbParam.paramName, callback);
    }
}

void EventExportEngine::UnregistSettingsObservers()
{
    for (auto& config : configs_) {
        SettingObserverManager::GetInstance()->UnregisterObserver(config->settingDbParam.paramName);
    }
}

void EventExportEngine::HandleExportSwitchOn(const std::string& moduleName)
{
    dbMgr_->HandleExportSwitchChanged(moduleName, SysEventServiceAdapter::GetCurrentEventSeq());
}

void EventExportEngine::HandleExportSwitchOff(const std::string& moduleName)
{
    dbMgr_->HandleExportSwitchChanged(moduleName, INVALID_SEQ_VAL);
}
} // HiviewDFX
} // OHOS