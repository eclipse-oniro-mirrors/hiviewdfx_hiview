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

#include "notify_leak_compress_common.h"
#include "notify_leak_compress_manager.h"
#include "telemetry_manager.h"
#include "fault_info_base.h"
#include "fault_common.h"
#include "old_log_cleaner.h"
#include "notify_leak_detector.h"
#include "dfr_util.h"
#include "ffrt.h"
#include "dfr_zip_util.h"
#include "hiview_logger.h"
#include "native_leak_state_context.h"
#include "native_leak_removal_state.h"
#include "parameters.h"

using namespace std;
namespace {
    constexpr int KILL_REPEAT_INTERVAL_S = 10 * 60 * 60; // 10 H
    constexpr int COMPRESS_DELAY_S = 5; // 5 s
}
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NotifyLeakCompressCommon");

NotifyLeakCompressCommon::NotifyLeakCompressCommon()
{
}

NotifyLeakCompressCommon::~NotifyLeakCompressCommon()
{
}

void NotifyLeakCompressCommon::NotifyCommonEvent(CommonEventType eventName)
{
    string nightcleanEable = OHOS::system::GetParameter("const.dfx.nightclean.jsheap", "-1");
    HIVIEW_LOGI("nightcleanEableis : %{public}s", nightcleanEable.c_str());
    switch (eventName) {
        case EVENT_SCREEN_OFF:
            isScreenOn_ = false;
            MayStartCompress();
            break;
        case EVENT_SCREEN_ON:
            isScreenOn_ = true;
            break;
        case EVENT_POWER_CONNECTED:
            isPowerConnected_ = true;
            MayStartCompress();
            break;
        case EVENT_POWER_DISCONNECTED:
            isPowerConnected_ = false;
            break;
        case EVENT_CHARGE_SLEEP:
            isChargeSleep_ = true;
            if (nightcleanEable != "-1") {
                DoScheduleKill();
            }
            MayStartCompress();
            break;
        case EVENT_NOT_CHARGE_SLEEP:
            isChargeSleep_ = false;
            break;
        default:
            HIVIEW_LOGE("unknown evevnt: %{public}d", eventName);
            break;
    }
}

bool NotifyLeakCompressCommon::CouldCompressCheck(const shared_ptr<NotifyLeakInfo> &notifyInfo)
{
    bool couldTelemetry = notifyInfo->CouldTelemetry();
    compressMutex_.lock();
    bool commonCompress = notifyInfo->CommonCompress();
    compressMutex_.unlock();
    bool isBetaOrDevelopMode = DfrUtil::GetInstance().IsBetaOrDevelopMode();
    return couldTelemetry || (commonCompress && isBetaOrDevelopMode);
}

void NotifyLeakCompressCommon::MayAddToCompressList(const shared_ptr<NotifyLeakInfo> &notifyInfo)
{
    if (!CouldCompressCheck(notifyInfo)) {
        HIVIEW_LOGI("no telemetry quota to compress, and cant not do common compress, bundleName: %{public}s",
            notifyInfo->GetBundleName().c_str());
        return;
    }
    {
        std::lock_guard<ffrt::mutex> lock(compressMutex_);
        notifyInfo->SetCommpressStatus(NEED_COMPRESS_AND_NOT_COMPRESSED);
        compressList_.push_back(notifyInfo);
        GetTelemetry().SetCompressListSize(compressList_.size());
    }
    OldLogCleaner::GetInstance().AddToBlackList(GetLeakFilePath(notifyInfo));
    if (notifyInfo->GetNotifyType() == JS_LEAK && notifyInfo->HasStaticJsHeap()) {
        OldLogCleaner::GetInstance().AddToBlackList(notifyInfo->GetStaticJsLeakFilePath());
    }
    MayStartCompress();
}

// Only used in compress_Mutex lock
int NotifyLeakCompressCommon::GetCommonCompressQueueSize()
{
    return compressList_.size();
}

string NotifyLeakCompressCommon::GetLeakFilePath(const shared_ptr<NotifyLeakInfo> &notifyInfo)
{
    if (notifyInfo == nullptr) {
        HIVIEW_LOGE("notifyInfo is null");
        return "";
    }
    NotifyType type = notifyInfo->GetNotifyType();
    const std::map<NotifyType, std::string> leakTypeToPathMap = {
        {JS_LEAK, notifyInfo->GetJsLeakFilePath()},
        {ARKWEB_LEAK, notifyInfo->GetArkWebLeakFilePath()},
        {JSVM_LEAK, notifyInfo->GetJSVMLeakFilePath()},
        {KMP_LEAK, notifyInfo->GetKMPLeakFilePath()}
    };
    auto it = leakTypeToPathMap.find(type);
    if (it != leakTypeToPathMap.end()) {
        return it->second;
    }
    HIVIEW_LOGW("get leak file path fail, bundleName: %{public}s", notifyInfo->GetBundleName().c_str());
    return "";
}

string NotifyLeakCompressCommon::GetLeakFileZipPath(const shared_ptr<NotifyLeakInfo> &notifyInfo, bool isTelemetry)
{
    if (notifyInfo == nullptr) {
        HIVIEW_LOGE("notifyInfo is null");
        return "";
    }
    NotifyType type = notifyInfo->GetNotifyType();
    const std::map<NotifyType, std::string> leakTypeToZipPathMap = {
        {JS_LEAK, isTelemetry ? notifyInfo->GetJsLeakFileZipPath() : notifyInfo->GetJsLeakFileZipPath()},
        {ARKWEB_LEAK, notifyInfo->GetArkWebLeakFileZipPath()},
        {JSVM_LEAK, notifyInfo->GetJSVMLeakFileZipPath()},
        {KMP_LEAK, notifyInfo->GetKMPLeakFileZipPath()}
    };
    auto it = leakTypeToZipPathMap.find(type);
    if (it != leakTypeToZipPathMap.end()) {
        return it->second;
    }
    HIVIEW_LOGW("get zip file path fail, bundleName: %{public}s", notifyInfo->GetBundleName().c_str());
    return "";
}

void NotifyLeakCompressCommon::DoScheduleKill()
{
    time_t curTime = time(nullptr);
    HIVIEW_LOGI("curTime is %{public}ld,lastKillTime is %{public}ld", curTime, lastKillTime);
    if (difftime(curTime, lastKillTime) < KILL_REPEAT_INTERVAL_S) {
        return;
    } else {
        lastKillTime = curTime;
    }
    std::shared_ptr<AAFwk::AbilityManagerClient> abilityMgrClient{nullptr};
    abilityMgrClient = AAFwk::AbilityManagerClient::GetInstance();
    if (abilityMgrClient == nullptr) {
        return;
    }
    HIVIEW_LOGI("ScheduleKillRecovery process");
    std::vector<AAFwk::AbilityRunningInfo> Infos;
    auto result = abilityMgrClient->GetAbilityRunningInfos(Infos);
    if (Infos.empty()) {
        return;
    }
    Infos.erase(Infos.begin());
    std::reverse(Infos.begin(), Infos.end());
    DoScheduleRecovery(Infos);
}

void NotifyLeakCompressCommon::DoScheduleRecovery(std::vector<AAFwk::AbilityRunningInfo> &Infos)
{
    std::shared_ptr<AppExecFwk::AppMgrClient> appMgrClient{nullptr};
    appMgrClient = make_shared<AppExecFwk::AppMgrClient>();
    if (appMgrClient == nullptr) {
        return;
    }
    for (auto info : Infos) {
        if (isChargeSleep_ == false) {
            break;
        }
        HIVIEW_LOGI("notifySleepClean processName: %{public}s", info.processName.c_str());
        int pid = info.pid;
        OHOS::AppExecFwk::AppFaultDataBySA faultData;
        faultData.faultType = OHOS::AppExecFwk::FaultDataType::SLEEP_CLEAN;
        faultData.pid = pid;
        faultData.notifyApp = true;
        faultData.waitSaveState = true;
        faultData.timeoutMarkers = "timeout";
        appMgrClient->NotifyAppFaultBySA(faultData);
    }
}

bool NotifyLeakCompressCommon::DoCompressFirstItemUnLocked(bool isTelemetry)
{
    // Re-verify conditions to avoid TOCTOU race condition
    if (isTelemetry) {
        if (!compressList_[0]->CouldTelemetry() || !GetTelemetry().CouldCompress()) {
            return false;
        }
    } else {
        if (!compressList_[0]->CommonCompress() || !NotifyLeakCompressManager::GetInstance().CouldCompress()) {
            return false;
        }
    }
    return true;
}

void NotifyLeakCompressCommon::MayStartCompress()
{
    bool needCompress = false;
    bool isTelemetry = false;
    {
        std::lock_guard<ffrt::mutex> lock(compressMutex_);
        if (compressList_.empty()) {
            return;
        }
        if (compressList_[0]->CouldTelemetry() && GetTelemetry().CouldCompress()) {
            needCompress = true;
            isTelemetry = true;
        } else if (compressList_[0]->CommonCompress() && NotifyLeakCompressManager::GetInstance().CouldCompress()) {
            needCompress = true;
            isTelemetry = false;
        }
    }
    if (needCompress) {
        DoCompressDelay(isTelemetry);
    }
}

void NotifyLeakCompressCommon::DoCompressDelay(bool isTelemetry)
{
    ffrt::submit([=] { // do compress after first report
        std::this_thread::sleep_for(std::chrono::seconds(COMPRESS_DELAY_S));
        string src;
        string dst;
        std::list<std::string> srcList;
        std::shared_ptr<NotifyLeakInfo> notifyInfo;
        {
            std::lock_guard<ffrt::mutex> lock(compressMutex_);
            if (compressList_.empty() || !DoCompressFirstItemUnLocked(isTelemetry)) {
                HIVIEW_LOGW("compressList is empty or no need to compress");
                return;
            }
            src = GetLeakFilePath(compressList_[0]);
            dst = GetLeakFileZipPath(compressList_[0], isTelemetry);
            if (src.empty() || dst.empty()) {
                HIVIEW_LOGE("Invalid src or dst path");
                return;
            }
            srcList = { src };
            if (compressList_[0]->GetNotifyType() == JS_LEAK && compressList_[0]->HasStaticJsHeap()) {
                string staticSrc = compressList_[0]->GetStaticJsLeakFilePath();
                if (!staticSrc.empty()) {
                    srcList.push_back(staticSrc);
                }
            }
            notifyInfo = compressList_[0];
            if (!DfrZipUtil::ZipMultiFiles(srcList, dst)) {
                HIVIEW_LOGW("failed to ZipMultiFiles, src: %{public}s, dst: %{public}s", src.c_str(), dst.c_str());
            } else {
                notifyInfo->SetCommpressStatus(NEED_COMPRESS_AND_COMPRESSED);
                HIVIEW_LOGI("success to ZipMultiFiles, src: %{public}s, dst: %{public}s", src.c_str(), dst.c_str());
                NotifyLeakDetector::GetInstance().ExeNextStateProcess(notifyInfo, PROC_REPORT_STATE);
                if (!isTelemetry) {
                    NotifyLeakCompressManager::GetInstance().UpdateCompressHistory(compressList_[0]->GetBundleName());
                }
            }
            compressList_.erase(compressList_.begin());
        }
        OldLogCleaner::GetInstance().RemoveFromBlackList(src);
        if (notifyInfo->GetNotifyType() == JS_LEAK && notifyInfo->HasStaticJsHeap()) {
            OldLogCleaner::GetInstance().RemoveFromBlackList(notifyInfo->GetStaticJsLeakFilePath());
        }
        if (isTelemetry) {
            GetTelemetry().ConsumeCompressQuota();
            GetTelemetry().ConsumeQuota(notifyInfo->GetFaultType(), notifyInfo->GetBundleName());
        }
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyLeakCompressCommon::DoCompressDelay")});
}

bool NotifyLeakCompressCommon::GetScreenOnStatus()
{
    return isScreenOn_;
}

bool NotifyLeakCompressCommon::GetPowerConnectStatus()
{
    return isPowerConnected_;
}

bool NotifyLeakCompressCommon::GetChargeSleepStatus()
{
    return isChargeSleep_;
}
} // namespace HiviewDFX
} // namespace OHOS