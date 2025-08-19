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

#ifndef VIDEO_PLAY_RECORD_H
#define VIDEO_PLAY_RECORD_H

#include "xperf_service_log.h"
#include <string>

namespace OHOS {
namespace HiviewDFX {

class VideoPlayRecord {
public:
    int32_t pid{0};
    int64_t audioUniqueId{0};
    std::string audioBundleName;
    int64_t audioHappenTime{0};
    int16_t audioStatus{-1};
    int64_t codecUniqueId{0};
    std::string codecBundleName;
    std::string surfaceName;
    int16_t fps{0};
    int32_t reportInterval{0};

    std::string toString()
    {
        std::string str = "pid:" + std::to_string(pid)
                +" audioUniqueId:"+std::to_string(audioUniqueId)
                +" audioBundleName:"+audioBundleName
                +" audioHappenTime:"+std::to_string(audioHappenTime)
                +" audioStatus:"+std::to_string(audioStatus)
                +" codecUniqueId:"+std::to_string(codecUniqueId)
                +" codecBundleName:"+codecBundleName
                +" surfaceName:"+surfaceName
                +" fps:"+std::to_string(fps)
                +" reportInterval:"+std::to_string(reportInterval);
        return str;
    }
};
} // namespace HiviewDFX
} // namespace OHOS

#endif