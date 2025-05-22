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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_PERF_COLLECT_CONFIG_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_PERF_COLLECT_CONFIG_H

#include <map>

#include "perf_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class PerfCollectConfig {
public:
    PerfCollectConfig() {};
    ~PerfCollectConfig() = default;
    static std::string GetConfigPath();
    static std::map<PerfCaller, uint8_t> GetPerfCount(const std::string& configPath);
    static int64_t GetAllowMemory(const std::string& configPath);
    static std::string MapPerfCallerToString(PerfCaller caller);
};
} // UCollectUtil
} // HiViewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_PERF_COLLECT_CONFIG_H
