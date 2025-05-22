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

#include "perf_collect_config.h"

#include "cjson_util.h"
#include "config_policy_utils.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("PerfCollectConfig");
}

std::string PerfCollectConfig::MapPerfCallerToString(PerfCaller caller)
{
    switch (caller) {
        case PerfCaller::EVENTLOGGER:
            return "EVENTLOGGER";
        case PerfCaller::XPOWER:
            return "XPOWER";
        case PerfCaller::UNIFIED_COLLECTOR:
            return "UNIFIED_COLLECTOR";
        case PerfCaller::PERFORMANCE_FACTORY:
            return "PERFORMANCE_FACTORY";
        default:
            return "";
    }
}

std::string PerfCollectConfig::GetConfigPath()
{
    const std::string collectorConfigPath = "etc/hiview/unified_collection/collector.json";
    char buf[MAX_PATH_LEN];
    char* path = GetOneCfgFile(collectorConfigPath.c_str(), buf, MAX_PATH_LEN);
    if (path == nullptr || *path == '\0') {
        return "";
    }
    return path;
}

std::map<PerfCaller, uint8_t> PerfCollectConfig::GetPerfCount(const std::string& configPath)
{
    // default config for perf collect concurrency
    std::map<PerfCaller, uint8_t> perfMaxCountForCaller = {
        // key : caller, value : max hiperf processes count can be started at same time
        {PerfCaller::EVENTLOGGER, 2},
        {PerfCaller::XPOWER, 2},
        {PerfCaller::UNIFIED_COLLECTOR, 2},
        {PerfCaller::PERFORMANCE_FACTORY, 8},
    };
    if (configPath.empty()) {
        return perfMaxCountForCaller;
    }
    auto root = CJsonUtil::ParseJsonRoot(configPath);
    if (root == nullptr) {
        HIVIEW_LOGW("parse config failed");
        return perfMaxCountForCaller;
    }
    auto perfConfig = CJsonUtil::GetObjectValue(root, "Perf");
    if (perfConfig == nullptr) {
        HIVIEW_LOGW("parse perf config failed");
        cJSON_Delete(root);
        return perfMaxCountForCaller;
    }
    auto concurrencyConfig = CJsonUtil::GetObjectValue(perfConfig, "ConcurrencyStrategy");
    if (concurrencyConfig == nullptr) {
        HIVIEW_LOGW("parse config failed of ConcurrencyStrategy");
        cJSON_Delete(root);
        return perfMaxCountForCaller;
    }
    for (auto& item : perfMaxCountForCaller) {
        std::string callerStr = MapPerfCallerToString(item.first);
        int64_t quota = CJsonUtil::GetIntValue(concurrencyConfig, callerStr, -1);
        if (quota >= 0) {
            item.second =static_cast<uint8_t>(quota);
        }
    }
    cJSON_Delete(root);
    return perfMaxCountForCaller;
}

int64_t PerfCollectConfig::GetAllowMemory(const std::string& configPath)
{
    const int64_t allowMemory = -1; // perf collect allowed memory, -1 means not limit
    if (configPath.empty()) {
        return allowMemory;
    }
    auto root = CJsonUtil::ParseJsonRoot(configPath);
    if (root == nullptr) {
        HIVIEW_LOGW("parse config failed");
        return allowMemory;
    }
    auto perfConfig = CJsonUtil::GetObjectValue(root, "Perf");
    if (perfConfig == nullptr) {
        HIVIEW_LOGW("parse perf config failed");
        cJSON_Delete(root);
        return allowMemory;
    }
    auto res = CJsonUtil::GetIntValue(perfConfig, "AllowMemory", allowMemory);
    cJSON_Delete(root);
    return res;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
