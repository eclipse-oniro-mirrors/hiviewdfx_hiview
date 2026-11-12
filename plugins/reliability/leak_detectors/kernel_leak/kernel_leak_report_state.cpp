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

#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include "kernel_leak_info.h"
#include "kernel_leak_dump_state.h"
#include "kernel_leak_sample_state.h"
#include "kernel_leak_judge_state.h"
#include "kernel_leak_report_state.h"
#include "kernel_leak_removal_state.h"
#include "detail_info_parser.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "fault_detector_manager.h"
#include "dfr_util.h"

using namespace std;

namespace {
const map<string, string> ReportLeakTypeList = {
    {"ashmem", "ASHMEM_LEAK"},
    {"ion", "ION_LEAK"},
    {"gpu", "GPU_LEAK"},
    {"fd_rs", "FD_RS_LEAK"},
    {"gpu_rs", "GPU_RS_LEAK"},
    {"vma_leak", "VMA_LEAK"},
    {"kernel_zone", "KERNEL_ZONE_LOW"},
    {"dmabuf_quick_kill", "DMABUF_QUICK_KILL"}
};

const std::unordered_map<std::string, OHOS::AppExecFwk::LeakType> ProcessLeakConfig = {
    {"ashmem", {OHOS::AppExecFwk::LeakType::ASHMEM_MEMORY}},
    {"ion",    {OHOS::AppExecFwk::LeakType::ION_MEMORY}},
    {"gpu",    {OHOS::AppExecFwk::LeakType::GPU_MEMORY}}
};

}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "KernelLeakReportState");

std::string KernelLeakReportState::GetSecondFrame(const shared_ptr<KernelLeakInfo> &kernelInfo, FaultStateType curState)
{
    string secondFrame = curState == PROC_REPORT_OVERLIMIT_STATE ? "OVER_LIMIT" :
                         LEAK_GRADE_STRING[kernelInfo->GetLeakGrade()];
    if (kernelInfo->GetProcessName() == NAME_GPU_RS_LEAK ||
        kernelInfo->GetProcessName() == NAME_FD_RS_LEAK) {
        secondFrame = "OVER_LIMIT";
    }
    return secondFrame;
}

std::string KernelLeakReportState::GetInfoForReport(const shared_ptr<KernelLeakInfo> &kernelInfo)
{
    string infoForReport = kernelInfo->GetProcessInfoReport();
    if (kernelInfo->ShouldBeKilled() == SHOULD_KILL) {
        infoForReport += "\n has killed:" + kernelInfo->GetProcessNameForKill() +
            (FaultDetectorManager::CheckIfAppForeGround(kernelInfo->GetProcessNameForKill()) ?
            "(foreground)" : "(background)");
    }
    return infoForReport;
}

void KernelLeakReportState::HandleAppLeakReport(shared_ptr<FaultInfoBase> &faultInfo)
{
    if (faultInfo == nullptr) {
        HIVIEW_LOGE("HandleAppLeakReport kernelInfo is null");
        return;
    }
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    std::string processName = kernelInfo->GetProcessName();

    auto config = ProcessLeakConfig.find(processName);
    if (config == ProcessLeakConfig.end()) {
        HIVIEW_LOGE("no config for process: %{public}s", processName.c_str());
        return;
    }

    AppExecFwk::LeakType leakType = config->second;
    int32_t pid = kernelInfo->GetProcessPidForReport();
    unsigned long size = static_cast<unsigned long>(kernelInfo->GetProcessSizeForReport() / BYTE_PER_KB);

    DfrUtil::GetInstance().NotifyWarningToApp(pid, leakType, size);
}

void KernelLeakReportState::CreateMetaInfoFile(const shared_ptr<KernelLeakInfo> &kernelInfo,
                                               const shared_ptr<FaultInfoBase> &faultInfo)
{
    FaultStateType curState = faultInfo->GetState();
    string secondFrame = GetSecondFrame(kernelInfo, curState);
    string firstFrame = ReportLeakTypeList.count(kernelInfo->GetProcessName()) ?
        ReportLeakTypeList.at(kernelInfo->GetProcessName()) : kernelInfo->GetFirstFrame();

    MetaInfoParams params = {
        .eventName = "memleak",
        .processName = kernelInfo->GetProcessNameForReport(),
        .pid = kernelInfo->GetProcessPidForReport(),
        .firstFrame = firstFrame,
        .secondFrame = secondFrame,
        .thirdFrame = "0",
        .timeStamp = faultInfo->GetLeakedTime(),
        .basePath = MEMORY_LEAK_PATH,
        .versionName = kernelInfo->GetVersionForReport(),
        .versionCode = kernelInfo->GetHapVersionCode(),
        .foreground = FaultDetectorManager::CheckIfAppForeGround(kernelInfo->GetProcessNameForReport()) ? "1" : "0",
        .appRunningId = kernelInfo->GetAppRunningId()
    };
    DfrUtil::GetInstance().CreateMetaInfoFile(params);
}

ErrCode KernelLeakReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    HIVIEW_LOGD("begin KernelLeakDetector SendBoardCast");
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    if (kernelInfo == nullptr) {
        HIVIEW_LOGE("StateProcess kernelInfo is null");
        return NULL_POINTER;
    }
    FaultStateType curState = faultInfo->GetState();
    string secondFrame = GetSecondFrame(kernelInfo, curState);
    string infoForReport = GetInfoForReport(kernelInfo);

    if (secondFrame == "OVER_LIMIT") {
        HIVIEW_LOGD("when is OVER_LIMIT, report sub event");
        ReportSubEvent(faultInfo);
    }
    kernelInfo->DecideReportType();
    if (!kernelInfo->ShouldReport()) {
        return ChangeNextState(faultInfo, detectorObj);
    }
    EventQuota eq = kernelInfo->GetEventQuota();

    string eventName = kernelInfo->GetReportEventName();
    string firstFrame = ReportLeakTypeList.count(kernelInfo->GetProcessName()) ?
        ReportLeakTypeList.at(kernelInfo->GetProcessName()) : kernelInfo->GetFirstFrame();

    CreateMetaInfoFile(kernelInfo, faultInfo);

    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, eventName, HiSysEvent::EventType::FAULT,
        "PID", kernelInfo->GetProcessPidForReport(),
        "TELEMETRY_ID", eq.telemetryId,
        "MEMTOTAL", "0",
        "MODULE", kernelInfo->GetProcessNameForReport(),
        "VERSION", kernelInfo->GetVersionForReport().empty() ? "/" : kernelInfo->GetVersionForReport(),
        "HAPPEN_TIME", faultInfo->GetLeakedTime(),
        "FIRST_FRAME", firstFrame,
        "SECOND_FRAME", secondFrame,
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(
            kernelInfo->GetProcessNameForReport() + kernelInfo->GetVersionForReport() +
            LEAK_GRADE_STRING[kernelInfo->GetLeakGrade()] + kernelInfo->GetProcessName(), 0),
        "FOREGROUND", (FaultDetectorManager::CheckIfAppForeGround(kernelInfo->GetProcessNameForReport()) ?
            "YES" : "NO"),
        "LOG_PATH", kernelInfo->GetLogPath(),
        "FOCUS_MODE", DfrUtil::GetInstance().GetFocusMode(),
        "APP_RUNNING_ID", kernelInfo->GetAppRunningId(),
        "TELEMETRY_TASK", DfrUtil::GetInstance().GetLeakTypeName(eq.faultType, eq.subLeakType),
        "SUMMARY", kernelInfo->GetEventMsg().empty() ? (faultInfo->GetProcessName() + " leaked, " +
            "threashold: " + to_string(kernelInfo->GetMemoryLimit() / KB_PER_MB) + "MB, " +
            "top Memory: "+ to_string(kernelInfo->GetTopMemory() / KB_PER_MB) + "MB\n" +
            infoForReport) : kernelInfo->GetEventMsg() + infoForReport
    );
    kernelInfo->RecordReport();
    HandleAppLeakReport(faultInfo);
    return ChangeNextState(faultInfo, detectorObj);
}

ErrCode KernelLeakReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    FaultStateType nextState = kernelInfo->GetNextState();
    return detectorObj.ExeNextStateProcess(faultInfo, nextState);
}

void KernelLeakReportState::ReportSubEvent(std::shared_ptr<FaultInfoBase> &faultInfo)
{
    if (faultInfo->GetProcessName() == "ion") {
        ReportIonSubEvent(faultInfo);
    } else if (faultInfo->GetProcessName() == "gpu") {
        ReportGpuSubEvent(faultInfo);
    } else if (faultInfo->GetProcessName() == "gpu_rs") {
        ReportGpuRsSubEvent(faultInfo);
    }
}

void KernelLeakReportState::ReportIonSubEvent(std::shared_ptr<FaultInfoBase> &faultInfo)
{
    HIVIEW_LOGD("report sysevent IonSubEvent");
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    if (!kernelInfo->GetIsReportInfoSaveSuccess()) {
        HIVIEW_LOGE("can't get reportInfo before kill.");
        return;
    }
    DetailDmaBufInfo detailDmaBufInfo = kernelInfo->GetDmabufReportInfo();
    int adjTemp = kernelInfo->GetAdjForReport();
    int32_t ret = HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK_ION", HiSysEvent::EventType::FAULT,
        "APP_RUNNING_ID", kernelInfo->GetAppRunningId(),
        "MODULE", kernelInfo->GetProcessNameForReport(),
        "ADJ", adjTemp,
        "DEVICE_RAM_SIZE", DfrUtil::GetInstance().GetRamSize(),
        "TOTAL_SIZE", detailDmaBufInfo.totalSize,
        "PIXELMAP_SIZE", detailDmaBufInfo.pixelmapSize,
        "XCOM_SIZE", detailDmaBufInfo.xcomSize,
        "WEB_SIZE", detailDmaBufInfo.webSize,
        "HW_VIDEO_SIZE", detailDmaBufInfo.hwVideoSize,
        "SW_VIDEO_SIZE", detailDmaBufInfo.swVideoSize,
        "EXTERNAL_SIZE", detailDmaBufInfo.externalSize,
        "OTHERS_SIZE", detailDmaBufInfo.othersSize);
    if (ret < 0) {
        HIVIEW_LOGW("HiSysEventWrite MEMORY_LEAK_ION failed");
    }
}

void KernelLeakReportState::ReportGpuSubEvent(std::shared_ptr<FaultInfoBase> &faultInfo)
{
    HIVIEW_LOGD("report sysevent GPUSubEvent");
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    if (!kernelInfo->GetIsReportInfoSaveSuccess()) {
        HIVIEW_LOGE("can't get reportInfo before kill.");
        return;
    }
    DetailGpuInfo detailGpuInfo = kernelInfo->GetGpuReportInfo();
    int adjTemp = kernelInfo->GetAdjForReport();
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK_GPU", HiSysEvent::EventType::FAULT,
        "APP_RUNNING_ID", kernelInfo->GetAppRunningId(),
        "MODULE", kernelInfo->GetProcessNameForReport(),
        "ADJ", adjTemp,
        "DEVICE_RAM_SIZE", DfrUtil::GetInstance().GetRamSize(),
        "VULKAN_DEFAULT", detailGpuInfo.vulKanDefault,
        "VULKAN_IMAGE", detailGpuInfo.vulKanImage,
        "VULKAN_BUFFER", detailGpuInfo.vulKanBuffer,
        "GLES_IMAGE", detailGpuInfo.glesImage,
        "GLES_BUFFER", detailGpuInfo.glesBuffer,
        "CL_IMAGE", detailGpuInfo.clImage,
        "CL_BUFFER", detailGpuInfo.clBuffer);
}

void KernelLeakReportState::ReportGpuRsSubEvent(std::shared_ptr<FaultInfoBase> &faultInfo)
{
    HIVIEW_LOGD("report sysevent GPURSSubEvent");
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    if (kernelInfo == nullptr) {
        HIVIEW_LOGE("ReportGpuRsSubEvent kernelInfo is null");
        return;
    }
    int adjTemp = kernelInfo->GetAdjForReport();
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK_GPU_RS", HiSysEvent::EventType::FAULT,
        "APP_RUNNING_ID", kernelInfo->GetAppRunningId(),
        "MODULE", kernelInfo->GetProcessNameForReport(),
        "ADJ", adjTemp,
        "DEVICE_RAM_SIZE", DfrUtil::GetInstance().GetRamSize(),
        "RENDER_NODE", kernelInfo->GetRenderNode(),
        "MAX_IMAGE_SIZE", kernelInfo->GetMaxImageSize(),
        "MAX_IMAGE_NAME", kernelInfo->GetMaxImageName(),
        "MAX_IMAGE_RESOLUTION", kernelInfo->GetMaxImageResolution());
}
} // namespace HiviewDFX
} // namespace OHOS
