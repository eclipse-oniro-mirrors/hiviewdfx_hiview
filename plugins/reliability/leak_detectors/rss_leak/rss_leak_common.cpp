/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "rss_leak_common.h"
#include "dfr_util.h"
#include "detail_info_parser.h"
#include "native_leak/native_leak_detector.h"
#include "native_leak/native_leak_info.h"
#include "i_telemetry.h"
#include <sstream>
#include <algorithm>

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002DA0, "RssLeakCommon");

namespace {
constexpr int RSS_THRESHOLD_DIVISOR = 4;
constexpr int RSS_THRESHOLD_MULTIPLIER = 3;
constexpr int RSS_HALF_DIVISOR = 2;
#define DFR_KMP_DUMP KMP_ ## K ## OTLIN
}

int32_t RssLeakCommon::CalcThreeQuarterThreshold(int32_t rssThreshold)
{
    return rssThreshold / RSS_THRESHOLD_DIVISOR * RSS_THRESHOLD_MULTIPLIER;
}

int32_t RssLeakCommon::CalcHalfThreshold(int32_t rssThreshold)
{
    return rssThreshold / RSS_HALF_DIVISOR;
}

bool RssLeakCommon::IsRssOverThreeQuarterThreshold(pid_t pid, int32_t rssThreshold)
{
    int rss = static_cast<int>(DfrUtil::GetInstance().GetProcessRss(pid));
    if (rss == 0) {
        HIVIEW_LOGE("get rss failed, pid: %{public}d, rss: %{public}d", pid, rss);
        return false;
    }
    int swaprss = DfrUtil::GetInstance().GetSwapRssByPid(pid);
    if (swaprss == -1) {
        HIVIEW_LOGE("get swapRss failed, pid: %{public}d, swaprss: %{public}d", pid, swaprss);
        return false;
    }
    int rssPlusSwapRss = rss + swaprss;
    int32_t threeQuarterThreshold = CalcThreeQuarterThreshold(rssThreshold);
    HIVIEW_LOGD("pid: %{public}d, rss: %{public}d, swaprss: %{public}d, rss+swaprss: %{public}d, "
                "3/4 threshold: %{public}d", pid, rss, swaprss, rssPlusSwapRss, threeQuarterThreshold);
    return rssPlusSwapRss > threeQuarterThreshold;
}

std::map<SubLeakType, LastLeakInfo>& RssLeakCommon::GetLastLeakInfoMap()
{
    static std::map<SubLeakType, LastLeakInfo> lastLeakInfoMap;
    return lastLeakInfoMap;
}

ffrt::mutex& RssLeakCommon::GetLastLeakMtx()
{
    static ffrt::mutex mtx;
    return mtx;
}

bool RssLeakCommon::IsDuplicateLeak(SubLeakType subLeakType, pid_t pid, const std::string& bundleName)
{
    std::lock_guard<ffrt::mutex> lock(GetLastLeakMtx());
    auto& leakMap = GetLastLeakInfoMap();
    auto it = leakMap.find(subLeakType);
    if (it != leakMap.end()) {
        if (it->second.pid == pid && it->second.bundleName == bundleName) {
            HIVIEW_LOGD("skip duplicate leakType: %{public}d, pid: %{public}d, bundleName: %{public}s",
                        subLeakType, pid, bundleName.c_str());
            return true;
        }
    }
    return false;
}

void RssLeakCommon::UpdateLastLeakInfo(SubLeakType subLeakType, pid_t pid, const std::string& bundleName)
{
    std::lock_guard<ffrt::mutex> lock(GetLastLeakMtx());
    auto& leakMap = GetLastLeakInfoMap();
    leakMap[subLeakType] = {pid, bundleName};
}

bool RssLeakCommon::IsMemoryOverThreshold(SubLeakType subLeakType, int32_t threshold,
                                          const DetailSmapsInfo& detailSmapsInfo)
{
    HIVIEW_LOGD("threshold: %{public}d, use nativeHeap: %{public}d, arktsHeap: %{public}d, "
                "jsvmHeap: %{public}d, arkwebV8: %{public}d, ashmem: %{public}d, kotlinHeap: %{public}d, "
                "arktsStaHeap: %{public}d, anonPageOther: %{public}d",
                threshold, detailSmapsInfo.nativeHeap, detailSmapsInfo.arktsHeap, detailSmapsInfo.jsvmHeap,
                detailSmapsInfo.arkwebV8, detailSmapsInfo.ashmem, detailSmapsInfo.kotlinHeap,
                detailSmapsInfo.arktsStaHeap, detailSmapsInfo.anonPageOther);
    switch (subLeakType) {
        case SUB_NATIVE_HEAP_LEAK:
            return threshold < detailSmapsInfo.nativeHeap;
        case SUB_ARKTS_LEAK:
            return threshold < detailSmapsInfo.arktsHeap;
        case SUB_JSVM_LEAK:
            return threshold < detailSmapsInfo.jsvmHeap;
        case SUB_ARKWEB_JS_LEAK:
            return threshold < detailSmapsInfo.arkwebV8;
        case SUB_ASHMEM_LEAK:
            return threshold < detailSmapsInfo.ashmem;
        case SUB_KMP_LEAK:
            return threshold < detailSmapsInfo.kotlinHeap;
        case SUB_ARKTS_STA_LEAK:
            return threshold < detailSmapsInfo.arktsStaHeap;
        case SUB_ANON_LEAK:
            return threshold < detailSmapsInfo.anonPageOther;
        default:
            return false;
    }
}

bool RssLeakCommon::CheckSubLeakOverThreshold(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                              std::string& bundleName)
{
    bundleName = telemetryInfo.bundleName;
    if (bundleName.empty()) {
        HIVIEW_LOGE("failed to get bundleName for pid: %{public}d", pid);
        return false;
    }

    if (IsDuplicateLeak(telemetryInfo.subLeakType, pid, bundleName)) {
        return false;
    }

    DetailSmapsInfo detailSmapsInfo = DetailInfoParser::GetInstance().GetSmapsAndParse(
        pid, "RSS_LEAK", SmapsStatType::RSS_TYPE);
    int32_t threshold = telemetryInfo.subleakOverThreshold;
    if (threshold < 0 || threshold == INT_MAX) {
        HIVIEW_LOGE("telemetryInfo threshold max");
        return false;
    }
    return IsMemoryOverThreshold(telemetryInfo.subLeakType, threshold, detailSmapsInfo);
}

bool RssLeakCommon::CheckSubLeakOverHalfThreshold(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
    int32_t rssThreshold)
{
    DetailSmapsInfo detail = DetailInfoParser::GetInstance().GetSmapsAndParse(
        pid, "RSS_LEAK", SmapsStatType::RSS_TYPE);
    int32_t halfTh = CalcHalfThreshold(rssThreshold);
    int subTypeValue = detail.nativeHeap;
    HIVIEW_LOGI("pid %{public}d subLeakType=%{public}d subTypeValue=%{public}d halfTh=%{public}d",
                pid, telemetryInfo.subLeakType, subTypeValue, halfTh);
    if (subTypeValue <= halfTh) {
        HIVIEW_LOGW("pid %{public}d subTypeValue %{public}d <= halfTh %{public}d, skip telemetry",
                    pid, subTypeValue, halfTh);
        return false;
    }
    return true;
}

void RssLeakCommon::HandleSystemTelemetryBySubLeakType(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                                       const std::string& bundleName,
                                                       std::shared_ptr<FaultInfoBase>& faultInfo)
{
    auto nlInfo = std::static_pointer_cast<NativeLeakInfo>(faultInfo);
    switch (telemetryInfo.subLeakType) {
        case SUB_ARKTS_LEAK: {
            UpdateLastLeakInfo(SUB_ARKTS_LEAK, pid, bundleName);
            nlInfo->DumpArkTsRawHeapInfo("RSS_SYSTEM_TELEMETRY");
            break;
        }
        case SUB_ARKWEB_JS_LEAK: {
            UpdateLastLeakInfo(SUB_ARKWEB_JS_LEAK, pid, bundleName);
            nlInfo->DumpRawHeapMemoryForThirdParty("RSS_SYSTEM_TELEMETRY",
                                                   OHOS::AppExecFwk::MemDumpType::ARKWEB_JS);
            break;
        }
        case SUB_JSVM_LEAK: {
            UpdateLastLeakInfo(SUB_JSVM_LEAK, pid, bundleName);
            nlInfo->DumpRawHeapMemoryForThirdParty("RSS_SYSTEM_TELEMETRY", OHOS::AppExecFwk::MemDumpType::JSVM);
            break;
        }
        case SUB_KMP_LEAK: {
            UpdateLastLeakInfo(SUB_KMP_LEAK, pid, bundleName);
            nlInfo->DumpRawHeapMemoryForThirdParty("RSS_SYSTEM_TELEMETRY",
                                                   OHOS::AppExecFwk::MemDumpType::DFR_KMP_DUMP);
            break;
        }
        case SUB_ARKTS_STA_LEAK: {
            UpdateLastLeakInfo(SUB_ARKTS_STA_LEAK, pid, bundleName);
            nlInfo->DumpArkTsRawHeapInfo("RSS_SYSTEM_TELEMETRY");
            break;
        }
        default:
            break;
    }
}

void RssLeakCommon::HandleSystemTelemetry(std::shared_ptr<FaultInfoBase>& faultInfo, SubLeakType subLeakType)
{
    auto nlInfo = std::static_pointer_cast<NativeLeakInfo>(faultInfo);
    nlInfo->SetLeakSource(LeakSource::RSS);
    if (!nlInfo->CouldTelemetry()) {
        return;
    }
    EventQuota eq = nlInfo->GetEventQuota();
    if (!eq.subLeakType) {
        return;
    }
    nlInfo->SetTelemetrySubLeakType(subLeakType);
    UpdateLastLeakInfo(subLeakType, faultInfo->GetPid(), faultInfo->GetBundleName());
    nlInfo->SetTelemetryFlag();
    nlInfo->SetAllDumpLogType(static_cast<DumpLogType>(PSS_LOG));
    nlInfo->SetDumpLogType(BASIC_AND_EXTERNAL_LOG);
    nlInfo->ReportMemoryLeakEvent();
    nlInfo->SetLeakGrade(LEAK_CONTROL);
    NativeLeakDetector::GetInstance().AddToLeakingList(faultInfo);
}

void RssLeakCommon::TriggerSystemTelemetry(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                           const std::string& bundleName)
{
    std::string name = DfrUtil::GetInstance().GetProcessName(pid);
    std::shared_ptr<FaultInfoBase> faultInfo = std::make_shared<NativeLeakInfo>(pid, name);
    auto nlInfo = std::static_pointer_cast<NativeLeakInfo>(faultInfo);
    nlInfo->SetEventName(RSS_LEAK_STR);
    if (!DfrUtil::GetInstance().IsDeviceLowmem()) {
        HandleSystemTelemetryBySubLeakType(pid, telemetryInfo, bundleName, faultInfo);
    }
    if (telemetryInfo.subLeakType == SUB_NATIVE_HEAP_LEAK &&
        !NativeLeakDetector::GetInstance().IsLeakingPidsContain(pid)) {
        HandleSystemTelemetry(faultInfo, telemetryInfo.subLeakType);
    } else if (telemetryInfo.subLeakType == SUB_ASHMEM_LEAK &&
               !NativeLeakDetector::GetInstance().IsLeakingPidsContain(pid)) {
        HandleSystemTelemetry(faultInfo, telemetryInfo.subLeakType);
    } else if (telemetryInfo.subLeakType == SUB_ANON_LEAK &&
               !NativeLeakDetector::GetInstance().IsLeakingPidsContain(pid)) {
        HandleSystemTelemetry(faultInfo, telemetryInfo.subLeakType);
    }
}

void RssLeakCommon::HandleAppTelemetry(pid_t pid, const std::string& bundleName,
                                       const TelemetryLeakInfo& telemetryInfo)
{
    if (telemetryInfo.subLeakType == SUB_NATIVE_HEAP_LEAK) {
        UpdateLastLeakInfo(SUB_NATIVE_HEAP_LEAK, pid, bundleName);
        DfrUtil::GetInstance().CollectAppTelemetry(pid, AppExecFwk::ATLT_RSS_NATIVE_HEAP);
    } else if (telemetryInfo.subLeakType == SUB_ARKTS_LEAK) {
        UpdateLastLeakInfo(SUB_ARKTS_LEAK, pid, bundleName);
        DfrUtil::GetInstance().CollectAppTelemetry(pid, AppExecFwk::ATLT_RSS_ARK_TS);
    } else if (telemetryInfo.subLeakType == SUB_KMP_LEAK) {
        UpdateLastLeakInfo(SUB_KMP_LEAK, pid, bundleName);
        DfrUtil::GetInstance().CollectAppTelemetry(pid, AppExecFwk::ATLT_RSS_KMP_DUMP);
    }
}

void RssLeakCommon::TriggerAppTelemetry(pid_t pid, const std::string& bundleName,
                                        const TelemetryLeakInfo& telemetryInfo)
{
    HIVIEW_LOGI("type: %{public}d, pid: %{public}d", telemetryInfo.subLeakType, pid);
    HandleAppTelemetry(pid, bundleName, telemetryInfo);
}

bool RssLeakCommon::TriggerRandomTelemetry(pid_t pid, int32_t rssThreshold,
                                           const TelemetryLeakInfo& telemetryInfo)
{
    HIVIEW_LOGI("TriggerRandomTelemetry pid=%{public}d threshold=%{public}d", pid, rssThreshold);
    if (!CheckSubLeakOverHalfThreshold(pid, telemetryInfo, rssThreshold)) {
        return false;
    }
    TriggerSystemTelemetry(pid, telemetryInfo, telemetryInfo.bundleName);
    return true;
}

bool RssLeakCommon::IsBundleInFilterList(const std::string& bundleName, const std::string& filterList)
{
    if (bundleName.empty() || filterList.empty()) {
        return false;
    }
    std::stringstream ss(filterList);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token == bundleName) {
            return true;
        }
    }
    return false;
}

} // namespace HiviewDFX
} // namespace OHOS
