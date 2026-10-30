/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#ifndef DFR_NATIVE_LEAK_CONFIG_H
#define DFR_NATIVE_LEAK_CONFIG_H

#include <map>
#include <unordered_map>
#include <string>
#include <singleton.h>
#include <cJSON.h>
#include <resource/memory.h>

#include "leak_json_parser.h"

namespace OHOS {
namespace HiviewDFX {

// service leak
constexpr int LOWMEM_THRESHOLD_MB = 800; // 800 MB
constexpr int DEFAULT_SERVICE_LEAK_OVER_RAM_RATIO = 40; // over 40% of total RAM
inline constexpr const char* const DEFAULT_SERVICE_LEAK_BLOCKLIST = "";

struct ProfilerParam {
    int profilerTime { 900 };
    int secondNmdDelay { 300 };
    int profilerCatNmdInterval { 300 };
    int profilerIntervalService { 256 };
    int profilerIntervalApp { 512 };
    bool profilerAsyncStackEnable { false };
    int nmdListMinSize { 2 };
    int nmdListMaxSize { 50 };
    int minSampleSize { 384 };
    int gwpAsanEnabled { 0 };
    // Memory blocks not released after exceeding holdTimeSecond
    int gwpHoldTimeSecond { 300 };
    // A query returns at most maxEntries memory blocks
    int gwpMaxEntries { 100 };
    int gwpMaxStackDepth = { 10 };
};

struct ServiceLeakControlParam {
    bool enable {false};
    // force kill flag, to replace nativeleak's kill enable flag
    bool enableForceKill {false};
    int lowMemThreshold {LOWMEM_THRESHOLD_MB};
    int overRamRatio {DEFAULT_SERVICE_LEAK_OVER_RAM_RATIO};
    // processes in the blocklist, only dump basic log, not kill
    std::string blockList {DEFAULT_SERVICE_LEAK_BLOCKLIST};
};

struct DumpFunctionMap {
    // key: log_grade, value: dump_function
    std::map<std::string, std::string> defaultDumpFunctionMap;
    // key: process_name, value: dump_function
    std::map<std::string, std::string> specialDumpFunctionMap;
};

struct DumpConfig {
    // field_name: xperf_threshold_info_mb
    int thresholdInfo;
    // field_name: xperf_threshold_error_mb
    int thresholdError;
};

struct NativeParam {
    bool enable { false };
    bool killEnable { false };
    DumpFunctionMap dumpFunctionMap;
    ProfilerParam profilerParam;
    ServiceLeakControlParam serviceLeakControlParam;
    std::vector<std::string> ignoreList;
    std::vector<std::string> lowSampleServiceList;
    std::unordered_map<std::string, ConfigValueStru> commonKeyMap;
    std::unordered_map<std::string, DumpConfig> overThresholdDumpMap; // phone only
};

class NativeLeakConfig : public Singleton<NativeLeakConfig> {
    DECLARE_SINGLETON(NativeLeakConfig);
public:
    void UpdatePolicy(const cJSON *node);
    void UpdateThirdParty(const cJSON *node);
    void Print();

    const NativeParam &GetNativeConfig();
    void ParseNativeConfig(const cJSON* node);
    bool ParseStrToProcessMemInfo(const std::string &eventStr,
                                  std::vector<ProcessMemory> &processMemoryList,
                                  std::string &faultEventId);
    ConfigValueStru GetCommonKeyValue(const std::string &key);
    inline int GetProfilerCatNmdInterval()
    {
        return param_.profilerParam.profilerCatNmdInterval;
    }
    inline const ServiceLeakControlParam &GetServiceLeakControlParam()
    {
        return param_.serviceLeakControlParam;
    }
    inline int GetProfilerTime()
    {
        return param_.profilerParam.profilerTime;
    }
    inline int GetSecondNmdDelay()
    {
        return param_.profilerParam.secondNmdDelay;
    }
    inline bool GetAsyncStackFlag()
    {
        return param_.profilerParam.profilerAsyncStackEnable;
    }
    inline int GetMinSampleSize()
    {
        return param_.profilerParam.minSampleSize;
    }
    inline int GetNmdListMaxSize()
    {
        return param_.profilerParam.nmdListMaxSize;
    }
    inline int GetNmdListMinSize()
    {
        return param_.profilerParam.nmdListMinSize;
    }
    inline bool GetKillEnable()
    {
        return param_.killEnable;
    }
    inline int GetGwpAsanEnabled()
    {
        return param_.profilerParam.gwpAsanEnabled;
    }
    inline int GetGwpHoldTimeSecond()
    {
        return param_.profilerParam.gwpHoldTimeSecond;
    }
    inline int GetGwpMaxEntries()
    {
        return param_.profilerParam.gwpMaxEntries;
    }
    inline int GetGwpMaxStackDepth()
    {
        return param_.profilerParam.gwpMaxStackDepth;
    }

private:
    void ForEachParseDumpFunction(const cJSON* node, std::map<std::string, std::string> &map);
    void DoParseDumpFunctionMap(const cJSON* node);
    void DoParseProfilerParam(const cJSON* node);
    void DoParseServiceLeakControlParam(const cJSON* node);
    void DoParseConfig(const cJSON* node);
    void DoParseOverThresholdMap(const cJSON *node, std::unordered_map<std::string, DumpConfig> &configTable);

private:
    NativeParam param_;
    LeakJsonParser parser_;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif  // DFR_NATIVE_LEAK_CONFIG_H
