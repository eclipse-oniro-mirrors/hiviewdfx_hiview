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

#include "page_trace_node.h"

#include <securec.h>

namespace OHOS {
namespace HiviewDFX {
PageTraceNode::PageTraceNode(int32_t pid, uint64_t ts, const std::string& url, const std::string& name)
    : pid_(pid), timestamp_(ts)
{
    SetPageInfo(url, name);
}

void PageTraceNode::SetPageInfo(const std::string& url, const std::string& name)
{
    pagePath_ = url + (name.empty() ? "" : ":" + name);
}

std::string PageTraceNode::ToString() const
{
    return GetFormatedTimeHHMMSS(timestamp_, true) + " " + pagePath_;
}

std::string PageTraceNode::GetFormatedTimeHHMMSS(uint64_t target, bool isMillsec)
{
    constexpr uint64_t timeRatio = 1000;
    uint64_t millsec = 0;
    if (isMillsec) {
        millsec = target % timeRatio;
        target = target / timeRatio; // 1000 : convert millisecond to seconds
    }

    time_t out = static_cast<time_t>(target);
    struct tm tmStruct {0};
    struct tm* timeInfo = localtime_r(&out, &tmStruct);
    if (timeInfo == nullptr) {
        return "00:00:00";
    }

    constexpr int defaultBufferSize = 64;
    char buf[defaultBufferSize] = {0};
    if (strftime(buf, defaultBufferSize - 1, "%H:%M:%S", timeInfo) == 0) {
        return "00:00:00";
    }
    auto timeStr = std::string(buf);
    if (isMillsec) {
        int ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, ".%03lu", millsec);
        timeStr += (ret > 0) ? std::string(buf) : ".000";
    }
    return timeStr;
}
} // HiviewDFX
} // OHOS
