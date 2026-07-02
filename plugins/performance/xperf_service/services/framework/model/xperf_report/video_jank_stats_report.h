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

#ifndef VIDEO_JANK_STATS_REPORT_H
#define VIDEO_JANK_STATS_REPORT_H

#include <string>

namespace OHOS {
namespace HiviewDFX {

struct VideoJankStatsReport {
    int32_t pid{0};
    int64_t uniqueId{0};
    std::string bundleName;
    std::string surfaceName;
    int64_t lastUpTime{0};
    int64_t latency{0}; // 起播时延

    int64_t rsDur{0}; // rs侧时长
    int32_t rsJankTimes{0}; // rs侧卡顿次数
    int64_t rsJankDur{0}; // rs侧卡顿时长
    int64_t codecDur{0}; // codec侧时长
    int32_t codecJankTimes{0}; // codec侧卡顿次数
    int64_t codecJankDur{0}; // codec侧卡顿时长
};
} // namespace HiviewDFX
} // namespace OHOS

#endif