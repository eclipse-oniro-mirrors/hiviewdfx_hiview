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

#include "listener_status_monitor.h"

#include "hisysevent.h"
#include "hiview_logger.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
using OHOS::HiviewDFX::HiSysEvent;
DEFINE_LOG_TAG("ListenerMonitor");

ListenerStatusMonitor::ListenerStatusMonitor(size_t capacity, uint64_t taskInterval)
    : capacityOfCaller_(capacity), taskInterval_(taskInterval)
{
    lastDate_ = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y%m%d");
}

ListenerStatusMonitor::~ListenerStatusMonitor()
{
    if (looper_ != nullptr) {
        bool ret = looper_->RemoveEvent(taskSeq_);
        HIVIEW_LOGI("remove task result=%{public}d", ret);
    }
}

void ListenerStatusMonitor::SetWorkLoop(std::shared_ptr<EventLoop> looper)
{
    if (looper == nullptr) {
        HIVIEW_LOGW("looper is null");
        return;
    }

    std::lock_guard<std::mutex> lock(statusMutex_);
    looper_ = looper;
    InitReportTask();
}

void ListenerStatusMonitor::InitReportTask()
{
    taskSeq_ = looper_->AddTimerEvent(nullptr, nullptr, [monitor = shared_from_this()] {
        std::lock_guard<std::mutex> lock(monitor->statusMutex_);
        if (monitor->NeedReport()) {
            monitor->ReportEvent();
            monitor->Reset();
        }
    }, taskInterval_, true);
    HIVIEW_LOGI("init report task, cycle=%{public}" PRIu64, taskInterval_);
}

bool ListenerStatusMonitor::NeedReport()
{
    std::string currDate = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y%m%d");
    if (lastDate_ != currDate) {
        HIVIEW_LOGI("need report event, lastDate=%{public}s, currDate=%{public}s", lastDate_.c_str(), currDate.c_str());
        lastDate_ = currDate;
        return true;
    }
    HIVIEW_LOGD("do not need report event, currDate=%{public}s", currDate.c_str());
    return false;
}

void ListenerStatusMonitor::ReportEvent()
{
    HIVIEW_LOGI("report event size=%{public}zu", listenerCallers_.size());
    for (const auto& [caller, status] : listenerCallers_) {
        auto ret = HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, "SYSEVENT_LISTENER_ANALYSIS", HiSysEvent::STATISTIC,
            "USER_ID", caller.listenerUid,
            "LISTENER_NAME", caller.listenerName,
            "EVENT_RULE", caller.eventRule,
            "LISTENER_MATCH_COUNT", status.addSuccCount - status.removeSuccCount,
            "ADDLISTENER_SUCC_COUNT", status.addSuccCount,
            "ADDLISTENER_FAULT_COUNT", status.addFaultCount,
            "REMOVELISTENER_FAULT_COUNT", status.removeFaultCount);
        if (ret != 0) {
            HIVIEW_LOGW("failed to report event, uid=%{public}d, name=%{public}s, ret=%{public}d",
                caller.listenerUid, caller.listenerName.c_str(), ret);
        }
    }
}

void ListenerStatusMonitor::Reset()
{
    std::unordered_map<ListenerCallerInfo, ListenerStatusInfo, ListenerCallerInfo::Hash>().swap(listenerCallers_);
}

void ListenerStatusMonitor::RecordAddListener(const ListenerCallerInfo& info, bool isSucc)
{
    HIVIEW_LOGD("record listener uid=%{public}d, name=%{public}s, ret=%{public}d",
        info.listenerUid, info.listenerName.c_str(), isSucc);
    int32_t addSuccCount = isSucc ? 1 : 0;
    int32_t addFaultCount = isSucc ? 0 : 1;
    std::lock_guard<std::mutex> lock(statusMutex_);
    if (NeedReport()) {
        ReportEvent();
        Reset();
    }

    if (listenerCallers_.find(info) != listenerCallers_.end()) {
        listenerCallers_[info].addSuccCount += addSuccCount;
        listenerCallers_[info].addFaultCount += addFaultCount;
        return;
    }

    if (listenerCallers_.size() >= capacityOfCaller_) {
        HIVIEW_LOGW("failed to add caller due to container full, size=%{public}zu", listenerCallers_.size());
        return;
    }
    listenerCallers_[info] = ListenerStatusInfo{addSuccCount, addFaultCount, 0, 0};
}

void ListenerStatusMonitor::RecordRemoveListener(const ListenerCallerInfo& info, bool isSucc)
{
    HIVIEW_LOGD("record listener uid=%{public}d, name=%{public}s, ret=%{public}d",
        info.listenerUid, info.listenerName.c_str(), isSucc);
    int32_t removeSuccCount = isSucc ? 1 : 0;
    int32_t removeFaultCount = isSucc ? 0 : 1;
    std::lock_guard<std::mutex> lock(statusMutex_);
    if (NeedReport()) {
        ReportEvent();
        Reset();
    }

    if (listenerCallers_.find(info) != listenerCallers_.end()) {
        listenerCallers_[info].removeSuccCount += removeSuccCount;
        listenerCallers_[info].removeFaultCount += removeFaultCount;
        return;
    }

    if (listenerCallers_.size() >= capacityOfCaller_) {
        HIVIEW_LOGW("failed to add caller due to container full, size=%{public}zu", listenerCallers_.size());
        return;
    }
    listenerCallers_[info] = ListenerStatusInfo{0, 0, removeSuccCount, removeFaultCount};
}
}  // namespace HiviewDFX
}  // namespace OHOS
