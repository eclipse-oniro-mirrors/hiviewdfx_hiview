/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "event_export_util.h"

#include "event.h"
#include "export_db_storage.h"
#include "export_dir_creator.h"
#include "file_util.h"
#include "setting_observer_manager.h"
#include "sys_event_sequence_mgr.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "parameter.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-EventExportUtil");
constexpr int REGISTER_RETRY_CNT = 100;
constexpr int REGISTER_LOOP_DURATION = 6;

std::string GenerateDeviceId()
{
    constexpr int32_t deviceIdLength = 65;
    char id[deviceIdLength] = {0};
    if (GetDevUdid(id, deviceIdLength) == 0) {
        return std::string(id);
    }
    return "";
}

bool IsExportDirEmpty(const std::string& exportDir)
{
    std::vector<std::string> eventZipFiles;
    FileUtil::GetDirFiles(exportDir, eventZipFiles);
    return !any_of(eventZipFiles.begin(), eventZipFiles.end(), [] (const std::string& file) {
        return !FileUtil::IsDirectory(file);
    });
}

void PostExportEvent(const std::string& moduleName, int16_t taskType)
{
    auto event = std::make_shared<Event>("post_export_type_event");
    event->messageType_ = Event::MessageType::EVENT_EXPORT_TYPE;
    event->SetValue("taskType", std::to_string(taskType));
    event->SetValue("reportModule", moduleName);
    if (taskType == ALL_EVENT_TASK_TYPE) {
        event->SetValue("reportInterval", "0");
    } else {
        event->SetValue("reportInterval", std::to_string(taskType));
    }

    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        HIVIEW_LOGW("hiview context is invalid.");
        return;
    }
    context->PostUnorderedEvent(event);
}

bool IsNeedPostEvent(std::shared_ptr<ExportConfig> config)
{
    if (config == nullptr) {
        HIVIEW_LOGW("export cfg file is invalid.");
        return false;
    }
    if (!config->needPostEvent) {
        HIVIEW_LOGW("no need to post event");
        return false;
    }
    if (IsExportDirEmpty(config->exportDir)) {
        HIVIEW_LOGW("no event zip file found");
        return false;
    }
    return true;
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

int64_t GetModuleExportStartSeq(std::shared_ptr<ExportConfig> config)
{
    auto& dbMgr = ExportDbManager::GetInstance();
    int64_t startSeq = EventStore::SysEventSequenceManager::GetInstance().GetStartSequence();
    if (config == nullptr || !dbMgr.IsUnrecordedModule(config->moduleName) ||
        config->inheritedModule.empty() || dbMgr.IsUnrecordedModule(config->inheritedModule)) {
        HIVIEW_LOGI("start sequence is %{public}" PRId64, startSeq);
        return startSeq;
    }
    startSeq = dbMgr.GetExportEndSeq(config->inheritedModule);
    HIVIEW_LOGI("inherited start sequence is %{public}" PRId64 "", startSeq);
    return startSeq;
}
}

std::string EventExportUtil::GetDeviceId()
{
    static std::string deviceId = Parameter::GetUserType() == Parameter::USER_TYPE_OVERSEA_COMMERCIAL ?
        Parameter::GetString("persist.hiviewdfx.priv.packid", "") : GenerateDeviceId();
    return deviceId;
}

bool EventExportUtil::CheckAndPostExportEvent(std::shared_ptr<ExportConfig> config)
{
    if (!IsNeedPostEvent(config)) {
        return false;
    }
    PostExportEvent(config->moduleName, config->taskType);
    return true;
}

bool EventExportUtil::RegisterSettingObserver(std::shared_ptr<ExportConfig> config)
{
    SettingObserver::ObserverCallback callback =
        [config] (const std::string& paramKey) {
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

void EventExportUtil::UnregisterSettingObserver(std::shared_ptr<ExportConfig> config)
{
    SettingObserverManager::GetInstance()->UnregisterObserver(config->exportSwitchParam.name);
}

void EventExportUtil::SyncDbByExportSwitchStatus(std::shared_ptr<ExportConfig> config, bool isSwitchOff)
{
    auto& dbMgr = ExportDbManager::GetInstance();
    if (isSwitchOff) {
        HIVIEW_LOGI("export switch for module %{public}s is off", config->moduleName.c_str());
        int64_t enabledSeq = dbMgr.GetExportEnabledSeq(config->moduleName);
        if (enabledSeq != INVALID_SEQ_VAL &&
            !FileUtil::FileExists(dbMgr.GetEventInheritFlagPath(config->moduleName))) {
            dbMgr.HandleExportSwitchChanged(config->moduleName, INVALID_SEQ_VAL);
        }
        return;
    }
    HIVIEW_LOGI("export switch for module %{public}s is on", config->moduleName.c_str());
    int64_t enabledSeq = dbMgr.GetExportEnabledSeq(config->moduleName);
    if (enabledSeq == INVALID_SEQ_VAL) {
        enabledSeq = GetModuleExportStartSeq(config);
        dbMgr.HandleExportSwitchChanged(config->moduleName, enabledSeq);
    }
}

void EventExportUtil::InitEnvBeforeExport(std::shared_ptr<ExportConfig> config)
{
    // create export directory
    (void)ExportDirCreator::GetInstance().CreateExportDir(config->exportDir);
    // do inherit by switch status
    bool isSwitchOff = (SettingObserverManager::GetInstance()->GetStringValue(config->exportSwitchParam.name)
        != config->exportSwitchParam.enabledVal);
    SyncDbByExportSwitchStatus(config, isSwitchOff);
}
} // namespace HiviewDFX
} // namespace OHOS
