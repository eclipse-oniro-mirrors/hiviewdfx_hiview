/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "xperf_event_reporter.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002D66, "Hiview-XPerformance");

XperfEventReporter::XperfEventReporter()
{
}

XperfEventReporter::~XperfEventReporter()
{
}

void XperfEventReporter::Report(const char* domain, const OHOS::HiviewDFX::XperfEvent& event)
{
    if (event.evtName.empty() || (event.paramSize == 0) || (event.params == nullptr)) {
        HIVIEW_LOGE("[XperfEventReporter::Report] invalid XperfEvent data, will not report");
        return;
    }
    OH_HiSysEvent_Write(domain, event.evtName.c_str(), event.evtType, event.params, event.paramSize);
}

}
}