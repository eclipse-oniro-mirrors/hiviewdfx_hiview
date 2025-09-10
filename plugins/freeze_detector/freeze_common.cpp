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
#include <iomanip>

#include "freeze_common.h"

#include "hiview_logger.h"
#include "file_util.h"
#include "time_util.h"
namespace OHOS {
namespace HiviewDFX {
namespace {
    const int APPLICATION_RESULT_ID = 0;
    const int SYSTEM_RESULT_ID = 1;
    const int SYSTEM_WARNING_RESULT_ID = 2;
    const uint32_t PLACEHOLDER = 3;
}
DEFINE_LOG_LABEL(0xD002D01, "FreezeDetector");
FreezeCommon::FreezeCommon()
{
    freezeRuleCluster_ = nullptr;
}

FreezeCommon::~FreezeCommon()
{
    freezeRuleCluster_ = nullptr;
}

bool FreezeCommon::Init()
{
    freezeRuleCluster_ = std::make_shared<FreezeRuleCluster>();
    return freezeRuleCluster_->Init();
}

bool FreezeCommon::IsFreezeEvent(const std::string& domain, const std::string& stringId) const
{
    return IsApplicationEvent(domain, stringId) || IsSystemEvent(domain, stringId) ||
        IsSysWarningEvent(domain, stringId);
}

bool FreezeCommon::IsApplicationEvent(const std::string& domain, const std::string& stringId) const
{
    return IsAssignedEvent(domain, stringId, APPLICATION_RESULT_ID);
}
 
bool FreezeCommon::IsSystemEvent(const std::string& domain, const std::string& stringId) const
{
    return IsAssignedEvent(domain, stringId, SYSTEM_RESULT_ID);
}

bool FreezeCommon::IsSysWarningEvent(const std::string& domain, const std::string& stringId) const
{
    return IsAssignedEvent(domain, stringId, SYSTEM_WARNING_RESULT_ID);
}

bool FreezeCommon::IsAssignedEvent(const std::string& domain, const std::string& stringId, int freezeId) const
{
    if (freezeRuleCluster_ == nullptr) {
        HIVIEW_LOGW("freezeRuleCluster_ == nullptr.");
        return false;
    }

    std::map<std::string, std::pair<std::string, bool>> pairs;
    switch (freezeId) {
        case APPLICATION_RESULT_ID:
            pairs = freezeRuleCluster_->GetApplicationPairs();
            break;
        case SYSTEM_RESULT_ID:
            pairs = freezeRuleCluster_->GetSystemPairs();
            break;
        case SYSTEM_WARNING_RESULT_ID:
            pairs = freezeRuleCluster_->GetSysWarningPairs();
            break;
        default:
            return false;
    }
    for (auto const &pair : pairs) {
        if (stringId == pair.first && domain == pair.second.first) {
            return true;
        }
    }
    return false;
}

std::set<std::string> FreezeCommon::GetPrincipalStringIds() const
{
    std::set<std::string> set;
    if (freezeRuleCluster_ == nullptr) {
        HIVIEW_LOGW("freezeRuleCluster_ == nullptr.");
        return set;
    }
    auto applicationPairs = freezeRuleCluster_->GetApplicationPairs();
    auto systemPairs = freezeRuleCluster_->GetSystemPairs();
    auto sysWarningPairs = freezeRuleCluster_->GetSysWarningPairs();
    for (auto const &pair : applicationPairs) {
        if (pair.second.second) {
            set.insert(pair.first);
        }
    }
    for (auto const &pair : systemPairs) {
        if (pair.second.second) {
            set.insert(pair.first);
        }
    }
    for (auto const &pair : sysWarningPairs) {
        if (pair.second.second) {
            set.insert(pair.first);
        }
    }

    return set;
}

std::shared_ptr<FreezeRuleCluster> FreezeCommon::GetFreezeRuleCluster() const
{
    return freezeRuleCluster_;
}

void FreezeCommon::WriteTimeInfoToFd(int fd, const std::string& msg, bool isStart)
{
    if (isStart) {
        FileUtil::SaveStringToFd(fd, "\n---------------------------------------------------\n");
    }
    uint64_t ms = TimeUtil::GetMilliseconds();
    std::ostringstream timeStr;
    timeStr << msg << TimeUtil::TimestampFormatToDate(ms / TimeUtil::SEC_TO_MILLISEC, "%Y/%m/%d-%H:%M:%S")
        << ":" << std::setw(PLACEHOLDER) << std::setfill('0')
        << (ms % TimeUtil::SEC_TO_MILLISEC) << std::endl;
    FileUtil::SaveStringToFd(fd, timeStr.str());
    if (!isStart) {
        FileUtil::SaveStringToFd(fd, "---------------------------------------------------\n");
    }
}
}  // namespace HiviewDFX
}  // namespace OHOS