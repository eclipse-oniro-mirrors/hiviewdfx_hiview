/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include "event_export_util.h"
#include "ffrt.h"
#include "ffrt_util.h"
#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "setting_observer_manager.h"
#include "sys_event_sequence_mgr.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");
namespace {
constexpr int REGISTER_RETRY_CNT = 100;
constexpr int REGISTER_LOOP_DURATION = 6;

std::string GenerateUuid()
{
    std::string uuid;
    int8_t retryTimes = 3; // max retry 3 times
    do {
        FileUtil::LoadStringFromFile("/proc/sys/kernel/random/uuid", uuid);
        if (!uuid.empty()) {
            break;
        }
        --retryTimes;
    } while (retryTimes > 0);

    if (!uuid.empty() && uuid.back() == '\n') {
        // remove line breaks at the end
        uuid.pop_back();
    }
    uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end()); // remove character '-'
    return uuid;
}

void HandleExportSwitchOn(const std::string& moduleName)
{
    auto& dbMgr = ExportDbManager::GetInstance();
    if (FileUtil::FileExists(dbMgr.GetEventInheritFlagPath(moduleName))) {
        // if inherit flag file exists, no need to update export enabled seq
        return;
    }
    auto curEventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    HIVIEW_LOGI("update enabled seq:%{public}" PRId64 " for module %{public}s", curEventSeq, moduleName.c_str());
    dbMgr.HandleExportSwitchChanged(moduleName, curEventSeq);
}

void HandleExportSwitchOff(const std::string& moduleName)
{
    auto& dbMgr = ExportDbManager::GetInstance();
    dbMgr.HandleExportSwitchChanged(moduleName, INVALID_SEQ_VAL);
    FileUtil::RemoveFile(dbMgr.GetEventInheritFlagPath(moduleName)); // remove inherit flag file if switch changes
}

bool RegisterSettingObserver(std::shared_ptr<ExportConfig> config)
{
    SettingObserver::ObserverCallback callback =
        [&config] (const std::string& paramKey) {
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
    auto& dbMgr = ExportDbManager::GetInstance();
    if (dbMgr.IsUnrecordedModule(config->moduleName)) { // first time to export event for current module
        auto upgradeParam = config->sysUpgradeParam;
        if (!upgradeParam.name.empty() &&
            SettingObserverManager::GetInstance()->GetStringValue(upgradeParam.name) == upgradeParam.enabledVal) {
            int64_t startSeq = EventStore::SysEventSequenceManager::GetInstance().GetStartSequence();
            HIVIEW_LOGI("reset enabled sequence to %{public}" PRId64 " for moudle %{public}s",
                startSeq, config->moduleName.c_str());
            dbMgr.HandleExportSwitchChanged(config->moduleName, startSeq);
            FileUtil::CreateFile(dbMgr.GetEventInheritFlagPath(config->moduleName)); // create inherit flag file
        }
    }
    HIVIEW_LOGI("succeed to regist setting db observer for module %{public}s", config->moduleName.c_str());
    return regRet;
}

void UnregisterSettingObserver(std::vector<std::shared_ptr<ExportConfig>>& configs)
{
    for (auto& config : configs) {
        SettingObserverManager::GetInstance()->UnregisterObserver(config->exportSwitchParam.name);
    }
}
}

EventExportEngine& EventExportEngine::GetInstance()
{
    static EventExportEngine instance;
    return instance;
}

EventExportEngine::~EventExportEngine()
{
    std::vector<std::shared_ptr<ExportConfig>> configs;
    ExportConfigManager::GetInstance().GetAllExportConfigs(configs);
    UnregisterSettingObserver(configs);
}

void EventExportEngine::Start()
{
    std::lock_guard<std::mutex> lock(mgrMutex_);
    if (isTaskRunning_) {
        HIVIEW_LOGW("tasks have been started.");
        return;
    }
    isTaskRunning_ = true;
    ffrt::submit([this] () {
            InitAndRunTasks();
        }, { }, {}, ffrt::task_attr().name("dft_export_start").qos(ffrt::qos_default));
}

void EventExportEngine::Stop()
{
    std::lock_guard<std::mutex> lock(mgrMutex_);
    if (!isTaskRunning_) {
        HIVIEW_LOGW("tasks have been stopped");
        return;
    }
    isTaskRunning_ = false;
}

void EventExportEngine::SetTaskDelayedSecond(int second)
{
    delayedSecond_ = second;
}

void EventExportEngine::InitAndRunTasks()
{
    std::vector<std::shared_ptr<ExportConfig>> configs;
    ExportConfigManager::GetInstance().GetAllExportConfigs(configs);
    HIVIEW_LOGI("total count of periodic config is %{public}zu", configs.size());
    for (const auto& config : configs) {
        auto task = std::bind(&EventExportEngine::InitAndRunTask, this, config);
        ffrt::submit(task, {}, {}, ffrt::task_attr().name("dft_event_export").qos(ffrt::qos_default));
    }
}

void EventExportEngine::InitAndRunTask(std::shared_ptr<ExportConfig> config)
{
    // reg setting db observer
    if (!RegisterSettingObserver(config)) {
        return;
    }
    ffrt::this_task::sleep_for(std::chrono::seconds(delayedSecond_));
    // init tasks of current config then run them
    auto expireTask = std::make_shared<EventExpireTask>(config);
    auto exportTask = std::make_shared<EventExportTask>(config);
    while (isTaskRunning_) {
        expireTask->Run();
        exportTask->Run();
        if (!EventExportUtil::CheckAndPostExportEvent(config)) {
            HIVIEW_LOGW("failed to post export event");
        }
        // sleep for a task cycle
        FfrtUtil::Sleep(config->taskCycle);
    }
}

void EventExportEngine::InitPackId()
{
    if (Parameter::GetUserType() != Parameter::USER_TYPE_OVERSEA_COMMERCIAL) {
        return;
    }
    const std::string packIdProp = "persist.hiviewdfx.priv.packid";
    if (!Parameter::GetString(packIdProp, "").empty()) {
        return;
    }
    if (std::string packId = GenerateUuid(); packId.empty()) {
        HIVIEW_LOGW("init packid failed.");
    } else {
        HIVIEW_LOGI("init packid success.");
        Parameter::SetProperty(packIdProp, packId);
    }
}
} // HiviewDFX
} // OHOS
