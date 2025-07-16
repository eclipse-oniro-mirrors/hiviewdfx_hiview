/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_HIVIEWDFX_UCOLLECTUTIL_COMMON_UTIL_H
#define OHOS_HIVIEWDFX_UCOLLECTUTIL_COMMON_UTIL_H

#include <cinttypes>
#include <sstream>
#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
constexpr const char* const PROC = "/proc/";
constexpr const char* const IO = "/io";
constexpr const char* const SMAPS_ROLLUP = "/smaps_rollup";
constexpr const char* const STATM = "/statm";
constexpr const char* const MEM_INFO = "/proc/meminfo";
constexpr const char* const GPU_CUR_FREQ = "/sys/class/devfreq/gpufreq/cur_freq";
constexpr const char* const GPU_MAX_FREQ = "/sys/class/devfreq/gpufreq/max_freq";
constexpr const char* const GPU_MIN_FREQ = "/sys/class/devfreq/gpufreq/min_freq";
constexpr const char* const GPU_LOAD = "/sys/class/devfreq/gpufreq/gpu_scene_aware/utilisation";
constexpr const char* const MEMINFO_SAVE_DIR = "/data/log/hiview/unified_collection/memory";
const static int VSS_BIT = 4;

class CommonUtil {
private:
    CommonUtil() = default;
    ~CommonUtil() = default;

public:
    static bool ParseTypeAndValue(const std::string &str, std::string &type, int64_t &value);
    static void GetDirRegexFiles(const std::string& path, const std::string& pattern, std::vector<std::string>& files);
    static int GetFileNameNum(const std::string& fileName, const std::string& ext);
    static std::string CreateExportFile(const std::string& path, int32_t maxFileNum, const std::string& prefix,
        const std::string& ext);
    static int32_t ReadNodeWithOnlyNumber(const std::string& fileName);
    template <typename T> static bool StrToNum(const std::string& sString, T &tX)
    {
        std::istringstream iStream(sString);
        return (iStream >> tX) ? true : false;
    }
}; // CommonUtil
} // UCollectUtil
} // HiviewDFX
} // OHOS
#endif // OHOS_HIVIEWDFX_UCOLLECTUTIL_COMMON_UTIL_H
