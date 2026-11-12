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

#include "native_leak_report_state.h"
#include "native_leak_info.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "fault_detector_manager.h"
#include "parameters.h"
#include "i_telemetry.h"
#include "dfr_util.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakReportState");

namespace {
// 浏览器渲染进程名后缀，按后缀匹配以兼容不同浏览器的包名前缀
constexpr char BROWSER_RENDER_SUFFIX[] = "browser:render";

// 判断字符串是否以指定后缀结尾
bool EndsWith(const string &str, const string &suffix)
{
    return str.length() >= suffix.length() &&
        str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
}

void NativeLeakReportState::HandleAppLeakReport(shared_ptr<FaultInfoBase> &faultInfo)
{
    if (faultInfo == nullptr) {
        HIVIEW_LOGE("HandleAppLeakReport kernelInfo is null");
        return;
    }

    const pid_t pid = faultInfo->GetPid();
    if (!DfrUtil::GetInstance().IsApp(pid)) {
        return;
    }

    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    SampleRecord lastSR = nlInfo->GetLastSampleRecord();
    if (lastSR.totalMem < 0 || lastSR.gpu < 0 || lastSR.ion < 0 || lastSR.totalPss < 0) {
        HIVIEW_LOGE("data less than 0, totalMem is %{public}d, gpu is %{public}d,"
            "ion is %{public}d, totalPss is %{public}d", lastSR.totalMem, lastSR.gpu, lastSR.ion, lastSR.totalPss);
        return;
    }
    unsigned long leakSize = static_cast<unsigned long>(lastSR.totalMem);
    AppExecFwk::LeakDetailInfo leakDetailInfo{};
    AppExecFwk::LeakType leakType = AppExecFwk::LeakType::PSS_MEMORY;

    leakDetailInfo.arktsSize = 0;
    leakDetailInfo.nativeSize = static_cast<unsigned long>(lastSR.totalPss);
    leakDetailInfo.ionSize = static_cast<unsigned long>(lastSR.ion);
    leakDetailInfo.gpuSize = static_cast<unsigned long>(lastSR.gpu);
    leakDetailInfo.ashmemSize = 0;
    leakDetailInfo.otherSize = leakSize - (leakDetailInfo.nativeSize + leakDetailInfo.ionSize + leakDetailInfo.gpuSize);

    DfrUtil::GetInstance().NotifyWarningToApp(pid, leakType, leakSize, leakDetailInfo);
}

NativeLeakReportState::EventFrameInfo NativeLeakReportState::PrepareEventFrames(
    const shared_ptr<NativeLeakInfo> &nlInfo, FaultStateType curState, bool isTelemetry)
{
    EventFrameInfo frameInfo;
    EventQuota eq = nlInfo->GetEventQuota();

    frameInfo.eventName = isTelemetry ? "MEMORY_LEAK" : nlInfo->GetReportEventName();
    frameInfo.secondFrame = curState == PROC_REPORT_OVERLIMIT_STATE ?
                            "OVER_LIMIT" : LEAK_GRADE_STRING[nlInfo->GetLeakGrade()];
    frameInfo.firstFrame = (nlInfo->GetLeakSource() == LeakSource::RSS) ? "RSS_MEMORY" : "PSS_MEMORY";
    frameInfo.thirdFrame = "0";

    if (nlInfo->GetLeakSource() == LeakSource::RSS) {
        if (eq.subLeakType != 0) {
            auto it = SubLeakTypeStr.find(eq.subLeakType);
            if (it != SubLeakTypeStr.end()) {
                frameInfo.thirdFrame = it->second;
            }
        }
    } else {
        DetailExtPssInfo pssInfo = nlInfo->GetExtPssInfo();
        string eventName = nlInfo->GetEventName();
        if (eventName == "ION_LEAK") {
            frameInfo.thirdFrame = "ION_LEAK";
        } else if (eventName == "GPU_LEAK") {
            frameInfo.thirdFrame = "GPU_LEAK";
        } else if (eventName == "AVC_MEMORY") {
            frameInfo.thirdFrame = "AVCODEC_LEAK";
        } else if (eventName == "MEDIA_MEMORY") {
            frameInfo.thirdFrame = "MEDIA_LEAK";
        } else if (eventName == "PSS_MEMORY") {
            frameInfo.thirdFrame = GetMaxPssType(pssInfo.detailSmapsInfo);
        }
    }

    return frameInfo;
}

string NativeLeakReportState::GenerateSummary(const shared_ptr<NativeLeakInfo> &nlInfo,
                                              const string &name, bool isTelemetry)
{
    string summaryInfo;
    if (isTelemetry) {
        summaryInfo = nlInfo->GetEventMsg().empty() ?
            (name + " leaked, threshold: " + to_string(nlInfo->GetMemoryLimit()) + "KB, top Memory: "+
            to_string(nlInfo->GetRealMemory().realMemory) + "KB") : nlInfo->GetEventMsg();
        if (nlInfo->ShouldBeKilled() == SHOULD_KILL) {
            summaryInfo += "\n has killed:" + name + (FaultDetectorManager::CheckIfAppForeGround(name) ?
                "(foreground)" : "(background)");
        }
    } else {
        summaryInfo = GenerateSummaryInfo(name, nlInfo);
    }
    return summaryInfo;
}

void NativeLeakReportState::WriteHiSysEvent(const EventFrameInfo &frameInfo, const shared_ptr<NativeLeakInfo> &nlInfo,
                                            const shared_ptr<FaultInfoBase> &faultInfo, bool isTelemetry)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    EventQuota eq = nlInfo->GetEventQuota();
    string summaryInfo = GenerateSummary(nlInfo, name, isTelemetry);
    ProcessSourceInfo processSourceInfo = nlInfo->GetProcessSourceInfo();
    string reason = isTelemetry ? processSourceInfo.GetLeakReason() : processSourceInfo.detectMsg;
    string faultType = isTelemetry ? to_string(nlInfo->GetTelemetryType()) : "";
    string fileName = isTelemetry ? nlInfo->GetProfilerName() : "";
    string bundleName = isTelemetry ? nlInfo->GetBundleName() : "";
    MetaInfoParams params = {
        .eventName = "memleak",
        .processName = name,
        .pid = pid,
        .firstFrame = frameInfo.firstFrame,
        .secondFrame = frameInfo.secondFrame,
        .thirdFrame = frameInfo.thirdFrame,
        .timeStamp = faultInfo->GetLeakedTime(),
        .basePath = MEMORY_LEAK_PATH,
        .versionName = faultInfo->GetHapVersion(),
        .versionCode = faultInfo->GetHapVersionCode(),
        .foreground = FaultDetectorManager::CheckIfAppForeGround(name) ? "1" : "0",
        .appRunningId = faultInfo->GetAppRunningId()
    };
    DfrUtil::GetInstance().CreateMetaInfoFile(params);

    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, frameInfo.eventName, HiSysEvent::EventType::FAULT,
        "PID", pid,
        "TELEMETRY_ID", eq.telemetryId,
        "FAULT", faultType,
        "FILENAME", fileName,
        "BUNDLE_NAME", bundleName,
        "MEMTOTAL", "0",
        "MODULE", name.empty() ? "/" : name,
        "VERSION", faultInfo->GetHapVersion().empty() ? "/" : faultInfo->GetHapVersion(),
        "HAPPEN_TIME", faultInfo->GetLeakedTime(),
        "FIRST_FRAME", frameInfo.firstFrame,
        "SECOND_FRAME", frameInfo.secondFrame,
        "THIRD_FRAME", frameInfo.thirdFrame,
        "FOREGROUND", (FaultDetectorManager::CheckIfAppForeGround(name) ? "YES" : "NO"),
        "LOG_PATH", nlInfo->GetSmapsPath(),
        "FOCUS_MODE", DfrUtil::GetInstance().GetFocusMode(),
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(name + faultInfo->GetHapVersion() +
            nlInfo->GetEventName() + LEAK_GRADE_STRING[nlInfo->GetLeakGrade()], 0),
        "APP_RUNNING_ID", faultInfo->GetAppRunningId(),
        "TELEMETRY_TASK", DfrUtil::GetInstance().GetLeakTypeName(eq.faultType, eq.subLeakType),
        "SUMMARY", summaryInfo,
        "REASON", reason);
}

void NativeLeakReportState::HandleTelemetry(const shared_ptr<NativeLeakInfo> &nlInfo, const EventQuota &eq)
{
    nlInfo->ConsumeQuota();
    nlInfo->SetEverSendTelemetryEvent();
    GetTelemetry().Report(HiSysEvent::EventType::STATISTIC, eq, NO_ERROR,
                          "TELEMETRY report, BundleQuota: " + to_string(eq.quota));
}

ErrCode NativeLeakReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    if (EndsWith(name, BROWSER_RENDER_SUFFIX)) {
        OHOS::system::SetParameter("web.debug.memleak.time",
            to_string(pid) + "," + DfrUtil::GetInstance().GetRealTime());
    }
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    ProcessSourceInfo processSourceInfo = nlInfo->GetProcessSourceInfo();
    string xperfNotifyReason;
    if (processSourceInfo.type == XPERF_EVENT_DETECT) {
        xperfNotifyReason = processSourceInfo.detectMsg;
    }

    bool isTelemetry = !nlInfo->IsEverSendTelemetryEvent() && nlInfo->GetTelemetryFlag();

    if (!nlInfo->ShouldReport()) {
        return ChangeNextState(faultInfo, faultObj);
    }

    HIVIEW_LOGI("StateProcess process: %{public}s pid: %{public}d, eventName: %{public}s, "
        "xperfNotifyReason: %{public}s", name.c_str(), pid, nlInfo->GetEventName().c_str(), xperfNotifyReason.c_str());

    FaultStateType curState = faultInfo->GetState();
    EventQuota eq = nlInfo->GetEventQuota();
    if (eq.subLeakType == SubLeakType::SUB_ARKTS_LEAK) {
        eq.telemetryId = "";
    }

    EventFrameInfo frameInfo = PrepareEventFrames(nlInfo, curState, isTelemetry);
    WriteHiSysEvent(frameInfo, nlInfo, faultInfo, isTelemetry);

    if (isTelemetry) {
        HandleTelemetry(nlInfo, eq);
    }

    nlInfo->RecordReport();
    DoReportExtPssEvent(faultInfo, frameInfo.secondFrame, eq.telemetryId);
    if (!xperfNotifyReason.empty()) {
        nlInfo->SetProcessSourceInfo({COMMON_DETECT, ""});
    }
    HandleAppLeakReport(faultInfo);

    return ChangeNextState(faultInfo, faultObj);
}

void NativeLeakReportState::DoReportExtPssEvent(shared_ptr<FaultInfoBase> &faultInfo, string secondFrame,
                                                string telemetryId)
{
    if (secondFrame == "OVER_LIMIT") {
        const string &name = faultInfo->GetProcessName();
        const pid_t pid = faultInfo->GetPid();
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
        auto pssInfo = nlInfo->GetExtPssInfo();
        HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK_EXT_PSS", HiSysEvent::EventType::FAULT,
            "PID", pid, "MEMTOTAL", "0",
            "MODULE", name.empty() ? "/" : name,
            "VERSION", faultInfo->GetHapVersion().empty() ? "/" : faultInfo->GetHapVersion(),
            "HAPPEN_TIME", faultInfo->GetLeakedTime(), "ADJ", DfrUtil::GetInstance().GetProcessAdj(pid),
            "APP_RUNNING_ID", faultInfo->GetAppRunningId(),
            // PSS (Smaps)
            "PSS_TOTAL_SIZE", pssInfo.detailSmapsInfo.totalSize,
            "PSS_NATIVE_HEAP", pssInfo.detailSmapsInfo.nativeHeap, "PSS_HAP", pssInfo.detailSmapsInfo.hap,
            "PSS_STACK", pssInfo.detailSmapsInfo.stack, "PSS_DB", pssInfo.detailSmapsInfo.db,
            "PSS_SO", pssInfo.detailSmapsInfo.so, "PSS_DEV", pssInfo.detailSmapsInfo.dev,
            "PSS_TTF", pssInfo.detailSmapsInfo.ttf, "PSS_ASHMEM", pssInfo.detailSmapsInfo.ashmem,
            "PSS_ARKTS_HEAP", pssInfo.detailSmapsInfo.arktsHeap,
            "PSS_ARKTS_STA_HEAP", pssInfo.detailSmapsInfo.arktsStaHeap,
            "PSS_KOTLIN_HEAP", pssInfo.detailSmapsInfo.kotlinHeap,
            "PSS_FLUTTER_DART_HEAP", pssInfo.detailSmapsInfo.dartHeap,
            "PSS_RN_HERMES_HEAP", pssInfo.detailSmapsInfo.rnHermesHeap,
            "PSS_JSVM_HEAP", pssInfo.detailSmapsInfo.jsvmHeap,
            "PSS_ARKWEB_V8", pssInfo.detailSmapsInfo.arkwebV8, "PSS_ARKWEB_PA", pssInfo.detailSmapsInfo.arkwebPa,
            "PSS_ANON_PAGE_OTHER", pssInfo.detailSmapsInfo.anonPageOther,
            "PSS_FILE_PAGE_OTHER", pssInfo.detailSmapsInfo.filePageOther,
            "PSS_OTHERS", pssInfo.detailSmapsInfo.others,
            // ION (DmaBuf)
            "ION_SIZE", pssInfo.detailDmaBufInfo.totalSize,
            "ION_PIXELMAP_SIZE", pssInfo.detailDmaBufInfo.pixelmapSize,
            "ION_XCOM_SIZE", pssInfo.detailDmaBufInfo.xcomSize, "ION_WEB_SIZE", pssInfo.detailDmaBufInfo.webSize,
            "ION_HW_VIDEO_SIZE", pssInfo.detailDmaBufInfo.hwVideoSize,
            "ION_SW_VIDEO_SIZE", pssInfo.detailDmaBufInfo.swVideoSize,
            "ION_EXTERNAL_SIZE", pssInfo.detailDmaBufInfo.externalSize,
            "ION_OTHERS_SIZE", pssInfo.detailDmaBufInfo.othersSize,
            // GPU
            "GPU_SIZE", pssInfo.detailGpuInfo.vulKanDefault + pssInfo.detailGpuInfo.vulKanImage +
                        pssInfo.detailGpuInfo.vulKanBuffer + pssInfo.detailGpuInfo.glesImage +
                        pssInfo.detailGpuInfo.glesBuffer + pssInfo.detailGpuInfo.clImage +
                        pssInfo.detailGpuInfo.clBuffer,
            "VULKAN_DEFAULT", pssInfo.detailGpuInfo.vulKanDefault, "VULKAN_IMAGE", pssInfo.detailGpuInfo.vulKanImage,
            "VULKAN_BUFFER", pssInfo.detailGpuInfo.vulKanBuffer, "GLES_IMAGE", pssInfo.detailGpuInfo.glesImage,
            "GLES_BUFFER", pssInfo.detailGpuInfo.glesBuffer, "CL_IMAGE", pssInfo.detailGpuInfo.clImage,
            "CL_BUFFER", pssInfo.detailGpuInfo.clBuffer,
            // Other
            "AVCODEC", nlInfo->GetAvcMem(), "MEDIA", nlInfo->GetMediaMem(),
            "RAM_SIZE", DfrUtil::GetInstance().GetRamSize());
    }
}

ErrCode NativeLeakReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    FaultStateType nextState = nlInfo->GetNextState();
    return faultObj.ExeNextStateProcess(faultInfo, nextState);
}

string NativeLeakReportState::GenerateSummaryInfo(const string &name, const shared_ptr<NativeLeakInfo> &nlInfo)
{
    string foregroundState = FaultDetectorManager::CheckIfAppForeGround(name) ? "(foreground)" : "(background)";
    SampleRecord lastSR = nlInfo->GetLastSampleRecord();
    string summaryInfo = nlInfo->GetEventMsg().empty() ?
            (name + foregroundState + " leaked: " + nlInfo->GetEventName() + ", threshold: " +
            to_string(nlInfo->GetMemoryLimit()) + "KB, top Memory: " +
            to_string(nlInfo->GetRealMemory().topMemory) + "KB, PSS: " + to_string(lastSR.pss) + "KB, GPU: " +
            to_string(lastSR.gpu) + "KB, ION: " + to_string(lastSR.ion) + "KB") :
            nlInfo->GetEventMsg();
    summaryInfo += nlInfo->ShouldBeKilled() == SHOULD_KILL ? "\n" + name + "should kill." : "";
    return summaryInfo;
}

string NativeLeakReportState::GetMaxPssType(const DetailSmapsInfo &detailSmapsInfo)
{
    uint64_t maxSize = 0;
    string maxType = "OTHER_LEAK";

    auto checkAndUpdateMax = [&maxSize, &maxType](uint64_t size, const string& type) {
        if (size > maxSize) {
            maxSize = size;
            maxType = type;
        }
    };

    checkAndUpdateMax(detailSmapsInfo.nativeHeap, "NATIVE_HEAP_LEAK");
    checkAndUpdateMax(detailSmapsInfo.hap, "HAP_LEAK");
    checkAndUpdateMax(detailSmapsInfo.stack, "STACK_LEAK");
    checkAndUpdateMax(detailSmapsInfo.db, "DB_LEAK");
    checkAndUpdateMax(detailSmapsInfo.so, "SO_LEAK");
    checkAndUpdateMax(detailSmapsInfo.dev, "DEV_LEAK");
    checkAndUpdateMax(detailSmapsInfo.ttf, "TTF_LEAK");
    checkAndUpdateMax(detailSmapsInfo.ashmem, "ASHMEM_LEAK");
    checkAndUpdateMax(detailSmapsInfo.arktsHeap, "ARKTS_HEAP_LEAK");
    checkAndUpdateMax(detailSmapsInfo.arktsStaHeap, "ARKTS_STA_HEAP_LEAK");
    checkAndUpdateMax(detailSmapsInfo.kotlinHeap, "KOTLIN_HEAP_LEAK");
    checkAndUpdateMax(detailSmapsInfo.dartHeap, "FLUTTER_DART_HEAP_LEAK");
    checkAndUpdateMax(detailSmapsInfo.rnHermesHeap, "RN_HERMES_HEAP_LEAK");
    checkAndUpdateMax(detailSmapsInfo.jsvmHeap, "JSVM_HEAP_LEAK");
    checkAndUpdateMax(detailSmapsInfo.arkwebV8, "ARKWEB_V8_LEAK");
    checkAndUpdateMax(detailSmapsInfo.arkwebPa, "ARKWEB_PA_LEAK");

    return maxType;
}
} // namespace HiviewDFX
} // namespace OHOS

