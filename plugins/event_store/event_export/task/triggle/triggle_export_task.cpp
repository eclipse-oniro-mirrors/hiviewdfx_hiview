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
#include "triggle_export_task.h"

#include "event_export_util.h"
#include "hiview_logger.h"
#include "sys_event_sequence_mgr.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-TriggleExportFlow");

TriggleExportTask::TriggleExportTask(std::shared_ptr<ExportConfig> config, int taskId)
    : EventExportTask(config)
{
    id_ = taskId;
    endSeq_ = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
}

void TriggleExportTask::AppendEvent(std::shared_ptr<SysEvent> sysEvent)
{
    if (sysEvent == nullptr) {
        HIVIEW_LOGE("invalid event");
        return;
    }
    auto event = std::make_shared<TriggleExportEvent>();
    event->domain = sysEvent->domain_;
    event->eventName = sysEvent->eventName_;
    event->timeStamp = sysEvent->happenTime_;
    event->seq = sysEvent->GetEventSeq();
    {
        std::unique_lock<ffrt::mutex> lock(listMutex_);
        allEvent_.emplace_back(event);
    }
}

std::string TriggleExportTask::GetModuleName()
{
    if (config_ == nullptr) {
        HIVIEW_LOGE("export config is invalid");
        return "";
    }
    return config_->moduleName;
}

int64_t TriggleExportTask::GetTimeStamp()
{
    std::unique_lock<ffrt::mutex> lock(listMutex_);
    if (allEvent_.empty()) {
        return 0;
    }
    auto frontEvent = allEvent_.front();
    if (frontEvent == nullptr) {
        HIVIEW_LOGE("front event is an invalid event");
        return 0;
    }
    return frontEvent->timeStamp;
}

int TriggleExportTask::GetId()
{
    return id_;
}

std::chrono::seconds TriggleExportTask::GetTriggleCycle()
{
    if (config_ == nullptr) {
        HIVIEW_LOGE("export config is invalid");
        return std::chrono::seconds(0);
    }
    return std::chrono::seconds(config_->taskTriggleCycle);
}

void TriggleExportTask::OnTaskRun()
{
    EventExportTask::OnTaskRun();
    EventExportUtil::CheckAndPostExportEvent(config_);
}

bool TriggleExportTask::ParseExportEventList(ExportEventList& list)
{
    std::unique_lock<ffrt::mutex> lock(listMutex_);
    if (allEvent_.empty()) {
        return EventExportTask::ParseExportEventList(list);
    }
    for (auto& event : allEvent_) {
        auto iter = list.find(event->domain);
        if (iter == list.end()) {
            list.insert({event->domain, std::vector<std::string> { event->eventName }});
            continue;
        }
        auto& eventNames = iter->second;
        if (std::find(eventNames.begin(), eventNames.end(), event->eventName) != eventNames.end()) {
            continue;
        }
        eventNames.emplace_back(event->eventName);
    }
    return true;
}

int64_t TriggleExportTask::GetExportRangeEndSeq()
{
    std::unique_lock<ffrt::mutex> lock(listMutex_);
    if (allEvent_.empty()) {
        HIVIEW_LOGI("export event list is empty");
        return endSeq_;
    }
    auto lastEvent = allEvent_.back();
    if (lastEvent == nullptr) {
        HIVIEW_LOGE("last event is an invalid event");
        return 0;
    }
    auto endSeq = lastEvent->seq;
    endSeq++; // end of export range is open interval --> [beginSeq, endSeq)
    return endSeq;
}
} // namespace HiviewDFX
} // namespace OHOS