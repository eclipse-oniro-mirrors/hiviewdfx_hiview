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

#include "native_leak_dump_state.h"

#include <fstream>
#include <filesystem>
#include <memory>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <parameters.h>
#include <memory_trace.h>
#include "ashmem.h"
#include "native_leak_info.h"
#include "old_log_cleaner.h"
#include "hiview_logger.h"
#include "telemetry_manager.h"
#include "dfr_util.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakDumpState");

namespace {
constexpr int WEIGHT_16 = 16;
constexpr int REPEAT_REPORT_INTERVAL = SEC_PER_DAY;
constexpr bool IS_FILE_OPEN_APPEND  = true;

const std::map<SubLeakType, ProfilerType> SUBLEAK_TO_PROFILER_MAP = {
    {SubLeakType::SUB_ASHMEM_LEAK, ProfilerType::ASHMEM},
    {SubLeakType::SUB_GPU_LEAK, ProfilerType::GPU},
    {SubLeakType::SUB_ION_LEAK, ProfilerType::ION}
};

enum {
    NO_PROFILER = 0,
    GENERAL_STATISTICS = 1 << 0,
    ACCURATE_STATISTICS = 1 << 1,
    SIMPLIFY_PROFILER = 1 << 3,
    GPU_PROFILER = 1 << 4,
    ARKTS_RAWHEAP = 1 << 5,
    JSVM_RAWHEAP = 1 << 6,
    ARKWEB_V8_RAWHEAP = 1 << 7,
    PSSSUB_PROFILER = 1 << 8,
    KOTLIN_RAWHEAP = 1 << 9,
    ARKTS_STA_RAWHEAP = 1 << 10,
};

const char* const DUMP_LOG_TYPE_STRING[] {
    "NO_LOG",
    "BASIC_LOG",
    "BASIC_AND_EXTERNAL_LOG",
};
}

NativeLeakDumpState::NativeLeakDumpState()
    : simplifiedNmdInterval_(DfrUtil::GetInstance().IsMemTestEnable() ?
          PROFILER_CAT_NMD_INTERVAL_TEST : NativeLeakConfig::GetInstance().GetProfilerCatNmdInterval()),
      sendStopCommandTime_(DfrUtil::GetInstance().IsMemTestEnable() ?
          PROFILER_TEST_TIME : NativeLeakConfig::GetInstance().GetProfilerTime()),
      profilerTimeout_(sendStopCommandTime_ + PROFILER_STOP_DELAY),
      asyncStackEnable_(NativeLeakConfig::GetInstance().GetAsyncStackFlag())
{
}

ErrCode NativeLeakDumpState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    const string &name = faultInfo->GetProcessName();

    if (nlInfo->IsDied()) {
        HIVIEW_LOGI("process %{public}s has died, stop get native log.", name.c_str());
        return SUCCESSED;
    }
    if (nlInfo->GetAllDumpLogType() == NO_LOG) {
        HIVIEW_LOGD("process: %{public}s, leakGrade is too small, not dump and report", name.c_str());
        nlInfo->RecordReport();
        return SUCCESSED;
    }
    GetMemoryLeakLog(nlInfo, GENERAL_STATISTICS);
    DoDumpLog(nlInfo->GetDumperInfo());
    if (nlInfo->GetDumpLogType() == BASIC_LOG || DfrUtil::GetInstance().IsDeviceLowmem()) {
        GetMemoryLeakLog(nlInfo, ACCURATE_STATISTICS);
        return ChangeNextState(faultInfo, faultObj);
    }
    if (nlInfo->GetDumpLogType() == BASIC_AND_EXTERNAL_LOG) {
        if (NativeLeakConfig::GetInstance().GetGwpAsanEnabled()) {
            DumpGwpAsanInfo(nlInfo);
        }
        string systemGraphicGpuApiType = system::GetParameter("persist.sys.graphic.GpuApitype", "");
        if (name == RENDER_SERVICE_PROCESS && systemGraphicGpuApiType != "2") { // 2:ddgr, 1:vulkan
            return ChangeNextState(faultInfo, faultObj);
        }
        GetProfilerAndNmd(nlInfo);
    }
    return ChangeNextState(faultInfo, faultObj);
}

void NativeLeakDumpState::GetMemoryLeakLog(shared_ptr<NativeLeakInfo> &nlInfo, uint32_t flag)
{
    if (flag & GENERAL_STATISTICS) {
        DumpGeneralInfo(nlInfo);
        nlInfo->SetIsAppendSmapsFile(true);
    }
    if (flag & ACCURATE_STATISTICS) {
        DumpDetailInfo(nlInfo);
    }
    if (DfrUtil::GetInstance().IsDeviceLowmem()) {
        // do not get log when low memory
        HIVIEW_LOGI("current device is LOW MEM!");
        return;
    }
    if (flag & SIMPLIFY_PROFILER) {
        SetTelemetryParam(nlInfo);
        DumpMallocInfo(nlInfo);
    }
    if (flag & GPU_PROFILER) {
        DumpGpuInfo(nlInfo);
    }
    if (flag & ARKTS_RAWHEAP) {
        nlInfo->DumpArkTsRawHeapInfo("EXT_PSS_SYSTEM_TELEMETRY");
    }
    if (flag & JSVM_RAWHEAP) {
        nlInfo->DumpRawHeapMemoryForThirdParty("EXT_PSS_SYSTEM_TELEMETRY", AppExecFwk::MemDumpType::JSVM);
    }
    if (flag & ARKWEB_V8_RAWHEAP) {
        nlInfo->DumpRawHeapMemoryForThirdParty("EXT_PSS_SYSTEM_TELEMETRY", AppExecFwk::MemDumpType::ARKWEB_JS);
    }
    if (flag & PSSSUB_PROFILER) {
        DumpTelemetrySubLeakInfo(nlInfo);
    }
    if (flag & KOTLIN_RAWHEAP) {
        nlInfo->DumpRawHeapMemoryForThirdParty("EXT_PSS_SYSTEM_TELEMETRY", AppExecFwk::MemDumpType::KMP_KOTLIN);
    }
    if (flag & ARKTS_STA_RAWHEAP) {
        nlInfo->DumpArkTsRawHeapInfo("EXT_PSS_SYSTEM_TELEMETRY");
    }
}

void NativeLeakDumpState::SetTelemetryParam(shared_ptr<NativeLeakInfo> &nlInfo)
{
    EventQuota eventQuota = nlInfo->GetEventQuota();
    if (eventQuota.ts3 != 0) {
        ts3_ = eventQuota.ts3;
    }
    if (eventQuota.nmdInterval != 0) {
        simplifiedNmdInterval_ = eventQuota.nmdInterval;
    }
    if (eventQuota.profilerTime != 0) {
        sendStopCommandTime_ = eventQuota.profilerTime;
        profilerTimeout_ = sendStopCommandTime_ + PROFILER_STOP_DELAY;
    }
    if (eventQuota.ts1 != 0) {
        ts1_ = eventQuota.ts1;
    }
    if (eventQuota.ts2 != 0) {
        ts2_ = eventQuota.ts2;
    }
    if (!eventQuota.targetSoNames.empty()) {
        targetSoNames_ = eventQuota.targetSoNames;
    }
}

void NativeLeakDumpState::DumpGeneralInfo(shared_ptr<NativeLeakInfo> &nlInfo) const
{
    if (nlInfo == nullptr) {
        HIVIEW_LOGE("WRONG POINTER");
        return;
    }
    ofstream fout;
    OldLogCleaner::GetInstance().GetLogFile(SMAPS, nlInfo.get(), fout, nlInfo->GetIsAppendSmapsFile());
    if (!fout.is_open()) {
        HIVIEW_LOGE("FAILED TO OPEN FILE:%{public}s", nlInfo->GetSmapsPath().c_str());
        return;
    }
    fout << endl;
    fout << "Generated by HiviewDFX @OpenHarmony" << endl;
    fout << "LOGGER_MEMCHECK_GENERAL_INFO" << endl;
    fout << "\tpidNumber: " << nlInfo->GetPid() << endl;
    fout << "\tprocessName: " << nlInfo->GetProcessName() << endl;
    fout << "\tappRunningId: " << nlInfo->GetAppRunningId() << endl;
    fout << "\tPidStartTime: " << nlInfo->GetPidStartTime() << endl;
    fout << "\tRealPssMemory: " << nlInfo->GetLastSampleRecord().totalMem << endl;
    fout << "\tDeviceRamSize: " << DfrUtil::GetInstance().GetRamSize() << endl;
    fout << endl;

    DumpProcInfo(MEM_INFO_PATH, fout, LOGGER_MEMCHECK_MEMINFO);
    DfrUtil::GetInstance().DumpAppPageInfo(nlInfo->GetPid(), nlInfo->GetProcessName(), fout);
    HIVIEW_LOGI("WRITE GENERAL STATISITICS FINSISHED process %{public}s", nlInfo->GetProcessName().c_str());
    fout.close();
}

void NativeLeakDumpState::DumpDetailInfo(shared_ptr<NativeLeakInfo> &nlInfo)
{
    int fd = -1;
    OldLogCleaner::GetInstance().GetLogFile(SMAPS, nlInfo.get(), fd);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", nlInfo->GetSmapsPath().c_str(),
            strerror(errno));
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    string detailHead = string("\n*****************************\n") +
        string("LOGGER_MEMCHECK_DETIAL_INFO\n");

    bool isDied = nlInfo->IsDied();
    if (isDied) {
        string curProcessName = DfrUtil::GetInstance().GetProcessName(nlInfo->GetPid());
        detailHead += "detector pid " + to_string(nlInfo->GetPid()) + "may be died, raw process name is "
            + nlInfo->GetProcessName() + ", but now is " + curProcessName + "\n";
    }
    size_t detailHeadSize = detailHead.size();
    lseek(fd, 0, SEEK_END);
    ssize_t detailWrite = write(fd, detailHead.c_str(), detailHeadSize);
    if (isDied) {
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return;
    }
    if (detailWrite < 0) {
        HIVIEW_LOGE("write detail head failed, errno: %{public}s", strerror(errno));
    }

    MemProfilerParam profilerParam = {
        .duration = 0,
        .resType = NO_RES_TYPE,
        .profilerType = DETAIL_NMD,
        .faultInfo = static_pointer_cast<FaultInfoBase>(nlInfo),
        .waitProfilerTime = PROFILER_NMD_WAIT_TIME,
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = {},
        .saConfig = {},
        .gwpAsanConfig = {}
    };

    DfrProfilerManager::GetInstance().DoProfiler(profilerParam);
    fdsan_close_with_tag(fd, FDSAN_TAG);
}

uint64_t NativeLeakDumpState::GetResTypeByProfilerType(ProfilerType profilerType)
{
    switch (profilerType) {
        case ProfilerType::ASHMEM:
            return RES_FD_ASHMEM | RES_ASHMEM_MASK;
        case ProfilerType::ION:
            return RES_FD_DMABUF | RES_DMABUF_MASK;
        default:
            return NO_RES_TYPE;
    }
}

int NativeLeakDumpState::GetActuralProfilerTimeout(shared_ptr<NativeLeakInfo> &nlInfo)
{
    if (nlInfo->GetProcessSourceInfo().type != XPERF_EVENT_DETECT) {
        return profilerTimeout_;
    }
    const ConfigValueStru configValueStru = NativeLeakConfig::GetInstance().GetCommonKeyValue("xperf_profiler_time");
    return configValueStru.intValue + PROFILER_STOP_DELAY;
}

int NativeLeakDumpState::GetActuralStopCommandTime(shared_ptr<NativeLeakInfo> &nlInfo)
{
    if (nlInfo->GetProcessSourceInfo().type != XPERF_EVENT_DETECT) {
        return sendStopCommandTime_;
    }
    const ConfigValueStru configValueStru = NativeLeakConfig::GetInstance().GetCommonKeyValue("xperf_profiler_time");
    return configValueStru.intValue;
}

void NativeLeakDumpState::GetProfilerAndNmd(shared_ptr<NativeLeakInfo> &nlInfo)
{
    uint32_t profilerType = NO_PROFILER;
    bool isInSubLeakMap = SUBLEAK_TO_PROFILER_MAP.find(nlInfo->GetTelemetrySubLeakType())
                            != SUBLEAK_TO_PROFILER_MAP.end();
    bool isEventNameMatch = (nlInfo->GetEventName() == GPU_LEAK_STR ||
                            nlInfo->GetEventName() == ION_LEAK_STR ||
                            nlInfo->GetEventName() == PSS_MEMORY_STR);
    if (isEventNameMatch && isInSubLeakMap) {
        profilerType = PSSSUB_PROFILER;
    } else if (nlInfo->GetEventName() == PSS_MEMORY_STR) {
        profilerType = SIMPLIFY_PROFILER;
        if (nlInfo->GetTelemetrySubLeakType() == SubLeakType::SUB_ARKTS_LEAK) {
            profilerType = ARKTS_RAWHEAP;
        }
        if (nlInfo->GetTelemetrySubLeakType() == SubLeakType::SUB_JSVM_LEAK) {
            profilerType = JSVM_RAWHEAP;
        }
        if (nlInfo->GetTelemetrySubLeakType() == SubLeakType::SUB_ARKWEB_V8_LEAK) {
            profilerType = ARKWEB_V8_RAWHEAP;
        }
        if (nlInfo->GetTelemetrySubLeakType() == SubLeakType::SUB_KOTLIN_LEAK) {
            profilerType = KOTLIN_RAWHEAP;
        }
        if (nlInfo->GetTelemetrySubLeakType() == SubLeakType::SUB_ARKTS_STA_LEAK) {
            profilerType = ARKTS_STA_RAWHEAP;
        }
    } else if (nlInfo->GetEventName() == GPU_LEAK_STR) {
        profilerType = GPU_PROFILER;
    } else if (nlInfo->GetEventName() == RSS_LEAK_STR) {
        if (nlInfo->GetTelemetrySubLeakType() == SubLeakType::SUB_NATIVE_HEAP_LEAK) {
            profilerType = SIMPLIFY_PROFILER;
        } else {
            profilerType = PSSSUB_PROFILER;
        }
    }
    // must dump nmd before dump stack
    GetMemoryLeakLog(nlInfo, profilerType);
}

ErrCode NativeLeakDumpState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    const LeakGrade grade = faultInfo->GetLeakGrade();
    if (grade <= NO_LEAK) {
        HIVIEW_LOGE("leakgrade is too low to report");
        return FAILURE;
    }
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    if (nlInfo->GetProcessSourceInfo().type == SYSLOAD_DETECT) {
        // terminate the process immediately after the dump is completed
        return FAILURE;
    }
    FaultStateType nextStatType = nlInfo->GetNextState();
    return faultObj.ExeNextStateProcess(faultInfo, nextStatType);
}

void NativeLeakDumpState::DumpGpuInfo(shared_ptr<NativeLeakInfo> &nlInfo)
{
    if (nlInfo == nullptr) {
        HIVIEW_LOGE("nlInfo is nullptr.");
        return;
    }
    if (nlInfo->IsDied()) {
        HIVIEW_LOGE("process %{public}s is died, stop get profiler", nlInfo->GetProcessName().c_str());
        return;
    }
    int fd = -1;
    OldLogCleaner::GetInstance().GetLogFile(NATIVE_PROFILER, nlInfo.get(), fd);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", nlInfo->GetProfilerPath().c_str(), strerror(errno));
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    MemSaConfig saConfig;

    MemProfilerParam profilerParam = {
        .duration = GetActuralStopCommandTime(nlInfo),
        .resType = NO_RES_TYPE,
        .profilerType = GPU,
        .faultInfo = static_pointer_cast<FaultInfoBase>(nlInfo),
        .waitProfilerTime = GetActuralProfilerTimeout(nlInfo),
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = {},
        .saConfig = saConfig,
        .gwpAsanConfig = {}
    };

    bool ret = DfrProfilerManager::GetInstance().DoProfiler(profilerParam);
    if (!ret) {
        HIVIEW_LOGW("DoProfiler failed for process %{public}s", nlInfo->GetProcessName().c_str());
    }
    fdsan_close_with_tag(fd, FDSAN_TAG);
}

void NativeLeakDumpState::DumpTelemetrySubLeakInfo(std::shared_ptr<NativeLeakInfo> &nlInfo)
{
    if (nlInfo == nullptr) {
        HIVIEW_LOGE("nlInfo is nullptr.");
        return;
    }
    if (nlInfo->IsDied()) {
        HIVIEW_LOGE("process %{public}s is died, stop get profiler", nlInfo->GetProcessName().c_str());
        return;
    }
    int fd = -1;
    OldLogCleaner::GetInstance().GetLogFile(NATIVE_PROFILER, nlInfo.get(), fd);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", nlInfo->GetProfilerPath().c_str(), strerror(errno));
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    MemSaConfig saConfig;

    auto it = SUBLEAK_TO_PROFILER_MAP.find(nlInfo->GetTelemetrySubLeakType());
    if (it == SUBLEAK_TO_PROFILER_MAP.end()) {
        HIVIEW_LOGE("unknown sub leak type for telemetry: %{public}d", nlInfo->GetTelemetrySubLeakType());
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return;
    }
    ProfilerType profilerType = it->second;

    MemProfilerParam profilerParam = {
        .duration = GetActuralStopCommandTime(nlInfo),
        .resType = GetResTypeByProfilerType(profilerType),
        .profilerType = profilerType,
        .faultInfo = static_pointer_cast<FaultInfoBase>(nlInfo),
        .waitProfilerTime = GetActuralProfilerTimeout(nlInfo),
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = {},
        .saConfig = saConfig,
        .gwpAsanConfig = {}
    };

    bool ret = DfrProfilerManager::GetInstance().DoProfiler(profilerParam);
    if (!ret) {
        HIVIEW_LOGE("DoProfiler failed for process %{public}s", nlInfo->GetProcessName().c_str());
    }
    fdsan_close_with_tag(fd, FDSAN_TAG);
}

void NativeLeakDumpState::DumpMallocInfo(shared_ptr<NativeLeakInfo> &nlInfo)
{
    ofstream fout;
    OldLogCleaner::GetInstance().GetLogFile(SMAPS, nlInfo.get(), fout, IS_FILE_OPEN_APPEND);
    if (!fout.is_open()) {
        HIVIEW_LOGE("failed to open smaps file: %{public}s, err: %{public}s", nlInfo->GetSmapsPath().c_str(),
                    strerror(errno));
        return;
    }
    fout.close();
    SimplifiedMemConfig memConfig;
    ConfigureSimplifiedMemConfig(memConfig);
    if (!StartDumpSimplifiedProfiler(nlInfo, memConfig)) {
        HIVIEW_LOGE("failed to dump simplified info. current process: %{public}s", nlInfo->GetProcessName().c_str());
    }
}

void NativeLeakDumpState::DumpProcInfo(const string &procPath, ofstream &fout, const string &title) const
{
    string procInfo = DfrUtil::GetInstance().GetCacheProcInfo(procPath);
    fout << endl;
    fout << "******************************" << endl;
    fout << title << endl;
    fout << procInfo << endl;
    fout << "************ endl ************" << endl;
    fout << endl;
}

bool NativeLeakDumpState::StartDumpSimplifiedProfiler(shared_ptr<NativeLeakInfo> &nlInfo,
                                                      SimplifiedMemConfig &nativeDaemonConfig)
{
    if (nlInfo == nullptr) {
        HIVIEW_LOGE("nlInfo is nullptr");
        return false;
    }
    if (nlInfo->IsDied()) {
        HIVIEW_LOGE("process %{public}s is died, stop get profiler", nlInfo->GetProcessName().c_str());
        return false;
    }
    int duration = GetActuralStopCommandTime(nlInfo);
    int secondNmdDelay = DfrUtil::GetInstance().IsMemTestEnable() ?
        SECOND_NMD_DELAY_TEST : NativeLeakConfig::GetInstance().GetSecondNmdDelay();
    if (secondNmdDelay <= 0 || duration <= 0 || secondNmdDelay > duration) {
        HIVIEW_LOGE("invalid delay params, secondNmdDelay=%{public}d, duration=%{public}d", secondNmdDelay, duration);
        return false;
    }

    int fd = -1;
    OldLogCleaner::GetInstance().GetLogFile(NATIVE_PROFILER, nlInfo.get(), fd);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", nlInfo->GetProfilerPath().c_str(), strerror(errno));
        return false;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);

    MemProfilerParam profilerParam = {
        .duration = duration,
        .resType = NO_RES_TYPE,
        .profilerType = SIMPLIFIED_PROFILER,
        .faultInfo = static_pointer_cast<FaultInfoBase>(nlInfo),
        .waitProfilerTime = GetActuralProfilerTimeout(nlInfo),
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = nativeDaemonConfig,
        .saConfig = {},
        .gwpAsanConfig = {},
        .smapsPath = nlInfo->GetSmapsPath(),
        .secondNmdDelay = secondNmdDelay,
    };

    bool ret = DfrProfilerManager::GetInstance().DoProfiler(profilerParam);
    if (!ret) {
        HIVIEW_LOGE("dump process %{public}s failed", nlInfo->GetProcessName().c_str());
        nlInfo->SetIsDumpHiprofilerDrop(true);
    }
    fdsan_close_with_tag(fd, FDSAN_TAG);
    return ret;
}

void NativeLeakDumpState::ConfigureSimplifiedMemConfig(SimplifiedMemConfig &memConfig)
{
    // Adapt dump profile: set sampleSize = 0 when ts1_ or ts2_ exists valid value
    if (ts1_ > 0 || ts2_ > 0) {
        memConfig.sampleSize = 0;
    } else {
        memConfig.sampleSize = ts3_ > 0 ? static_cast<size_t>(ts3_) :
            static_cast<size_t>(NativeLeakConfig::GetInstance().GetMinSampleSize());
    }
    memConfig.asyncStackEnable = asyncStackEnable_;
    memConfig.asyncFlag = 0x07FF;
    memConfig.largestSize = ts1_ > 0 ? static_cast<size_t>(ts1_) : memConfig.largestSize;
    memConfig.secondLargestSize = ts2_ > 0 ? static_cast<size_t>(ts2_) : memConfig.secondLargestSize;
    memConfig.targetSoName = targetSoNames_;
    HIVIEW_LOGI("sampleSize: %{public}ld, largestSize: %{public}ld, secondLargestSize: %{public}ld",
        memConfig.sampleSize, memConfig.largestSize, memConfig.secondLargestSize);
    HIVIEW_LOGI("dump so names %{public}s", memConfig.targetSoName.c_str());
}

MemProfilerParam NativeLeakDumpState::BuildGwpAsanProfilerParam(shared_ptr<NativeLeakInfo> &nlInfo, int fd) const
{
    GwpAsanConfig gwpAsanConfig = {
        .holdTime = NativeLeakConfig::GetInstance().GetGwpHoldTimeSecond(),
        .maxEntries = NativeLeakConfig::GetInstance().GetGwpMaxEntries(),
        .maxStackDepth = NativeLeakConfig::GetInstance().GetGwpMaxStackDepth(),
    };
    return MemProfilerParam {
        .duration = 0,
        .resType = NO_RES_TYPE,
        .profilerType = GWP_ASAN,
        .faultInfo = static_pointer_cast<FaultInfoBase>(nlInfo),
        .waitProfilerTime = PROFILER_NMD_WAIT_TIME,
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = {},
        .saConfig = {},
        .gwpAsanConfig = gwpAsanConfig
    };
}

void NativeLeakDumpState::DumpGwpAsanInfo(shared_ptr<NativeLeakInfo> &nlInfo) const
{
    if (nlInfo == nullptr) {
        HIVIEW_LOGE("nlInfo is nullptr");
        return;
    }
    int fd = -1;
    OldLogCleaner::GetInstance().GetLogFile(GWPASAN, nlInfo.get(), fd, true);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", nlInfo->GetGwpAsanPath().c_str(),
            strerror(errno));
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    string detailHead = string("\n*****************************\n") +
        string("LOGGER_GWP_ASAN_INFO\n");

    bool isDied = nlInfo->IsDied();
    if (isDied) {
        string curProcessName = DfrUtil::GetInstance().GetProcessName(nlInfo->GetPid());
        detailHead += "detector pid " + to_string(nlInfo->GetPid()) + " may be died, raw process name is "
            + nlInfo->GetProcessName() + ", but now is " + curProcessName + "\n";
    }
    size_t detailHeadSize = detailHead.size();
    ssize_t detailWrite = write(fd, detailHead.c_str(), detailHeadSize);
    if (detailWrite < 0 || static_cast<size_t>(detailWrite) != detailHeadSize) {
        HIVIEW_LOGE("write detailHead failed, ret: %{public}zd", detailWrite);
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return;
    }
    if (isDied) {
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return;
    }
    MemProfilerParam profilerParam = BuildGwpAsanProfilerParam(nlInfo, fd);
    bool ret = DfrProfilerManager::GetInstance().DoProfiler(profilerParam);
    if (!ret) {
        HIVIEW_LOGE("dump gwpAsan process %{public}s failed", nlInfo->GetProcessName().c_str());
    }
    fdsan_close_with_tag(fd, FDSAN_TAG);
}

} // namespace HiviewDFX
} // namespace OHOS
