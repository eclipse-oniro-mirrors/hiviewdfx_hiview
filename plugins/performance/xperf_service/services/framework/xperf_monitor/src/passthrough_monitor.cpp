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
 
#include "passthrough_monitor.h"
 
#include <sstream>
 
#include "load_complete_reporter.h"
#include "perf_load_complete_event.h"
#include "xperf_service_log.h"
 
namespace OHOS {
namespace HiviewDFX {
 
const std::string EVENT_LOAD_COMPLETE = "LOAD_COMPLETE";
 
PassthroughMonitor& PassthroughMonitor::GetInstance()
{
    static PassthroughMonitor instance;
    return instance;
}
 
void PassthroughMonitor::ProcessEvent(OhosXperfEvent* event)
{
    LOGD("PassthroughMonitor msg:%{public}s", event->rawMsg.c_str());
    if (event->eventName == EVENT_LOAD_COMPLETE) {
        ProcessLoadCompleteEvent(event);
    }
}
 
void PassthroughMonitor::ProcessLoadCompleteEvent(OhosXperfEvent* event)
{
    PerfLoadCompleteEvent* loadCompleteEvent = (PerfLoadCompleteEvent*) event;
 
    std::stringstream pageLoadCost;
    pageLoadCost << "isLaunch:" << loadCompleteEvent->isLaunch << ",lastComponent:" <<
        loadCompleteEvent->lastComponent << ";";
    std::vector<std::string> array;
    array.push_back(pageLoadCost.str());
    LoadCompleteReport reportInfo = {
        .errorType = 6,
        .pageLoadCost = array,
        .abilityName = "",
        .packageName = loadCompleteEvent->bundleName,
    };
    LoadCompleteReporter::ReportLoadComplete(reportInfo);
}
 
} // namespace HiviewDFX
} // namespace OHOS