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

#ifndef OHOS_HIVIEWDFX_LISTENER_STATUS_MONITOR
#define OHOS_HIVIEWDFX_LISTENER_STATUS_MONITOR

#include <cstdint>
#include <string>
#include <unordered_map>

#include "event_loop.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr size_t DEFAULT_CAPACITY_OF_CALLER = 100;
constexpr uint64_t DEFAULT_TASK_INTERVAL = 10800; // 10800: 3h
}

struct ListenerCallerInfo {
    int32_t listenerUid = -1;
    std::string listenerName;
    std::string eventRule;

    ListenerCallerInfo() = default;
    ListenerCallerInfo(int32_t uid, const std::string& name, const std::string& rule)
        : listenerUid(uid), listenerName(name), eventRule(rule) {}
    ~ListenerCallerInfo() = default;

    bool operator==(const ListenerCallerInfo& other) const
    {
        return listenerUid == other.listenerUid &&
            listenerName == other.listenerName &&
            eventRule == other.eventRule;
    }

    struct Hash {
        std::size_t operator()(const ListenerCallerInfo& info) const
        {
            std::size_t h1 = std::hash<int32_t>{}(info.listenerUid);
            std::size_t h2 = std::hash<std::string>{}(info.listenerName);
            std::size_t h3 = std::hash<std::string>{}(info.eventRule);
            return h1 ^ (h2 << 1) ^ (h3 << 2); // 1 2: Using displacement operations to reduce conflicts
        }
    };
};

struct ListenerStatusInfo {
    int32_t addSuccCount = 0;
    int32_t addFaultCount = 0;
    int32_t removeSuccCount = 0;
    int32_t removeFaultCount = 0;
};

class ListenerStatusMonitorTest;

class ListenerStatusMonitor : public std::enable_shared_from_this<ListenerStatusMonitor> {
public:
    ListenerStatusMonitor(size_t capacity = DEFAULT_CAPACITY_OF_CALLER,
        uint64_t taskInterval = DEFAULT_TASK_INTERVAL);
    ~ListenerStatusMonitor();
    void SetWorkLoop(std::shared_ptr<EventLoop> looper);
    void RecordAddListener(const ListenerCallerInfo& info, bool isSucc);
    void RecordRemoveListener(const ListenerCallerInfo& info, bool isSucc);

private:
    void InitReportTask();
    bool NeedReport();
    void ReportEvent();
    void Reset();

private:
    std::mutex statusMutex_;
    std::shared_ptr<EventLoop> looper_;
    uint64_t taskSeq_ = 0;
    size_t capacityOfCaller_ = 0;
    uint64_t taskInterval_ = 0;
    std::string lastDate_;
    std::unordered_map<ListenerCallerInfo, ListenerStatusInfo, ListenerCallerInfo::Hash> listenerCallers_;

    friend class ListenerStatusMonitorTest;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_LISTENER_STATUS_MONITOR
