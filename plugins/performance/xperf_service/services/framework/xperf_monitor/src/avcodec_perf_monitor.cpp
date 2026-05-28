/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "avcodec_perf_monitor.h"
#include "xperf_constant.h"
#include "xperf_service_log.h"
#include "perf_trace.h"
#include "play_latency_reporter.h"

namespace OHOS {
namespace HiviewDFX {

AvcodecPerfMonitor& AvcodecPerfMonitor::GetInstance()
{
    static AvcodecPerfMonitor instance;
    return instance;
}

void AvcodecPerfMonitor::ProcessEvent(OhosXperfEvent* event)
{
    LOGD("AvcodecPerfMonitor_ProcessEvent logId:%{public}d msg:%{public}s", event->logId, event->rawMsg.c_str());
    switch (event->logId) {
        case XperfConstants::AVCODEC_INIT: //解码器创建
            {
                LOGI("AVCODEC_INIT msg:%{public}s", event->rawMsg.c_str());
                XPERF_TRACE_SCOPED("AVCODEC_INIT msg:%s", event->rawMsg.c_str());
            }
            break;
        case XperfConstants::AVCODEC_RELEASE: //解码器销毁
            {
                LOGI("AVCODEC_RELEASE msg:%{public}s", event->rawMsg.c_str());
                XPERF_TRACE_SCOPED("AVCODEC_RELEASE msg:%s", event->rawMsg.c_str());
            }
            break;
        case XperfConstants::AVCODEC_JANK_FAULT: //帧间隔大于300ms
            OnFault(event);
            break;
        case XperfConstants::AVCODEC_FRAME_STATS: //解码统计
            {
                LOGI("AVCODEC_FRAME_STATS msg:%{public}s", event->rawMsg.c_str());
                XPERF_TRACE_SCOPED("AVCODEC_FRAME_STATS msg:%s", event->rawMsg.c_str());
            }
            break;
        default:
            break;
    }
}

void AvcodecPerfMonitor::OnFault(OhosXperfEvent* event)
{
    LOGI("AVCODEC_JANK_FAULT msg:%{public}s", event->rawMsg.c_str());
    XPERF_TRACE_SCOPED("AVCODEC_JANK_FAULT msg:%s", event->rawMsg.c_str());
    PlayLatencyReporter::ReportFault("AVCODEC_JANK_FAULT", event->rawMsg);
}
} // namespace HiviewDFX
} // namespace OHOS
