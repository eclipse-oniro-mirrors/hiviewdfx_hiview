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

#include "native_leak_xperf_detector.h"
#include "hiview_logger.h"
#include "dfr_util.h"
#include "fault_detector_manager.h"
#include "sys_event.h"
#include "hiview_global.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakXperfDetector");

NativeLeakXperfDetector::NativeLeakXperfDetector() {}

NativeLeakXperfDetector::~NativeLeakXperfDetector() {}

namespace {
constexpr int XPERF_EVENT_DETECT_INTERVAL = SEC_PER_HOUR;
constexpr int XPERF_REPEAT_REPORT_INTERVAL = 2 * 24 * SEC_PER_HOUR; // two days
constexpr int MAX_SEND_PID_NUM = 10;
}

using namespace std;

void NativeLeakXperfDetector::DetectXperfMemoryPeak(shared_ptr<Event> event)
{
    if (!DfrUtil::GetInstance().ShouldDetectAfterInterval(lastReviceXperfTime_, XPERF_EVENT_DETECT_INTERVAL)) {
        HIVIEW_LOGW("has recive xperf event repeated! lastReviceXperfTime: %{public}s",
            DfrUtil::GetInstance().TimeStampToStr(lastReviceXperfTime_).c_str());
        return;
    }
    HIVIEW_LOGI("processMemory xperf PROCESS_MEMORY_REPORT event received.");
    auto sysEvent = Event::DownCastTo<SysEvent>(event);
    string memInfoStr = sysEvent->GetEventValue("PROCESS_MEMORY_INFO");
    if (memInfoStr.empty()) {
        HIVIEW_LOGE("PROCESS_MEMORY_INFO is empty");
        return;
    }
    // jsonstr content contains '\', convert to json failed
    memInfoStr.erase(remove(memInfoStr.begin(), memInfoStr.end(), '\\'), memInfoStr.end());
    vector<ProcessMemory> processMemoryList;
    string faultEventId;
    if (!NativeLeakConfig::GetInstance().ParseStrToProcessMemInfo(memInfoStr, processMemoryList, faultEventId)) {
        HIVIEW_LOGE("memInfoStr parse failed!");
        return;
    }
    JudgeXperfProcessOverThreshold(processMemoryList, faultEventId);
}

void NativeLeakXperfDetector::JudgeXperfProcessOverThreshold(const vector<ProcessMemory> &processMemoryList,
                                                             const string &faultEventId)
{
    auto overThresholdDumpMap = NativeLeakConfig::GetInstance().GetNativeConfig().overThresholdDumpMap;
    vector<int32_t> sendPids;
    std::lock_guard<ffrt::mutex> lock(NativeLeakDetector::GetInstance().GetAllListMtx());
    for (const auto &pm : processMemoryList) {
        int pid = pm.pid;
        string processName = pm.name;
        if (NativeLeakDetector::GetInstance().allList_.count(pid) == 0 || processName.empty()) {
            continue;
        }
        if (NativeLeakDetector::GetInstance().IsLeakingPidsContain(pid)) {
            HIVIEW_LOGI("pid: %{public}d was in leakingPidsInfo_, name: %{public}s", pid, processName.c_str());
            continue;
        }
        shared_ptr<FaultInfoBase> &faultInfo = NativeLeakDetector::GetInstance().allList_[pid];
        ApplyDumpConfig(faultInfo, pm, overThresholdDumpMap, faultEventId, sendPids);
    }
    SendPidsToXperf(sendPids);
}

void NativeLeakXperfDetector::SendPidsToXperf(const vector<int32_t> &sendPids)
{
    if (sendPids.size() == 0) {
        return;
    }
    string jsonString = DfrUtil::GetInstance().JoinPidsStr(sendPids);
    string eventName = "XPERF_PID_INFO_SEND";
    OHOS::HiviewDFX::SysEventCreator sysEventCreator(
        "RELIABILITY", eventName, OHOS::HiviewDFX::SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("PROCESS_MEMORY_EXCEED_BASELINE", jsonString);
    auto sysEvent = std::make_shared<SysEvent>(eventName, nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    auto &hiviewInstance = HiviewGlobal::GetInstance();
    if (!hiviewInstance) {
        HIVIEW_LOGE("HiviewGlobal instance is null");
        return;
    }

    if (!hiviewInstance->PostSyncEventToTarget("XperfPlugin", event)) {
        HIVIEW_LOGE("PostSyncEventToTarget ERROR");
        return;
    }
    HIVIEW_LOGI("EventMsg %{public}s send success", jsonString.c_str());
}

void NativeLeakXperfDetector::ApplyDumpConfig(shared_ptr<FaultInfoBase> &faultInfo, const ProcessMemory &pm,
                                              unordered_map<string, DumpConfig> &overThresholdDumpMap,
                                              const string &faultEventId, vector<int32_t> &sendPids)
{
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    if (faultInfo->IsDumping()) {
        HIVIEW_LOGI("pid: %{public}d is dumping, name: %{public}s", pm.pid, pm.name.c_str());
        return;
    }
    nlInfo->SetProcessSourceInfo({XPERF_EVENT_DETECT, faultEventId});
    int thresholdInfo = INT32_MAX;
    int thresholdError = nlInfo->GetThresholdByGrade(LEAK_ERROR);
    if (overThresholdDumpMap.count(pm.name) > 0) {
        DumpConfig &process = overThresholdDumpMap[pm.name];
        if (process.thresholdInfo != 0) {
            thresholdInfo = process.thresholdInfo * KB_PER_MB + nlInfo->GetMemoryLimit();
        }
        if (process.thresholdError != 0) {
            thresholdError = process.thresholdError * KB_PER_MB + nlInfo->GetMemoryLimit();
        }
    }
    int swapPss = pm.swapPss;
    int pss = pm.pss;
    LeakGrade grade;
    if (pss + swapPss >= thresholdError) {
        grade = LEAK_ERROR;
        if (sendPids.size() < MAX_SEND_PID_NUM) {
            sendPids.push_back(pm.pid);
        }
    } else if (pss + swapPss >= thresholdInfo)  {
        grade = LEAK_INFO;
    } else {
        nlInfo->SetProcessSourceInfo({COMMON_DETECT, ""});
        return;
    }
    // judge over threshold, set leakGrade error, report wp, dumpLogType BASIC_AND_EXTERNAL_LOG
    if (nlInfo->IsXperfEventEverReported(XPERF_REPEAT_REPORT_INTERVAL, grade)) {
        HIVIEW_LOGI("pid: %{public}d, name: %{public}s has reported!", pm.pid, pm.name.c_str());
        return;
    }
    SampleRecord newSR(pss, swapPss, pm.rss);
    nlInfo->AddRealMemoryRecord(newSR);
    SetLogTypeByLeakGrade(nlInfo, grade);
    lastReviceXperfTime_ = time(nullptr);
    HIVIEW_LOGI("pid: %{public}d, name: %{public}s, pss: %{public}d, swapPss: %{public}d,"
                " notifyReason: %{public}s", pm.pid, pm.name.c_str(), pss, swapPss, faultEventId.c_str());
    NativeLeakDetector::GetInstance().AddToLeakingList(faultInfo);
}

void NativeLeakXperfDetector::SetLogTypeByLeakGrade(const shared_ptr<NativeLeakInfo> &nlInfo, LeakGrade grade)
{
    nlInfo->SetLeakGrade(grade);
    if (grade == LEAK_ERROR) {
        nlInfo->SetDumpLogType(static_cast<DumpLogType>(BASIC_AND_EXTERNAL_LOG));
        nlInfo->SetAllDumpLogType(static_cast<DumpLogType>(PSS_LOG));
    } else if (grade == LEAK_INFO) {
        nlInfo->SetDumpLogType(static_cast<DumpLogType>(BASIC_LOG));
        nlInfo->SetAllDumpLogType(static_cast<DumpLogType>(PSS_LOG | GPU_LOG | ION_LOG));
    }
}
} // namespace HiviewDFX
} // namespace OHOS
