/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef HIVIEWDFX_FREEZE_COMMON_H
#define HIVIEWDFX_FREEZE_COMMON_H

#include <list>
#include <memory>
#include <set>
#include <vector>

#include "rule_cluster.h"

namespace OHOS {
namespace HiviewDFX {
const std::string FREEZE_DETECTOR_PLUGIN_NAME = "FreezeDetector";
const std::string FREEZE_DETECTOR_PLUGIN_VERSION = "1.0";
class FreezeCommon {
public:
    static constexpr const char* const EVENT_PID = "PID";
    static constexpr const char* const EVENT_TID = "TID";
    static constexpr const char* const EVENT_UID = "UID";
    static constexpr const char* const EVENT_PACKAGE_NAME = "PACKAGE_NAME";
    static constexpr const char* const EVENT_PROCESS_NAME = "PROCESS_NAME";
    static constexpr const char* const EVENT_MSG = "MSG";
    static constexpr const char* const HIREACE_TIME = "HITRACE_TIME";
    static constexpr const char* const SYSRQ_TIME = "SYSRQ_TIME";
    static constexpr const char* const TERMINAL_THREAD_STACK = "TERMINAL_THREAD_STACK";
    static constexpr const char* const TELEMETRY_ID = "TELEMETRY_ID";
    static constexpr const char* const TRACE_NAME = "TRACE_NAME";
    static constexpr const char* const PB_EVENTS[] = {
        "UI_BLOCK_3S", "THREAD_BLOCK_3S", "BUSSNESS_THREAD_BLOCK_3S", "LIFECYCLE_HALF_TIMEOUT"
    };
    static constexpr const char* const EVENT_TRACE_ID = "HITRACE_ID";
    static constexpr const char* const EVENT_SPAN_ID = "SPAN_ID";
    static constexpr const char* const EVENT_PARENT_SPAN_ID = "PARENT_SPAN_ID";
    static constexpr const char* const EVENT_TRACE_FLAG = "TRACE_FLAG";

    FreezeCommon();
    ~FreezeCommon();

    bool Init();
    bool IsFreezeEvent(const std::string& domain, const std::string& stringId) const;
    bool IsApplicationEvent(const std::string& domain, const std::string& stringId) const;
    bool IsBetaVersion() const;
    bool IsSystemEvent(const std::string& domain, const std::string& stringId) const;
    bool IsSysWarningEvent(const std::string& domain, const std::string& stringId) const;
    bool IsSystemResult(const FreezeResult& result) const;
    bool IsSysWarningResult(const FreezeResult& result) const;
    bool IsApplicationResult(const FreezeResult& result) const;
    std::set<std::string> GetPrincipalStringIds() const;
    std::shared_ptr<FreezeRuleCluster> GetFreezeRuleCluster() const;
    static void WriteStartInfoToFd(int fd, const std::string& msg);
    static void WriteEndInfoToFd(int fd, const std::string& msg);

private:
    std::shared_ptr<FreezeRuleCluster> freezeRuleCluster_;
    bool IsAssignedEvent(const std::string& domain, const std::string& stringId, int freezeId) const;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // HIVIEWDFX_FREEZE_COMMON_H