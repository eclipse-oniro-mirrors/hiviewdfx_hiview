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

#ifndef VIDEO_JANK_REPORT_H
#define VIDEO_JANK_REPORT_H

#include "xperf_service_log.h"
#include <string>

namespace OHOS {
namespace HiviewDFX {

class VideoJankReport {
public:
    int32_t appPid{0};
    std::string versionCode;
    std::string versionName;
    std::string bundleName;
    std::string abilityName;
    std::string pageUrl;
    std::string pageName;
    std::string surfaceName;
    int64_t maxFrameTime{0};
    int64_t happenTime{0};
    int16_t faultId{0};
    int16_t faultCode{0};
};
} // namespace HiviewDFX
} // namespace OHOS

#endif