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
#include "export_dir_creator.h"
#include "ffrt.h"
#include "ffrt_util.h"
#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "sys_event_sequence_mgr.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");
namespace {
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
}

EventExportEngine& EventExportEngine::GetInstance()
{
    static EventExportEngine instance;
    return instance;
}

EventExportEngine::~EventExportEngine()
{
    std::vector<std::shared_ptr<ExportConfig>> configs;
    ExportConfigManager::GetInstance().GetPeriodicExportConfigs(configs);
    for (auto& config : configs) {
        EventExportUtil::UnregisterSettingObserver(config);
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
    ExportConfigManager::GetInstance().GetPeriodicExportConfigs(configs);
    HIVIEW_LOGI("total count of periodic config is %{public}zu", configs.size());
    for (const auto& config : configs) {
        if (!ExportDirCreator::GetInstance().CreateExportDir(config->exportDir)) {
            continue;
        }
    }
    for (const auto& config : configs) {
        auto task = std::bind(&EventExportEngine::InitAndRunTask, this, config);
        ffrt::submit(task, {}, {}, ffrt::task_attr().name("dft_event_export").qos(ffrt::qos_default));
    }
}

void EventExportEngine::InitAndRunTask(std::shared_ptr<ExportConfig> config)
{
    // reg setting db observer
    if (!EventExportUtil::RegisterSettingObserver(config)) {
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
