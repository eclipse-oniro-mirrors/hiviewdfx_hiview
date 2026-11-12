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

#ifndef DFR_FD_LEAK_CONFIG_H
#define DFR_FD_LEAK_CONFIG_H

#include <map>
#include <string>

#include <singleton.h>
#include <cJSON.h>
#include <json/json.h>

namespace OHOS {
namespace HiviewDFX {
class FdLeakConfig : public Singleton<FdLeakConfig> {
    DECLARE_SINGLETON(FdLeakConfig);
public:
    void UpdateValue(const cJSON* node);
    void Print();
    inline bool GetEnable()
    {
        return enable_;
    }
    inline bool GetKillEnable()
    {
        return killEnable_;
    }
    inline int32_t GetThreshold()
    {
        return threshold_;
    }
    inline int64_t GetUploadInterval()
    {
        return uploadInterval_;
    }
    inline int32_t GetStackDebugTime()
    {
        return stackDebugTime_;
    }
    inline std::string GetDumpFunction()
    {
        return dumpFunction_;
    }
    inline std::string GetWhiteList()
    {
        return whiteList_;
    }
    inline int32_t GetKillConfigBackgroundKillThreshold()
    {
        return killConfigBackgroundKillThreshold_;
    }
    inline int32_t GetKillConfigForegroundKillThreshold()
    {
        return killConfigForegroundKillThreshold_;
    }
    inline std::string GetKillConfigProcessListForKill()
    {
        return killConfigProcessListForKill_;
    }
    inline int32_t GetProfilerTime()
    {
        return profilerTime_;
    }
    inline bool GetAsyncStackEnable()
    {
        return asyncStackEnable_;
    }
    inline int32_t GetOrphanVnodeThreshold()
    {
        return orphanVnodeThreshold_;
    }
private:
    bool enable_ { false };
    bool killEnable_ { false };
    int32_t threshold_ { 5000 };
    int64_t uploadInterval_ { 36000 };
    int32_t stackDebugTime_ { 600 };
    std::string dumpFunction_;
    std::string whiteList_;
    int32_t killConfigBackgroundKillThreshold_ { 20000 };
    int32_t killConfigForegroundKillThreshold_ { 30000 };
    std::string killConfigProcessListForKill_;
    int32_t profilerTime_ { 0 };
    bool asyncStackEnable_ { false };
    int32_t orphanVnodeThreshold_ { 100 };
private:
    void UpdateKillConfig(const cJSON *node);
    void UpdateCommonKeyMap(const cJSON *node);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FD_LEAK_CONFIG_H
