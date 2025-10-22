/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_HIVIEW_DFX_AVCODEC_EVENT_H
#define OHOS_HIVIEW_DFX_AVCODEC_EVENT_H

#include "xperf_service_log.h"
#include "xperf_event.h"

namespace OHOS {
namespace HiviewDFX {

//4001 "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#FAULT_ID:0#FAULT_CODE:0";
struct AvcodecJankEvent : public OhosXperfEvent {
    int16_t faultId{0};
    int16_t faultCode{0};
    int32_t pid{0};
    int64_t uniqueId{0};
    std::string jankReason;
    std::string bundleName;
    std::string surfaceName;
};

//4000 "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#SURFACE_NAME:399542385184Surface#FPS:60
// #REPORT_INTERVAL:100";
struct AvcodecFirstFrame : public OhosXperfEvent {
    int32_t pid{0};
    int32_t fps{0};
    int32_t reportInterval{0};
    int64_t uniqueId{0};
    std::string surfaceName;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif