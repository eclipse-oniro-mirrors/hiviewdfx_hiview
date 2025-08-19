
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

#include "network_event_parser.h"
#include "network_event.h"
#include "network_event_parser.h"
#include "xperf_parser.h"
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

// "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#FAULT_ID:0#FAULT_CODE:0";
OhosXperfEvent* ParseNetworkFaultMsg(const std::string& msg)
{
    NetworkJankEvent* event = new (std::nothrow) NetworkJankEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_PID, 0);
    ExtractStrToInt(msg, event->appPid, TAG_PID, TAG_BUNDLE_NAME, 0);
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_FAULT_ID, "NA");
    ExtractStrToInt16(msg, event->faultId, TAG_FAULT_ID, TAG_FAULT_CODE, 0);
    ExtractStrToInt16(msg, event->faultCode, TAG_FAULT_CODE, "", -1);
    return event;
}

} // namespace HiviewDFX
} // namespace OHOS
