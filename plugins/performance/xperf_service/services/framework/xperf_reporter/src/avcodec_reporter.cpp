/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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
#include "avcodec_reporter.h"
#include "event_bridge.h"
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

void AvcodecReporter::ReportAvcodecInit(int64_t time)
{
    std::string data;
    data.append("HAPPEN_TIME:").append(std::to_string(time));
    EventBridge::GetInstance().OnEvent("AVCODEC_INIT_INNER", data);
}

void AvcodecReporter::ReportAvcodecRelease(int64_t time)
{
    std::string data;
    data.append("HAPPEN_TIME:").append(std::to_string(time));
    EventBridge::GetInstance().OnEvent("AVCODEC_RELEASE_INNER", data);
}

void AvcodecReporter::ReportAvcodecJank(const std::string& data)
{
    EventBridge::GetInstance().OnEvent("AVCODEC_JANK_FAULT_INNER", data);
}

} // namespace HiviewDFX
} // namespace OHOS