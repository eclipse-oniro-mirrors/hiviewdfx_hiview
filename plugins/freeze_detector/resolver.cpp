/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "resolver.h"

#include <sys/time.h>

#include "file_util.h"
#include "hiview_event_report.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "sys_event.h"
#include "sys_event_dao.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr int MINUTES_IN_HOUR = 60;
    constexpr int MIN_MATCH_NUM = 2;
    constexpr int DEFAULT_HOURS = 10;
    constexpr int SYS_MATCH_NUM = 1;
    constexpr int APP_MATCH_NUM = 1;
}

DEFINE_LOG_LABEL(0xD002D01, "FreezeDetector");
bool FreezeResolver::Init()
{
    if (freezeCommon_ == nullptr) {
        return false;
    }
    freezeRuleCluster_ = freezeCommon_->GetFreezeRuleCluster();
    if (freezeRuleCluster_ == nullptr) {
        return false;
    }
    dBHelper_ = std::make_unique<DBHelper>(freezeCommon_);
    vendor_ = std::make_unique<Vendor>(freezeCommon_);
    return vendor_->Init();
}

bool FreezeResolver::ResolveEvent(const WatchPoint& watchPoint,
    std::vector<WatchPoint>& list, std::vector<FreezeResult>& result) const
{
    if (freezeRuleCluster_ == nullptr || !freezeRuleCluster_->GetResult(watchPoint, result)) {
        return false;
    }
    unsigned long long timestamp = watchPoint.GetTimestamp();
    long pid = watchPoint.GetPid();
    long tid = watchPoint.GetTid();
    std::string packageName = watchPoint.GetPackageName().empty() ?
        watchPoint.GetProcessName() : watchPoint.GetPackageName();
    DBHelper::WatchParams params = {pid, tid, timestamp, packageName};
    for (auto& i : result) {
        long window = i.GetWindow();
        if (window == 0) {
            list.push_back(watchPoint);
        } else if (dBHelper_ != nullptr) {
            unsigned long long timeInterval = static_cast<unsigned long long>(std::abs(window) * MILLISECOND);
            unsigned long long start = window > 0 ? timestamp : timestamp - timeInterval;
            unsigned long long end = window > 0 ? timestamp + timeInterval : timestamp;
            unsigned long long delay =  static_cast<unsigned long long>(std::max(i.GetDelay(), 0L) * MILLISECOND);
            end += delay;
            dBHelper_->SelectEventFromDB(start, end, list, params, i);
        }
    }

    HIVIEW_LOGI("list size %{public}zu for pid:%{public}lu", list.size(), pid);
    return true;
}

bool FreezeResolver::JudgmentResult(const WatchPoint& watchPoint,
    const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const
{
    if (watchPoint.GetDomain() == "ACE" && watchPoint.GetStringId() == "UI_BLOCK_6S") {
        if (list.size() == result.size()) {
            HIVIEW_LOGI("ACE UI_BLOCK has UI_BLOCK_3S UI_BLOCK_6S UI_BLOCK_RECOVERED as UI_JANK");
            return false;
        }

        if (list.size() != (result.size() - 1)) {
            return false;
        }

        for (auto& i : list) {
            if (i.GetStringId() == "UI_BLOCK_RECOVERED") {
                return false;
            }
        }
        return true;
    }

    if (std::any_of(result.begin(), result.end(), [&list](auto& res) {
        return res.GetAction() == "or";
    })) {
        return list.size() >= MIN_MATCH_NUM;
    }

    if (IsSysWarning(watchPoint, list, result) || IsReportWarning(watchPoint, list, result)) {
        return true;
    }

    if (IsAppFreezeWarningForBetaVersion(watchPoint, list, result)) {
        return true;
    }

    if (list.size() == result.size()) {
        return true;
    }
    if (watchPoint.GetStringId() == "APP_INPUT_BLOCK"
        && (list.size() >= result.size() - APP_MATCH_NUM)) {
        return true;
    }
    return false;
}

bool FreezeResolver::IsReportWarning(const WatchPoint& watchPoint,
    const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const
{
    if (freezeCommon_ == nullptr) {
        return false;
    }
    return freezeCommon_->IsReportAppFreezeEvent(watchPoint.GetStringId()) &&
        (list.size() == result.size() - APP_MATCH_NUM);
}
bool FreezeResolver::IsSysWarning(const WatchPoint& watchPoint,
    const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const
{
    return watchPoint.GetStringId() == "SERVICE_WARNING" &&
            (list.size() == result.size() - SYS_MATCH_NUM);
}

bool FreezeResolver::IsAppFreezeWarningForBetaVersion(const WatchPoint& watchPoint,
    const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const
{
    return (watchPoint.GetStringId() == "THREAD_BLOCK_3S" ||
            watchPoint.GetStringId() == "LIFECYCLE_HALF_TIMEOUT") &&
            (list.size() == result.size() - APP_MATCH_NUM) &&
            Parameter::IsBetaVersion();
}
int FreezeResolver::ProcessEvent(const WatchPoint &watchPoint) const
{
    HIVIEW_LOGI("process event [%{public}s, %{public}s, %{public}lu]",
        watchPoint.GetDomain().c_str(), watchPoint.GetStringId().c_str(), watchPoint.GetPid());
    if (vendor_ == nullptr) {
        return -1;
    }
    std::vector<WatchPoint> list;
    std::vector<FreezeResult> result;
    if (!ResolveEvent(watchPoint, list, result)) {
        HIVIEW_LOGW("no rule for event [%{public}s, %{public}s]",
            watchPoint.GetDomain().c_str(), watchPoint.GetStringId().c_str());
        return -1;
    }

    if (!JudgmentResult(watchPoint, list, result)) {
        HIVIEW_LOGW("no match event for event [%{public}s, %{public}s]",
            watchPoint.GetDomain().c_str(), watchPoint.GetStringId().c_str());
        return -1;
    }

    WatchPoint watchPointCopy = watchPoint;
    vendor_->MergeEventLog(watchPointCopy, list, result);
    return 0;
}

std::string FreezeResolver::GetTimeZone() const
{
    std::string timeZone = "";
    struct timeval tv;
    struct timezone tz;
    if (gettimeofday(&tv, &tz) != 0) {
        HIVIEW_LOGE("failed to gettimeofday");
        return timeZone;
    }

    int hour = (-tz.tz_minuteswest) / MINUTES_IN_HOUR;
    timeZone = (hour >= 0) ? "+" : "-";

    int absHour = std::abs(hour);
    if (absHour < DEFAULT_HOURS) {
        timeZone.append("0");
    }
    timeZone.append(std::to_string(absHour));

    int minute = (-tz.tz_minuteswest) % MINUTES_IN_HOUR;
    int absMinute = std::abs(minute);
    if (absMinute < DEFAULT_HOURS) {
        timeZone.append("0");
    }
    timeZone.append(std::to_string(absMinute));

    return timeZone;
}
} // namespace HiviewDFX
} // namespace OHOS
