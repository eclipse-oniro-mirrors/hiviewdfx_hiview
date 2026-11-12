/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <iostream>
#include <fstream>
#include <cstdio>
#include "notify_leak_dump_state.h"
#include "notify_leak_detector.h"
#include "dfr_util.h"
#include "hiview_logger.h"
#include "fault_common.h"
#include "hisysevent.h"
#include "old_log_cleaner.h"
#include "notify_leak_compress_common.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NotifyLeakDumpState");
namespace {
constexpr int64_t RAWHEAP_SIZE_BASELINE = 3 * BYTE_PER_MB;
constexpr int WAIT_STATIC_JSHEAP_TIMEOUT_S = 60;
}
using namespace std;

ErrCode NotifyLeakDumpState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    string oldPath = notifyInfo->GetNotifyDir() + "/" + notifyInfo->GetNotifyFileName();
    if (DfrUtil::GetInstance().IsFileEmpty(oldPath)) {
        return FAILURE;
    }
    NotifyType type = notifyInfo->GetNotifyType();
    HIVIEW_LOGI("notifyInfo is %{public}s", NOTIFY_TYPE_STRING[type]);
    if (type == LEAK_STACK) {
        string newPath = string(MEMORY_LEAK_PATH) + "/" + notifyInfo->GetBigMallocFileName();
        if (DfrUtil::GetInstance().RenameFile(oldPath, newPath) &&
            !(DfrUtil::GetInstance().IsFileEmpty(newPath))) {
            ParseLeakStack(notifyInfo, newPath);
        } else {
            HIVIEW_LOGW("notifyInfo is %{public}s, move file failed", NOTIFY_TYPE_STRING[type]);
        }
        return ChangeNextState(faultInfo, detectorObj);
    }
    const std::map<NotifyType, std::string> leakTypeMap = {
        {JS_LEAK, notifyInfo->GetJsLeakFileName()},
        {ARKWEB_LEAK, notifyInfo->GetArkWebLeakFileName()},
        {JSVM_LEAK, notifyInfo->GetJSVMLeakFileName()},
        {KMP_LEAK, notifyInfo->GetKMPLeakFileName()}
    };
    auto it = leakTypeMap.find(type);
    if (it == leakTypeMap.end()) {
        return FAILURE;
    }
    return ProcessLeakTypeFile(faultInfo, detectorObj, type, oldPath);
}

ErrCode NotifyLeakDumpState::ProcessLeakTypeFile(shared_ptr<FaultInfoBase> &faultInfo,
    FaultDetectorBase &detectorObj, NotifyType type, const string &oldPath)
{
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    if (!DfrUtil::GetInstance().IsDevelopMode() && notifyInfo->IsEverReported() && !notifyInfo->CouldTelemetry() &&
        !notifyInfo->IsAppWantEvent()) {
        HIVIEW_LOGE("not developMode, IsEverReported, not telemetry, app not setEventConfig, not report");
        OldLogCleaner::GetInstance().DeleteLogFile(oldPath);
        return FAILURE;
    }
    const std::map<NotifyType, std::string> leakTypeMap = {
        {JS_LEAK, notifyInfo->GetJsLeakFileName()}, {ARKWEB_LEAK, notifyInfo->GetArkWebLeakFileName()},
        {JSVM_LEAK, notifyInfo->GetJSVMLeakFileName()}, {KMP_LEAK, notifyInfo->GetKMPLeakFileName()}
    };
    auto it = leakTypeMap.find(type);
    string newPath = string(MEMORY_LEAK_PATH) + "/" + it->second;
    if (!IsValidFile(oldPath, notifyInfo) ||
        !OldLogCleaner::GetInstance().CopyFile(oldPath, newPath, notifyInfo->GetProcessName(), it->second) ||
        (DfrUtil::GetInstance().IsFileEmpty(newPath))) {
        OldLogCleaner::GetInstance().DeleteLogFile(oldPath);
        return ChangeNextState(faultInfo, detectorObj);
    }
    OldLogCleaner::GetInstance().DeleteLogFile(oldPath);
    if (type == JS_LEAK) {
        CopyStaticJsHeap(notifyInfo, detectorObj);
    }
    if (type == JS_LEAK && !DfrUtil::GetInstance().AppendMetadata(newPath)) {
        OldLogCleaner::GetInstance().DeleteLogFile(newPath);
        HIVIEW_LOGW("AppendMetadata failed for %{public}s, continuing to next state", newPath.c_str());
    }
    if (notifyInfo->IsEverReported() && !notifyInfo->CouldTelemetry()) {
        HIVIEW_LOGE("IsEverReported and not telemetry, not report again");
        return MaySendHiappevent(faultInfo, detectorObj);
    }
    notifyInfo->RecordReport();
    NotifyLeakCompressCommon::GetInstance().MayAddToCompressList(notifyInfo);
    return ChangeNextState(faultInfo, detectorObj);
}

void NotifyLeakDumpState::CopyStaticJsHeap(shared_ptr<NotifyLeakInfo> &notifyInfo, FaultDetectorBase &detectorObj)
{
    if (notifyInfo->IsDynamicArkts()) {
        HIVIEW_LOGI("JS_LEAK: dynamic arkts app, skip waiting for static_jsheap, pid=%{public}d",
            notifyInfo->GetPid());
        return;
    }
    string staticJsheapFileName;
    if (notifyInfo->HasStaticJsHeap()) {
        staticJsheapFileName = notifyInfo->GetStaticJsLeakNotifyFileName();
        HIVIEW_LOGI("JS_LEAK: static_jsheap already known, fileName=%{public}s", staticJsheapFileName.c_str());
    } else {
        auto &detector = static_cast<NotifyLeakDetector&>(detectorObj);
        HIVIEW_LOGI("JS_LEAK: waiting for static_jsheap callback, pid=%{public}d, timeout=%{public}ds",
            notifyInfo->GetPid(), WAIT_STATIC_JSHEAP_TIMEOUT_S);
        staticJsheapFileName = detector.WaitForStaticJsHeapFile(
            notifyInfo->GetPid(), notifyInfo->GetTid(), WAIT_STATIC_JSHEAP_TIMEOUT_S);
    }
    if (staticJsheapFileName.empty()) {
        HIVIEW_LOGI("JS_LEAK: no static_jsheap for pid %{public}d, proceed with jsheap only",
            notifyInfo->GetPid());
        return;
    }
    notifyInfo->SetStaticJsLeakNotifyFileName(staticJsheapFileName);
    string staticOldPath = notifyInfo->GetNotifyDir() + "/" + staticJsheapFileName;
    string staticNewPath = notifyInfo->GetStaticJsLeakFilePath();
    if (OldLogCleaner::GetInstance().CopyFile(staticOldPath, staticNewPath,
        notifyInfo->GetProcessName(), notifyInfo->GetStaticJsLeakFileName())) {
        HIVIEW_LOGI("static_jsheap copy success, dst: %{public}s", staticNewPath.c_str());
    } else {
        HIVIEW_LOGW("static_jsheap copy failed");
    }
    OldLogCleaner::GetInstance().DeleteLogFile(staticOldPath);
}

ErrCode NotifyLeakDumpState::MaySendHiappevent(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    if (notifyInfo->IsAppWantEvent()) {
        HIVIEW_LOGI("app SetEventConfig event, must call appreport");
        notifyInfo->SetOnlyAppreportFlag(true);
        return ChangeNextState(faultInfo, detectorObj);
    }
    return FAILURE;
}

void NotifyLeakDumpState::ParseLeakStack(shared_ptr<NotifyLeakInfo> &notifyInfo, string newPath)
{
    ifstream fin;
    fin.open(newPath.c_str());
    if (!fin.is_open()) {
        HIVIEW_LOGE("failed to open: %{public}s", newPath.c_str());
        return;
    }
    string line;
    bool isCaptured = false;
    while (getline(fin, line)) {
        if (line.find("Tid:") != line.npos) {
            isCaptured = true;
        } else if (line.find("Registers:") != line.npos) {
            isCaptured = false;
            break;
        }
        if (!isCaptured) {
            continue;
        }
        notifyInfo->AddLeakStack(line);
    }
    fin.close();
}

ErrCode NotifyLeakDumpState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    FaultStateType nextStateType;
    if (notifyInfo->GetOnlyAppreportFlag()) {
        nextStateType = PROC_APP_REPORT_STATE;
    } else {
        nextStateType = PROC_REPORT_STATE;
    }

    return detectorObj.ExeNextStateProcess(faultInfo, nextStateType);
}

bool NotifyLeakDumpState::IsValidFile(const string &filePath, shared_ptr<NotifyLeakInfo> &notifyInfo)
{
    struct stat rawheapInfo;
    if (stat(filePath.c_str(), &rawheapInfo) != 0) {
        HIVIEW_LOGE("get rawheap file stat failed, file name: %{public}s", filePath.c_str());
        return false;
    }
    notifyInfo->AddLeakStack("JS_LEAK, rawheap is valid, size: " + to_string(rawheapInfo.st_size));
    HIVIEW_LOGI("rawheap size: %{public}ld", rawheapInfo.st_size);
    return rawheapInfo.st_size > RAWHEAP_SIZE_BASELINE;
}

} // namespace HiviewDFX
} // namespace OHOS
