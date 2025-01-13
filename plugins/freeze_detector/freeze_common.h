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
    static const int MILLISECOND = 1000;
    static const int SYSTEM_RESULT_ID = 1;
    static const int APPLICATION_RESULT_ID = 0;
    static const inline std::string COLON = ":";
    static const inline std::string EVENT_DOMAIN = "DOMAIN";
    static const inline std::string EVENT_STRINGID = "STRINGID";
    static const inline std::string EVENT_TIMESTAMP = "TIMESTAMP";
    static const inline std::string EVENT_PID = "PID";
    static const inline std::string EVENT_TID = "TID";
    static const inline std::string EVENT_UID = "UID";
    static const inline std::string EVENT_PACKAGE_NAME = "PACKAGE_NAME";
    static const inline std::string EVENT_PROCESS_NAME = "PROCESS_NAME";
    static const inline std::string EVENT_MSG = "MSG";
    static const inline std::string FORE_GROUND = "FOREGROUND";
    static const inline std::string HIREACE_TIME = "HITRACE_TIME";
    static const inline std::string SYSRQ_TIME = "SYSRQ_TIME";
    static const inline std::string SYSEVENT_PIPELINE = "SysEventPipeline";
    static const inline std::string DISPLAY_POWER_INFO = "DisplayPowerInfo:";

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
    std::shared_ptr<FreezeRuleCluster> GetFreezeRuleCluster() const
    {
        return freezeRuleCluster_;
    }
    static void WriteStartInfoToFd(int fd, const std::string& msg);
    static void WriteEndInfoToFd(int fd, const std::string& msg);

private:
    std::shared_ptr<FreezeRuleCluster> freezeRuleCluster_;
    bool IsAssignedEvent(const std::string& domain, const std::string& stringId, int freezeId) const;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // HIVIEWDFX_FREEZE_COMMON_H