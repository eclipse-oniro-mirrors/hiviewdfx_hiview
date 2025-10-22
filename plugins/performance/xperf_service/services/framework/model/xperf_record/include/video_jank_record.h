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

#ifndef OHOS_HIVIEW_DFX_VIDEO_JANK_RECORD_H
#define OHOS_HIVIEW_DFX_VIDEO_JANK_RECORD_H

#include "xperf_service_log.h"
#include "network_event.h"
#include "avcodec_event.h"
#include "rs_event.h"

namespace OHOS {
namespace HiviewDFX {

struct VideoJankRecord {
    RsJankEvent rsJankEvent;
    NetworkJankEvent nwJankEvent;
    AvcodecJankEvent avcodecJankEvent;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif