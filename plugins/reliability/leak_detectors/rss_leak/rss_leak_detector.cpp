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

#include "rss_leak_detector.h"
#include "dfr_util.h"
#include "native_leak/native_leak_detector.h"
#include "native_leak/native_leak_info.h"
#include "telemetry_manager.h"
#include <memory>
#include <algorithm>

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002DA0, "RssLeakDetector");

namespace {
constexpr int MAIN_INTERVAL = 20;
constexpr int MAIN_INTERVAL_TEST = 5;
// calculate 75% of the threshold
constexpr int RSS_THRESHOLD_DIVISOR = 4;
constexpr int RSS_THRESHOLD_MULTIPLIER = 3;
}

RssLeakDetector::RssLeakDetector()
{
    mainInterval_ = DfrUtil::GetInstance().IsMemTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL;
    HIVIEW_LOGI("RssLeakDetector initialized, mainInterval_: %{public}d", mainInterval_);
}

RssLeakDetector::~RssLeakDetector() {}

void RssLeakDetector::HandleDetector(int interval)
{
    // 应用灰度再开启
    if (ShouldDoMainProcess(interval)) {
        MainProcess();
    }
}

bool RssLeakDetector::ShouldDoMainProcess(int interval)
{
    if (mainInterval_ <= 0) {
        mainInterval_ = DfrUtil::GetInstance().IsMemTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL;
        return true;
    }
    mainInterval_ -= interval;
    return false;
}

ErrCode RssLeakDetector::MainProcess()
{
    HIVIEW_LOGI("=== RssLeakDetector MainProcess ===");
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    if (allInfoList_.empty()) {
        HIVIEW_LOGD("allInfoList_ is empty, skip MainProcess");
        return SUCCESSED;
    }
    std::vector<pid_t> needRemoveList;
    for (const auto &entry : allInfoList_) {
        pid_t pid = entry.first;
        const TelemetryLeakInfo &info = entry.second;

        int rss = static_cast<int>(DfrUtil::GetInstance().GetProcessRss(pid));
        if (rss == 0) {
            HIVIEW_LOGE("get rss failed, pid: %{public}d, rss: %{public}d",
                        pid, rss);
            continue;
        }
        int swaprss = DfrUtil::GetInstance().GetSwapRssByPid(pid);
        if (swaprss == -1) {
            HIVIEW_LOGE("get swapRss failed, pid: %{public}d, swaprss: %{public}d",
                        pid, swaprss);
            continue;
        }
        int32_t rssThreshold = DfrUtil::GetInstance().GetRssThreshold(pid);
        if (rssThreshold == -1) {
            HIVIEW_LOGW("get rss_threshold failed, pid: %{public}d", pid);
            continue;
        }
        rssThreshold = (rssThreshold / RSS_THRESHOLD_DIVISOR) * RSS_THRESHOLD_MULTIPLIER;
        int rssPlusSwapRss = rss + swaprss;
        HIVIEW_LOGD("pid: %{public}d, rss: %{public}d, swaprss: %{public}d, rss+swaprss: %{public}d, "
                    "threshold: %{public}d", pid, rss, swaprss, rssPlusSwapRss, rssThreshold);

        if (rssPlusSwapRss > rssThreshold) {
            JudgeNotify(pid, info);
            needRemoveList.push_back(pid);
        }
    }
    for (const auto pid : needRemoveList) {
        allInfoList_.erase(pid);
    }
    return SUCCESSED;
}

bool RssLeakDetector::CheckOverThreshold(pid_t pid, const TelemetryLeakInfo& telemetryInfo, std::string& bundleName)
{
    bundleName = telemetryInfo.bundleName;
    if (bundleName.empty()) {
        HIVIEW_LOGE("failed to get bundleName for pid: %{public}d", pid);
        return false;
    }

    std::map<SubLeakType, LastLeakInfo> leakTypeToInfo = {
        {SUB_NATIVE_HEAP_LEAK, lastNativeHeap_},
        {SUB_ARKTS_LEAK, lastArks_},
        {SUB_JSVM_LEAK, lastJsvm_},
        {SUB_ARKWEB_V8_LEAK, lastArkweb_},
        {SUB_ASHMEM_LEAK, lastAshmem_},
        {SUB_KOTLIN_LEAK, lastKmp_},
        {SUB_ARKTS_STA_LEAK, lastArksSta_}
    };
    auto it = leakTypeToInfo.find(telemetryInfo.subLeakType);
    if (it != leakTypeToInfo.end()) {
        if (it->second.pid == pid && it->second.bundleName == bundleName) {
            HIVIEW_LOGD("skip duplicate leakType: %{public}d, pid: %{public}d, bundleName: %{public}s",
                        telemetryInfo.subLeakType, pid, bundleName.c_str());
            return false;
        }
    }
    DetailSmapsInfo detailSmapsInfo = DetailInfoParser::GetInstance().GetSmapsAndParse(pid, "RSS_LEAK",
                                                                                       SmapsStatType::RSS_TYPE);
    int32_t threshold = telemetryInfo.subleakOverThreshold;
    if (threshold < 0 || threshold == INT_MAX) {
        HIVIEW_LOGE("telemetryInfo threshold max");
        return false;
    }
    return IsMemoryOverThreshold(telemetryInfo.subLeakType, threshold, detailSmapsInfo);
}

bool RssLeakDetector::IsMemoryOverThreshold(SubLeakType subLeakType, int32_t threshold,
                                            const DetailSmapsInfo& detailSmapsInfo)
{
    HIVIEW_LOGD("threshold: %{public}d, use nativeHeap: %{public}d, arktsHeap: %{public}d, "
                "jsvmHeap: %{public}d, arkwebV8: %{public}d, ashmem: %{public}d, kotlinHeap: %{public}d, "
                "arktsStaHeap: %{public}d",
                threshold, detailSmapsInfo.nativeHeap, detailSmapsInfo.arktsHeap, detailSmapsInfo.jsvmHeap,
                detailSmapsInfo.arkwebV8, detailSmapsInfo.ashmem, detailSmapsInfo.kotlinHeap,
                detailSmapsInfo.arktsStaHeap);
    switch (subLeakType) {
        case SUB_NATIVE_HEAP_LEAK:
            return threshold < detailSmapsInfo.nativeHeap;
        case SUB_ARKTS_LEAK:
            return threshold < detailSmapsInfo.arktsHeap;
        case SUB_JSVM_LEAK:
            return threshold < detailSmapsInfo.jsvmHeap;
        case SUB_ARKWEB_V8_LEAK:
            return threshold < detailSmapsInfo.arkwebV8;
        case SUB_ASHMEM_LEAK:
            return threshold < detailSmapsInfo.ashmem;
        case SUB_KOTLIN_LEAK:
            return threshold < detailSmapsInfo.kotlinHeap;
        case SUB_ARKTS_STA_LEAK:
            return threshold < detailSmapsInfo.arktsStaHeap;
        default:
            return false;
    }
}

void RssLeakDetector::HandleSystemTelemetry(std::shared_ptr<FaultInfoBase>& faultInfo, SubLeakType subLeakType)
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
    std::map<SubLeakType, LastLeakInfo> leakTypeToInfo = {
        {SUB_NATIVE_HEAP_LEAK, lastNativeHeap_},
        {SUB_ARKTS_LEAK, lastArks_},
        {SUB_JSVM_LEAK, lastJsvm_},
        {SUB_ARKWEB_V8_LEAK, lastArkweb_},
        {SUB_ASHMEM_LEAK, lastAshmem_},
        {SUB_KOTLIN_LEAK, lastKmp_},
        {SUB_ARKTS_STA_LEAK, lastArksSta_}
    };
    auto it = leakTypeToInfo.find(subLeakType);
    if (it != leakTypeToInfo.end()) {
        it->second.pid = faultInfo->GetPid();
        it->second.bundleName = faultInfo->GetBundleName();
    }
    nlInfo->SetTelemetryFlag();
    nlInfo->SetAllDumpLogType(static_cast<DumpLogType>(PSS_LOG));
    nlInfo->SetDumpLogType(BASIC_AND_EXTERNAL_LOG);
    nlInfo->ReportMemoryLeakEvent();
    nlInfo->SetLeakGrade(LEAK_CONTROL);
    NativeLeakDetector::GetInstance().AddToLeakingList(faultInfo);
}

void RssLeakDetector::HandleAppTelemetry(pid_t pid, const std::string& bundleName, SubLeakType subLeakType)
{
    if (subLeakType == SUB_NATIVE_HEAP_LEAK) {
        lastNativeHeap_.pid = pid;
        lastNativeHeap_.bundleName = bundleName;
        DfrUtil::GetInstance().CollectAppTelemetry(pid, AppExecFwk::ATLT_RSS_NATIVE_HEAP);
    } else if (subLeakType == SUB_ARKTS_LEAK) {
        lastArks_.pid = pid;
        lastArks_.bundleName = bundleName;
        DfrUtil::GetInstance().CollectAppTelemetry(pid, AppExecFwk::ATLT_RSS_ARK_TS);
    } else if (subLeakType == SUB_KOTLIN_LEAK) {
        lastKmp_.pid = pid;
        lastKmp_.bundleName = bundleName;
        DfrUtil::GetInstance().CollectAppTelemetry(pid, AppExecFwk::ATLT_RSS_KMP_DUMP);
    }
    RemoveTelemetryInfo(bundleName);
}

void RssLeakDetector::HandleSystemTelemetryBySubLeakType(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                                         const std::string& bundleName,
                                                         std::shared_ptr<FaultInfoBase>& faultInfo)
{
    auto nlInfo = std::static_pointer_cast<NativeLeakInfo>(faultInfo);
    switch (telemetryInfo.subLeakType) {
        case SUB_ARKTS_LEAK: {
            lastArks_.pid = pid;
            lastArks_.bundleName = bundleName;
            nlInfo->DumpArkTsRawHeapInfo("RSS_SYSTEM_TELEMETRY");
            break;
        }
        case SUB_ARKWEB_V8_LEAK: {
            lastArkweb_.pid = pid;
            lastArkweb_.bundleName = bundleName;
            nlInfo->DumpRawHeapMemoryForThirdParty("RSS_SYSTEM_TELEMETRY",
                                                   OHOS::AppExecFwk::MemDumpType::ARKWEB_JS);
            break;
        }
        case SUB_JSVM_LEAK: {
            lastJsvm_.pid = pid;
            lastJsvm_.bundleName = bundleName;
            nlInfo->DumpRawHeapMemoryForThirdParty("RSS_SYSTEM_TELEMETRY", OHOS::AppExecFwk::MemDumpType::JSVM);
            break;
        }
        case SUB_KOTLIN_LEAK: {
            lastKmp_.pid = pid;
            lastKmp_.bundleName = bundleName;
            nlInfo->DumpRawHeapMemoryForThirdParty("RSS_SYSTEM_TELEMETRY",
                                                   OHOS::AppExecFwk::MemDumpType::KMP_KOTLIN);
            break;
        }
        case SUB_ARKTS_STA_LEAK: {
            lastArksSta_.pid = pid;
            lastArksSta_.bundleName = bundleName;
            nlInfo->DumpArkTsRawHeapInfo("RSS_SYSTEM_TELEMETRY");
            break;
        }
        default:
            break;
    }
}

void RssLeakDetector::HandleOverThreshold(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                          const std::string& bundleName)
{
    if (telemetryInfo.telemetrySource == SYSTEM_TELEMETRY) {
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
        }
    } else if (telemetryInfo.telemetrySource == APP_TELEMETRY) {
        HIVIEW_LOGI("type: %{public}d, pid: %{public}d", telemetryInfo.subLeakType, pid);
        HandleAppTelemetry(pid, bundleName, telemetryInfo.subLeakType);
    }
}

void RssLeakDetector::DoSystemTelemetry(pid_t pid, const TelemetryLeakInfo &telemetryInfo)
{
    HandleOverThreshold(pid, telemetryInfo, telemetryInfo.bundleName);
}

void RssLeakDetector::JudgeNotify(pid_t pid, TelemetryLeakInfo telemetryInfo)
{
    std::string bundleName;
    if (CheckOverThreshold(pid, telemetryInfo, bundleName)) {
        HandleOverThreshold(pid, telemetryInfo, bundleName);
    }
}

void RssLeakDetector::NotifyProcessCreate(pid_t pid)
{
    std::vector<TelemetryLeakInfo> infos = GetTelemetry().GetTelemetryInfos(RSS_LEAK_TYPE);
    if (infos.empty()) {
        return;
    }
    std::string bundleName = DfrUtil::GetInstance().GetProcessName(pid);
    AddProcessInfo(pid, bundleName);
}

void RssLeakDetector::NotifyProcessDied(pid_t pid)
{
    RemoveProcessInfo(pid);
}

bool RssLeakDetector::AddAppTelemetryInfoToList(std::shared_ptr<FaultInfoBase> &faultInfo)
{
    if (faultInfo == nullptr) {
        HIVIEW_LOGE("AddAppTelemetryInfoToList faultInfo is null");
        return false;
    }
    std::string bundleName = faultInfo->GetBundleName();
    if (bundleName.empty()) {
        return false;
    }
    uint64_t subLeakType = 0;
    int threshold = 0;
    DfrUtil::GetInstance().GetAppTelemetryThresholdAndType(bundleName, subLeakType, &threshold);

    TelemetryLeakInfo telemetryInfo;
    telemetryInfo.leakType = RSS_LEAK_TYPE;
    telemetryInfo.telemetrySource = APP_TELEMETRY;
    telemetryInfo.subleakOverThreshold = threshold < 0 ? INT_MAX : threshold;
    telemetryInfo.bundleName = bundleName;
    telemetryInfo.pid = faultInfo->GetPid();

    if (subLeakType == AppExecFwk::ATLT_RSS_NATIVE_HEAP) {
        telemetryInfo.subLeakType = SUB_NATIVE_HEAP_LEAK;
    } else if (subLeakType == AppExecFwk::ATLT_RSS_ARK_TS) {
        telemetryInfo.subLeakType = SUB_ARKTS_LEAK;
    } else if (subLeakType == AppExecFwk::ATLT_RSS_KMP_DUMP) {
        telemetryInfo.subLeakType = SUB_KOTLIN_LEAK;
    } else {
        return false;
    }
    return AddTelemetryInfo(bundleName, telemetryInfo);
}

bool RssLeakDetector::AddProcessInfo(pid_t pid, std::string bundleName)
{
    if (bundleName.empty()) {
        return false;
    }

    TelemetryLeakInfo telemetryInfo;
    // Check system telemetry first
    std::vector<TelemetryLeakInfo> infos = GetTelemetry().GetTelemetryInfos(RSS_LEAK_TYPE);

    auto it = std::find_if(infos.begin(), infos.end(), [&bundleName](const TelemetryLeakInfo& info) {
        return info.bundleName == bundleName;
    });
    if (it != infos.end()) {
        telemetryInfo = *it;
    } else {
        std::lock_guard<ffrt::mutex> telemetryLock(telemetryInfoMtx_);
        auto appIt = telemetryInfoList_.find(bundleName);
        if (appIt == telemetryInfoList_.end()) {
            return false;
        }
        telemetryInfo = appIt->second;
    }
    std::lock_guard<ffrt::mutex> allLock(allListMtx_);
    allInfoList_[pid] = telemetryInfo;
    if (telemetryInfo.telemetrySource >= 0 && telemetryInfo.telemetrySource < TELEMETRY_SOURCE_MAX) {
        HIVIEW_LOGI("pid: %{public}d, bundleName: %{public}s, leakType: %{public}lu, source: %{public}s"
            " add to leak list", pid, bundleName.c_str(), telemetryInfo.leakType,
            TELEMETRY_SOURCE_NAME[telemetryInfo.telemetrySource]);
    } else {
        HIVIEW_LOGW("invalid telemetrySource: %{public}d", telemetryInfo.telemetrySource);
    }
    return true;
}

// app telemetry
bool RssLeakDetector::DoRssLeak(std::shared_ptr<FaultInfoBase> &faultInfo, TelemetrySource telemetrySource)
{
    if (DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        return false;
    }
    if (telemetrySource == APP_TELEMETRY) {
        if (!AddAppTelemetryInfoToList(faultInfo)) {
            return false;
        }
        std::string bundleName = faultInfo->GetBundleName();
        return AddProcessInfo(faultInfo->GetPid(), bundleName);
    }
    return false;
}

void RssLeakDetector::RemoveProcessInfo(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    auto it = allInfoList_.find(pid);
    if (it != allInfoList_.end()) {
        allInfoList_.erase(it);
        HIVIEW_LOGI("RemoveProcessInfo pid: %{public}d", pid);
    }
}

size_t RssLeakDetector::GetProcessInfoCount()
{
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    return allInfoList_.size();
}

bool RssLeakDetector::AddTelemetryInfo(const std::string &bundleName, const TelemetryLeakInfo &telemetryInfo)
{
    if (DfrUtil::GetInstance().IsBetaVersion()) {
        return false;
    }
    if (bundleName.empty()) {
        HIVIEW_LOGW("bundleName is empty");
        return false;
    }
    // Only accept APP_TELEMETRY, SYSTEM_TELEMETRY is managed by TelemetryManager
    if (telemetryInfo.telemetrySource != APP_TELEMETRY) {
        if (telemetryInfo.telemetrySource >= 0 && telemetryInfo.telemetrySource < TELEMETRY_SOURCE_MAX) {
            HIVIEW_LOGW("Only APP_TELEMETRY is allowed, source: %{public}s",
                TELEMETRY_SOURCE_NAME[telemetryInfo.telemetrySource]);
        } else {
            HIVIEW_LOGW("Invalid telemetrySource: %{public}d", static_cast<int>(telemetryInfo.telemetrySource));
        }
        return false;
    }

    std::lock_guard<ffrt::mutex> lock(telemetryInfoMtx_);
    telemetryInfoList_[bundleName] = telemetryInfo;
    HIVIEW_LOGD("bundleName: %{public}s, leakType: %{public}lu, source: APP_TELEMETRY,"
                " telemetryListSize: %{public}lu", bundleName.c_str(), telemetryInfo.leakType,
                telemetryInfoList_.size());
    return true;
}

bool RssLeakDetector::RemoveTelemetryInfo(const std::string &bundleName)
{
    if (bundleName.empty()) {
        HIVIEW_LOGW("RemoveTelemetryInfo bundleName is empty");
        return false;
    }
    std::lock_guard<ffrt::mutex> lock(telemetryInfoMtx_);
    auto it = telemetryInfoList_.find(bundleName);
    if (it != telemetryInfoList_.end()) {
        telemetryInfoList_.erase(it);
        HIVIEW_LOGI("RemoveTelemetryInfo bundleName: %{public}s, telemetryListSize: %{public}zu",
                    bundleName.c_str(), telemetryInfoList_.size());
        return true;
    }
    return false;
}

void RssLeakDetector::ClearTelemetryInfo()
{
    std::lock_guard<ffrt::mutex> lock(telemetryInfoMtx_);
    size_t count = telemetryInfoList_.size();
    telemetryInfoList_.clear();
    HIVIEW_LOGI("ClearTelemetryInfo cleared %{public}zu items", count);
}

void RssLeakDetector::OnChangeState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType)
{
    HIVIEW_LOGD("RssLeakDetector::OnChangeState not implemented");
}

ErrCode RssLeakDetector::ExeNextStateProcess(std::shared_ptr<FaultInfoBase> faultInfo, FaultStateType stateType)
{
    HIVIEW_LOGD("RssLeakDetector::ExeNextStateProcess not implemented");
    return SUCCESSED;
}

FaultStateBase *RssLeakDetector::GetStateObj(FaultStateType stateType)
{
    HIVIEW_LOGD("RssLeakDetector::GetStateObj not implemented");
    return nullptr;
}

void RssLeakDetector::SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt)
{
    HIVIEW_LOGD("RssLeakDetector::SetAppResourceLimit not implemented");
}

} // namespace HiviewDFX
} // namespace OHOS
