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

#include "native_leak_judge_state.h"
#include "native_leak_info.h"
#include "native_leak_detector.h"
#include "i_telemetry.h"
#include "fault_detector_manager.h"
#include "waiting_kill_list.h"
#include "detail_info_parser.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakJudgeState");

namespace {
constexpr int JUDGE_RECORD_NUM = 5;
constexpr int JUDGE_KILL_NUM = 1;
constexpr int MEM_DEVIDE_NUM = 2;
constexpr int REPEAT_REPORT_INTERVAL = SEC_PER_DAY;
constexpr int DUMP_REPORT_INTERVAL = 2 * SEC_PER_DAY;

bool IsBasicLogOnlyEvent(const string &eventName, bool isAllowDump)
{
    return eventName == "AVC_MEMORY" || (eventName == "ION_LEAK" && isAllowDump) ||
        eventName == "MEDIA_MEMORY";
}
}

ErrCode NativeLeakJudgeState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    const string &name = nlInfo->GetProcessName();
    const pid_t pid = nlInfo->GetPid();

    SampleRecord sr = nlInfo->GetLastSampleRecord();
    if (nlInfo->IsDumping()) {
        HIVIEW_LOGD("process: %{public}s:%{public}d is dumping", name.c_str(), pid);
        return FAILURE;
    }
    // over hard kill threshold in 2 sample period
    if (sr.overCnt > JUDGE_KILL_NUM && nlInfo->IsOverHardKillThresholdReally()) {
        return SetProcessDumpLogType(nlInfo);
    }
    if (sr.overCnt < JUDGE_RECORD_NUM) {
        HIVIEW_LOGD("process: %{public}s:%{public}d overCnt:%{public}d is too few", name.c_str(), pid, sr.overCnt);
        return FAILURE;
    }
    if (!nlInfo->GetPss()) {
        HIVIEW_LOGE("process: %{public}s:%{public}d GetPss failed", name.c_str(), pid);
        return FAILURE;
    }
    // over softkill threshold in 5 sample period
    if (nlInfo->IsOverSoftKillThreshold()) {
        return SetProcessDumpLogType(nlInfo);
    }
    sr = nlInfo->GetLastSampleRecord();
    if (sr.overCnt < JUDGE_RECORD_NUM) {
        HIVIEW_LOGE("process: %{public}s:%{public}d real PSS not over limit", name.c_str(), pid);
        return FAILURE;
    }
    if (!nlInfo->SetLeakGradeByPss()) {
        HIVIEW_LOGD("process: %{public}s:%{public}d SetLeakGradeByPss failed", name.c_str(), pid);
        return FAILURE;
    }
    return SetProcessDumpLogType(nlInfo);
}

ErrCode NativeLeakJudgeState::SetProcessDumpLogType(shared_ptr<NativeLeakInfo> &nlInfo)
{
    const string &name = nlInfo->GetProcessName();
    const LeakGrade leakGrade = nlInfo->GetLeakGrade();
    if (nlInfo->IsEverReported(leakGrade, REPEAT_REPORT_INTERVAL) &&
        nlInfo->ShouldBeKilled() == SHOULD_NOT_KILL) {
            HIVIEW_LOGD("Real process: %{public}s is erver reported Grade: %{public}s",
                name.c_str(), LEAK_GRADE_STRING[leakGrade]);
            return FAILURE;
    }
    if (!nlInfo->ShouldDump()) {
        HIVIEW_LOGD("process: %{public}s leakGrade is NO_LOG, ignore it", name.c_str());
        return FAILURE;
    }
    JudgeDumpLog(nlInfo);
    if (nlInfo->GetIsDumpHiprofilerDrop()) {
        HIVIEW_LOGI("process: %{public}s need to dump again", name.c_str());
        return SUCCESSED;
    }
    nlInfo->SetIsDumpHiprofilerDrop(false);
    HIVIEW_LOGI("process: %{public}s, leakGrade: %{public}s",
                name.c_str(), LEAK_GRADE_STRING[nlInfo->GetLeakGrade()]);
    return SUCCESSED;
}

void NativeLeakJudgeState::JudgeDumpLog(shared_ptr<NativeLeakInfo> &nlInfo)
{
    // judge dump type
    // dump leaktype when leakTypeMem > totalmem / memTypeCnt / 2
    uint32_t allDumpLogType = NO_LOG;
    SampleRecord lastSR = nlInfo->GetLastSampleRecord();
    int32_t mediaPss = nlInfo->GetMediaMem();
    int32_t avcPss = nlInfo->GetAvcMem();
    int32_t memTypeCnt = (lastSR.totalPss > 0 ? 1 : 0) +
                         (lastSR.ion > 0 ? 1 : 0) +
                         (lastSR.gpu > 0 ? 1 : 0) +
                         (mediaPss > 0 ? 1 : 0) +
                         (avcPss > 0 ? 1 : 0);
    if (memTypeCnt <= 0) {
        HIVIEW_LOGE("Total mem abnomal! process:%{public}s:%{public}d, PSS:%{public}d GPU:%{public}d ION:%{public}d",
                    nlInfo->GetProcessName().c_str(), nlInfo->GetPid(), lastSR.totalPss, lastSR.ion, lastSR.gpu);
        return;
    }
    HIVIEW_LOGD("totalPss used: %{public}d,"
                "ion used: %{public}d, gpu used: %{public}d, media: %{public}d, avcPss: %{public}d",
                lastSR.totalPss, lastSR.ion, lastSR.gpu, mediaPss, avcPss);
    int halfTotal = (lastSR.totalMem / MEM_DEVIDE_NUM) / memTypeCnt;

    // ion
    if (lastSR.ion > halfTotal) {
        HIVIEW_LOGD("should dump ion info, ion used: %{public}d", lastSR.ion);
        allDumpLogType = allDumpLogType | ION_LOG;
    }
    // gpu
    if (lastSR.gpu > halfTotal) {
        HIVIEW_LOGD("should dump gpu info, gpu used: %{public}d", lastSR.gpu);
        allDumpLogType = allDumpLogType | GPU_LOG;
    }
    // pss
    if (lastSR.totalPss > halfTotal) {
        HIVIEW_LOGD("should dump totalPss info, totalPss used: %{public}d", lastSR.totalPss);
        allDumpLogType = allDumpLogType | PSS_LOG;
    }
    // mediaPss
    if (mediaPss > halfTotal) {
        HIVIEW_LOGD("should dump mediaPss info, mediaPss used:%{public}d", mediaPss);
        allDumpLogType = allDumpLogType | MEDIA_LOG;
    }
    // avcPss
    if (avcPss > halfTotal) {
        HIVIEW_LOGD("should dump avcPss info, avcPss used: %{public}d", avcPss);
        allDumpLogType = allDumpLogType | AV_CODEC_LOG;
    }
    nlInfo->SetAllDumpLogType(allDumpLogType);
    JudgeExternalLog(nlInfo);
}

DumpLogType NativeLeakJudgeState::JudgeLogTypeByGrade(shared_ptr<NativeLeakInfo> &nlInfo,
    LeakGrade leakGrade, bool isAllowDump)
{
    const string &processName = nlInfo->GetProcessName();
    if (!isAllowDump && leakGrade >= LEAK_ERROR) {
        return BASIC_LOG;
    }
    if (leakGrade == LEAK_WARNING) {
        if (nlInfo->IsInThresholdList() && nlInfo->IsEverReported(leakGrade, DUMP_REPORT_INTERVAL)) {
            HIVIEW_LOGI("Current process: %{public}s has been reported in %{public}d hours, leakGrade: %{public}s",
                processName.c_str(), DUMP_REPORT_INTERVAL / SEC_PER_HOUR, LEAK_GRADE_STRING[leakGrade]);
            return BASIC_LOG;
        }
        return BASIC_AND_EXTERNAL_LOG;
    }
    if (nlInfo->ShouldBeKilled() >= SHOULD_WAIT_KILL && nlInfo->IsEverReported(leakGrade, DUMP_REPORT_INTERVAL)) {
        return BASIC_LOG;
    }
    if (leakGrade <= LEAK_CONTROL) {
        return BASIC_AND_EXTERNAL_LOG;
    }
    return NO_LOG;
}

DumpLogType NativeLeakJudgeState::JudgeExternalLog(shared_ptr<NativeLeakInfo> &nlInfo)
{
    bool isAllowDump = DfrUtil::GetInstance().IsBetaOrDevelopMode();
    const LeakGrade leakGrade = nlInfo->GetLeakGrade();
    DumpLogType logType = NO_LOG;

    // commen shouldbekill && not in app telemetry
    if (nlInfo->ShouldBeKilled() && !AppTelemetry(nlInfo, logType)) {
        nlInfo->FillExtPssInfo();
    }

    if (IsBasicLogOnlyEvent(nlInfo->GetEventName(), isAllowDump)) {
        return BASIC_LOG;
    }

    logType = JudgeLogTypeByGrade(nlInfo, leakGrade, isAllowDump);
    // 应用灰度
    if (!nlInfo->GetIsEverAppTelemetry() && AppTelemetry(nlInfo, logType)) {
        return logType;
    }

    // 系统灰度
    if (nlInfo->CouldTelemetry() && !nlInfo->IsEverSendTelemetryEvent()) {
        SystemTelemetry(nlInfo, logType);
    }
    if (nlInfo->InTelemetry()) {
        auto eq = nlInfo->GetEventQuota();
        GetTelemetry().Report(HiSysEvent::EventType::FAULT, eq, BUNDLE_QUOTA_EXHAUSTED,
                              "BundleQuota: " + to_string(eq.quota));
    }
    nlInfo->SetDumpLogType(logType);
    return logType;
}

bool NativeLeakJudgeState::AppTelemetry(shared_ptr<NativeLeakInfo> &nlInfo, DumpLogType &logType)
{
    const pid_t pid = nlInfo->GetPid();
    uint64_t appTelemetryType = 0;
    DfrUtil::GetInstance().GetAppTelemetryThresholdAndType(nlInfo->GetBundleName(), appTelemetryType);
    if (!appTelemetryType) {
        HIVIEW_LOGE("appTelemetryType is 0");
        return false;
    }
    HIVIEW_LOGI("into AppTelemetry and judge overlimit");
    DetailSmapsInfo detailSmapsInfo = DetailInfoParser::GetInstance().GetSmapsAndParse(pid, "EXT_PSS_LEAK");
    nlInfo->SetIsEverAppTelemetry(true);
    logType = BASIC_LOG;
    nlInfo->SetDumpLogType(logType);
    return true;
}

void NativeLeakJudgeState::SystemTelemetry(shared_ptr<NativeLeakInfo> &nlInfo, DumpLogType &logType)
{
    const pid_t pid = nlInfo->GetPid();
    const string &processName = nlInfo->GetProcessName();
    EventQuota eq = nlInfo->GetEventQuota();
    if (eq.subLeakType) {
        DetailSmapsInfo detailSmapsInfo = DetailInfoParser::GetInstance().GetSmapsAndParse(pid, "EXT_PSS_LEAK");
        const std::map<SubLeakType, int> leakTypeToSmapsInfo = {
            {SubLeakType::SUB_NATIVE_HEAP_LEAK, detailSmapsInfo.nativeHeap},
            {SubLeakType::SUB_ARKTS_LEAK, detailSmapsInfo.arktsHeap},
            {SubLeakType::SUB_JSVM_LEAK, detailSmapsInfo.jsvmHeap},
            {SubLeakType::SUB_ARKWEB_V8_LEAK, detailSmapsInfo.arkwebV8},
            {SubLeakType::SUB_ASHMEM_LEAK, detailSmapsInfo.ashmem},
            {SubLeakType::SUB_GPU_LEAK, nlInfo->GetLastSampleRecord().gpu},
            {SubLeakType::SUB_ION_LEAK, nlInfo->GetLastSampleRecord().ion},
            {SubLeakType::SUB_KOTLIN_LEAK, detailSmapsInfo.kotlinHeap},
            {SubLeakType::SUB_ARKTS_STA_LEAK, detailSmapsInfo.arktsStaHeap}
        };
        SubLeakType leakType = static_cast<SubLeakType>(eq.subLeakType);
        auto it = leakTypeToSmapsInfo.find(leakType);
        if (it != leakTypeToSmapsInfo.end()) {
            int smapsValue = it->second;
            HIVIEW_LOGD("subLeakType: %{public}d, %{public}d", leakType, smapsValue);
            if (smapsValue > eq.subLeakCaptureThreshold && leakType == SubLeakType::SUB_NATIVE_HEAP_LEAK) {
                HIVIEW_LOGI("into SystemTelemetry and overlimit SUB_NATIVE_HEAP_LEAK");
                logType = BASIC_AND_EXTERNAL_LOG;
                nlInfo->SetTelemetryFlag();
            } else if (smapsValue > eq.subLeakCaptureThreshold) {
                HIVIEW_LOGI("into SystemTelemetry and overlimit %{public}d", leakType);
                logType = BASIC_AND_EXTERNAL_LOG;
                nlInfo->SetTelemetrySubLeakType(leakType);
                nlInfo->SetTelemetryFlag();
            }
        }
    } else {
        HIVIEW_LOGD("%{public}s:%{public}d logType: %{public}d", processName.c_str(), pid, logType);
        if (logType >= BASIC_LOG) {
            HIVIEW_LOGI("%{public}s:%{public}d is in telemetry, should dump external log",
                processName.c_str(), pid);
            logType = BASIC_AND_EXTERNAL_LOG;
            nlInfo->SetTelemetryFlag();
        }
    }
}

ErrCode NativeLeakJudgeState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    return SUCCESSED;
}

bool NativeLeakJudgeState::JudgeOverThreshold(shared_ptr<NativeLeakInfo> &nlInfo)
{
    if (!nlInfo->GetPss()) {
        HIVIEW_LOGE("process: %{public}s:%{public}d GetPss failed", nlInfo->GetProcessName().c_str(), nlInfo->GetPid());
        return false;
    }
    int threshold = 0;
    if (nlInfo->GetProcessSourceInfo().type == COMMON_DETECT) {
        return nlInfo->IsOverHardKillThreshold() || nlInfo->IsOverSoftKillThreshold();
    }
    if (nlInfo->GetProcessSourceInfo().type == STANDBY_DETECT && NativeLeakDetector::GetInstance().GetStandByStatus()) {
        return nlInfo->IsInServiceLeakWhiteList(threshold) && nlInfo->ServiceLeakCheck(threshold);
    }
    return false;
}

void NativeLeakJudgeState::JudgeAndKillProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto it = static_pointer_cast<NativeLeakInfo>(faultInfo);
    const pid_t pid = it->GetPid();
    const string &processName = it->GetProcessName();
    auto isOverThreshold = JudgeOverThreshold(it);
    if (isOverThreshold && !FaultDetectorManager::CheckIfAppForeGround(processName)) {
        FaultStateBase *baseStateObj = NativeLeakStateContext::GetInstance().GetStateObj(PROC_REMOVAL_STATE);
        if (baseStateObj == nullptr) {
            HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_REMOVAL_STATE]);
            return;
        }
        auto removalStateObj = static_cast<NativeLeakRemovalState*>(baseStateObj);
        HIVIEW_LOGI("pid: %{public}d still over threshold, need kill in wait kill list", pid);
        removalStateObj->KillAndExeNextState(faultInfo, faultObj);
        WaitingKillList::GetInstance().DeleteProcessInWaitList(pid);
        return;
    }
    if (!isOverThreshold) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d, no leak any more, remove from waitkilllist",
            processName.c_str(), pid);
        WaitingKillList::GetInstance().DeleteLeakTypeInWaitList(pid,
            "NATIVE_LEAK:" + it->GetProcessInfoToKill().action);
        return;
    }
    HIVIEW_LOGI("process: %{public}s pid: %{public}d in foreground, should not be killed, but still in wait kill list",
        processName.c_str(), pid);
    return;
}

} // namespace HiviewDFX
} // namespace OHOS
