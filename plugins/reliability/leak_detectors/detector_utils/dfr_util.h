/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * Author: huangyu
 * Create: 2023-11-16
 */

#ifndef DFR_UTIL_H
#define DFR_UTIL_H

#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <list>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <extension_ability_info.h>
#include <singleton.h>
#include "fault_common.h"
#include "telemetry_types.h"
#include "app_event_handler.h"
#include "app_event_publisher.h"
#include "ffrt.h"
#include "operators/handlers/display/ForegroundAttr.h"
#include "app_mgr_client.h"
#include "app_malloc_info.h"
#include "app_mgr_interface.h"

namespace OHOS {
namespace HiviewDFX {
using Range = std::pair<uint64_t, uint64_t>;
using HookSize = std::pair<Range, Range>;
using GpuHookSize = std::map<std::string, HookSize>;

// adapt kernel ioctl
#define SYSLOAD_DETECTOR_MAGIC 0xE5AC01
#define SUMMARY_LOG_BASE 'S'
#define GET_PROC_DMA_INFO _IOWR(SUMMARY_LOG_BASE, 0x04, int32_t)

#define FDSAN_LEAK_TAG 1
#define LEAK_DOMAIN 0xD002DA0

constexpr uint64_t FDSAN_TAG = (static_cast<uint64_t>(LEAK_DOMAIN) << 32) | FDSAN_LEAK_TAG;

constexpr int WAIT_HISYSEVENT_LOGPACK_FINISHED = 5;
constexpr int WAIT_HIAPPEVENT_LOGPACK_FINISHED = 5;
constexpr int WAIT_FILE_COPY_FINISHED = 20;
constexpr int LEAK_SHA256_LENGTH = 32;

// psi
constexpr int DEFAULT_RAB_THRESHOLD_MB = 800;
constexpr int DEFAULT_STA_THRESHOLD_MB = 350;

// ioctl to kernel
constexpr uint32_t MEMCHECK_MAGIC = 0x5377FEFA;
constexpr uint32_t MEMCHECKIO = 0xAF;
constexpr uint32_t MEMCHECK_STACKINFO_MAXSIZE = (600 * 1024);
constexpr uint32_t MEMCHECK_DETAILINFO_MAXSIZE = (100 * 1024);
constexpr uint32_t MEMCHECK_CMDINFO_MAXSIZE = (10 * 1024 * 1024);
constexpr uint32_t LOGGER_MEMCHECK_RSS_READ = _IO(MEMCHECKIO, 1);
constexpr uint32_t LOGGER_MEMCHECK_COMMAND = _IO(MEMCHECKIO, 2);
constexpr uint32_t LOGGER_MEMCHECK_STACK_READ = _IO(MEMCHECKIO, 3);
constexpr uint32_t LOGGER_MEMCHECK_DETAIL_READ = _IO(MEMCHECKIO, 5);
constexpr uint32_t LOGGER_MEMCHECK_CMD_INFO = _IO(MEMCHECKIO, 7);
constexpr uint32_t LOGGER_MEMCHECK_FD_STACK_READ = _IO(MEMCHECKIO, 9);

// ioctl type
constexpr uint32_t MTYPE_INVALID = 0;
constexpr uint32_t MTYPE_USER_PSS_JAVA = (1 << 0);
constexpr uint32_t MTYPE_USER_PSS_NATIVE = (1 << 1);
constexpr uint32_t MTYPE_USER_PSS = (MTYPE_USER_PSS_JAVA | MTYPE_USER_PSS_NATIVE);
constexpr uint32_t MTYPE_USER_ION = (1 << 4);
constexpr uint32_t MTYPE_USER_GPU = (1 << 5);
constexpr uint32_t MTYPE_USER_ASHMEM = (1 << 6);
constexpr uint32_t MTYPE_USER_OPEN = (1 << 8);
constexpr uint32_t MTYPE_USER_SOCKET = (1 << 9);
constexpr uint32_t MTYPE_USER_PIPE = (1 << 10);
constexpr uint32_t MTYPE_USER_MMAP = (1 << 11);
constexpr uint32_t MTYPE_USER_FENCE = (1 << 12);
constexpr uint32_t MTYPE_LDK_SLAB = (1 << 13);
constexpr uint32_t MTYPE_LDK_BUDDY = (1 << 14);
constexpr uint32_t MTYPE_LDK_VMA = (1 << 15);

// each fd leak type dump with ioctl
constexpr uint32_t MTYPE_EACH_FD_DUMP = (MTYPE_USER_ASHMEM | MTYPE_USER_SOCKET | MTYPE_USER_PIPE | MTYPE_USER_FENCE);
constexpr uint32_t MTYPE_CMD_INFO_TYPE = (MTYPE_EACH_FD_DUMP | MTYPE_LDK_SLAB);

constexpr uint32_t TYPE_CATEGORY_MASK = 0xFFFF0000;
// freestate type
constexpr uint32_t FREE_STATE_CHECK_TYPE_BASE = 0x00100000;
constexpr uint32_t FREE_STATE_OPEN_CHECK = (FREE_STATE_CHECK_TYPE_BASE | 0x001);
constexpr uint32_t FREE_STATE_CLOSE_CHECK = (FREE_STATE_CHECK_TYPE_BASE | 0x002);
constexpr uint32_t FREE_STATE_GET_LAST_OPEN_TIME = (FREE_STATE_CHECK_TYPE_BASE | 0x003);
// psi manager
constexpr uint32_t PSI_MANAGER_TYPE_BASE = 0x00200000;
constexpr uint32_t PSI_SET_RAB_THRESHOLD = (PSI_MANAGER_TYPE_BASE | 0x001);
constexpr uint32_t PSI_SET_STA_THRESHOLD = (PSI_MANAGER_TYPE_BASE | 0x002);

// kill report type
constexpr uint32_t KILL_REPORT_TYPE_DETAIL_BASE = 0x01000000;
constexpr uint32_t ASHMEM_KILL_REPORT_DETAIL = (KILL_REPORT_TYPE_DETAIL_BASE | 0x001);
constexpr uint32_t GPU_KILL_REPORT_DETAIL = (KILL_REPORT_TYPE_DETAIL_BASE | 0x002);
constexpr uint32_t RSS_KILL_REPORT_DETAIL = (KILL_REPORT_TYPE_DETAIL_BASE | 0x003);
constexpr uint32_t RSS_KILL_REPORT_DETAIL_FREE = (KILL_REPORT_TYPE_DETAIL_BASE | 0x004);
constexpr uint32_t DMA_KILL_REPORT_DETAIL = (KILL_REPORT_TYPE_DETAIL_BASE | 0x005);
constexpr uint32_t STA_SMAPS_BUFFER_READ_DETAIL = (KILL_REPORT_TYPE_DETAIL_BASE | 0x007);
constexpr uint32_t STA_DMA_BUFFER_READ_DETAIL = (KILL_REPORT_TYPE_DETAIL_BASE | 0x009);
constexpr uint32_t STA_GPU_BUFFER_READ_DETAIL = (KILL_REPORT_TYPE_DETAIL_BASE | 0x00b);

// dlopen so to leak track
constexpr uint32_t US_PER_SECOND = 1000 * 1000;
constexpr time_t SECONDS_IN_A_WEEK = 7 * 24 * 60 * 60;

constexpr mode_t DEFAULT_LOG_FILE_MODE = 0644;

// ABCDEFG ==> AB*DE*G
constexpr int ANONY_MASK_COUNT = 3;
constexpr int ANONY_MASK_OFFSET = 2;

inline constexpr const char *const DETECTOR_TYPE_NAME[] = {"none_leak_detector",   "fd_leak_detector",
                                                           "thread_leak_detector", "notify_leak_detector",
                                                           "kernel_leak_detector", "native_leak_detector"};

enum MeminfoRecordItem {
    MEMINFO_NAME = 1,
    MEMINFO_MEMORY_USED,
};

enum MemCmd { MEMCMD_NONE, MEMCMD_ENABLE, MEMCMD_DISABLE, MEMCMD_SAVE_LOG, MEMCMD_CLEAR_LOG, MEMCMD_MAX };

struct TrackCmd {
    uint32_t magic;
    uint32_t id;
    uint32_t type;
    uint64_t timestamp;
    enum MemCmd cmd;
};

enum State {
    UNKNOWN = 0,
    STATE_TRUE,
    STATE_FALSE,
};

enum CommonEventType {
    EVENT_SCREEN_OFF = 1,
    EVENT_SCREEN_ON,
    EVENT_POWER_CONNECTED,
    EVENT_POWER_DISCONNECTED,
    EVENT_CHARGE_SLEEP,
    EVENT_NOT_CHARGE_SLEEP,
};

// Leak type enum is now in telemetry_types.h

inline const std::map<uint64_t, std::string> LeakTypeStr{
    {JS_LEAK_TYPE, "JS_LEAK"},
    {NATIVE_LEAK_TYPE, "NATIVE_LEAK"},
    {JS_OOM_TYPE, "JS_OOM"},
    {FD_LEAK_TYPE, "FD_LEAK"},
    {THREAD_LEAK_TYPE, "THREAD_LEAK"},
    {GPU_LEAK_TYPE, "GPU_LEAK"},
    {ION_LEAK_TYPE, "ION_LEAK"},
    {OTHER_LEAK, "OTHER_LEAK"},
    {RSS_LEAK_TYPE, "RSS_LEAK"},
    {EXT_PSS_LEAK_TYPE, "EXT_PSS"},
    {GPU_RS_LEAK_TYPE, "GPU_RS"},
    {ASHMEM_LEAK_TYPE, "ASHMEM_LEAK"},
    {LITE_PROFILER_LEAK_TYPE, "LITE_PROFILER"},
};

// SubLeakType enum is now in telemetry_types.h

inline const std::map<int, std::string> SubLeakTypeStr{
    {SUB_NATIVE_HEAP_LEAK, "NATIVE_HEAP_LEAK"},
    {SUB_ARKTS_LEAK, "ARKTS_HEAP_LEAK"},
    {SUB_ARKTS_STA_LEAK, "ARKTS_STA_HEAP_LEAK"},
    {SUB_KOTLIN_LEAK, "KOTLIN_HEAP_LEAK"},
    {SUB_JSVM_LEAK, "JSVM_HEAP_LEAK"},
    {SUB_ARKWEB_V8_LEAK, "ARKWEB_V8_LEAK"},
    {SUB_ASHMEM_LEAK, "ASHMEM_LEAK"},
    {SUB_GPU_LEAK, "GPU_LEAK"},
    {SUB_ION_LEAK, "ION_LEAK"},
    {SUB_AVCODEC_LEAK, "AVCODEC_LEAK"},
    {SUB_MEDIA_LEAK, "MEDIA_LEAK"},
};

// TelemetryLeakInfo struct is now in telemetry_types.h

enum CommonDumpType {
    NO_ACTION_DUMP = 0,
    SMAPS_INFO = 1 << 0,
    RENDER_SERVICE_MEM = 1 << 1,
    ASHMEM_PROCESS_INFO = 1 << 2,
    ION_PROCESS_INFO = 1 << 3,
    GPU_PROCESS_INFO = 1 << 4,
    PROFILER = 1 << 5,
    NMD = 1 << 6,
    SYSTEM_HEAP = 1 << 7,
    ION_NEW_PROC = 1 << 8,
    SLAB_INFO = 1 << 9,
    CMA_INFO = 1 << 10,
    VMALLOC_INFO = 1 << 11,
    SYSVIEW = 1 << 12,
    MEDIA_MEM = 1 << 13,
    AVC_MEM = 1 << 14,
};

inline const std::map<CommonDumpType, std::string> CommonDumpTypeStr{
    {SMAPS_INFO, "SMAPS_INFO"},
    {RENDER_SERVICE_MEM, "RENDER_SERVICE_MEM"},
    {ASHMEM_PROCESS_INFO, "ASHMEM_PROCESS_INFO"},
    {ION_PROCESS_INFO, "ION_PROCESS_INFO"},
    {GPU_PROCESS_INFO, "GPU_PROCESS_INFO"},
    {PROFILER, "PROFILER"},
    {NMD, "NMD"},
    {SYSTEM_HEAP, "SYSTEM_HEAP"},
    {ION_NEW_PROC, "ION_NEW_PROC"},
    {SLAB_INFO, "SLAB_INFO"},
    {CMA_INFO, "CMA_INFO"},
    {VMALLOC_INFO, "VMALLOC_INFO"},
    {SYSVIEW, "SYSVIEW"},
    {MEDIA_MEM, "MEDIA_MEM"},
    {AVC_MEM, "AVC_MEM"},
};

using CommonDumpPathList = std::vector<std::pair<CommonDumpType, std::string>>;

class DfrUtil : public Singleton<DfrUtil> {
    DECLARE_SINGLETON(DfrUtil);

public:
    bool IsMemLeakEnable();
    bool IsMemTestEnable();
    bool IsFdLeakEnable();
    bool IsFdTestEnable();
    bool IsThreadLeakEnable();
    bool IsThreadTestEnable();
    bool IsHilogBufferSizeModify();
    bool GetIsHmKernel();
    int32_t GetFocusMode();
    std::vector<std::string> Split(const std::string &str, char delim);
    CommonDumpPathList ParseDumpFunction(std::string &dumpFunctions, std::string defaultPath);
    std::string GetProcPath(std::string meminfoName, std::string procName);
    bool SafeStoi(const std::string &str, int &value);
    bool SafeStoll(const std::string &str, long long &value);
    bool CreateDir(const std::string &path, mode_t mode = 0775);  // 0775:rwxrwxr-x
    bool IsDirectory(const std::string &path);
    std::vector<std::string> GetSubDir(const std::string &path, bool digit = false);
    std::vector<std::string> GetSubFile(const std::string &path, bool digit = false);
    std::vector<std::string> GetSubNodes(const std::string &path, bool digit = false);
    std::vector<pid_t> GetAllPids();
    std::string ReadFileByChar(const std::string &path);
    bool FileExist(const std::string &path);
    bool IsFileEmpty(const std::string &path);
    bool RemoveFile(const std::string &path);
    bool CopyFile(const std::string &path, const std::string &newPath);
    bool CopyFileNoReal(const std::string &path, const std::string &newPath);
    bool RenameFile(const std::string &path, const std::string &newPath);
    int32_t GetAppUserId(int32_t pid);
    time_t StrToTimeStamp(const std::string &tmStr);
    time_t StrToTimeStampT(const std::string &tmStr);
    std::string GetRealTime();
    std::string GetRealTimeStampStr();
    std::string TimeStampToStr(const time_t timeStamp);
    std::string TimeStampToSimplelyStr(const time_t timeStamp);
    std::vector<std::string> GetStatInfo(const std::string &path);
    std::string GetProcessName(pid_t pid);
    int GetProcessAdj(pid_t pid);
    time_t GetProcessStartTime(int pid);  // jiffes unit
    int32_t GetSwapPssByPid(pid_t pid);
    int32_t GetSwapRssByPid(pid_t pid);
    uint64_t GetProcessVss(int pid);
    uint64_t GetProcessRss(int pid);
    int GetParentPid(int pid);
    bool IsPathSafe(const std::string &path);
    time_t GetThreadStartTime(int pid, int tid);  // jiffes unit
    bool Is32BitProcess(int32_t pid);
    bool IsKernelProcess(int pid);
    int GetThreadNums(int pid);
    time_t GetRunningMonotonicTime();
    void RemoveAllFilesInDir(const std::string &path);
    pid_t GetProcessUid(pid_t pid);
    std::string GetApplicationNameByUid(pid_t uid);
    std::string GetApplicationVersionByPid(pid_t pid);  // get app version
    std::string GetApplicationVersionByUid(pid_t uid);
    bool IsBetaVersion();
    bool IsDevelopMode();
    bool IsBetaOrDevelopMode();
    int64_t GetSizeInProcNode(std::string nodeName, std::string typeName);
    bool CallSmapsRollup(pid_t pid, int &pss, int &swappss, int &rss);
    int GetRamSize();
    int GetTotalRamSize();
    int ConvertToString(const unsigned char hash[LEAK_SHA256_LENGTH], char *outstr, size_t len);
    int CalcBufferSha(const std::string &buffer, size_t bufSize, char *hash, size_t len);
    std::string CalcFingerPrint(const std::string &val, size_t mask);
    void GetStatm(int32_t pid, uint64_t &vss, uint64_t &rss);
    int32_t GetRssThreshold(pid_t pid);
    void GetMeminfo(uint64_t &avaliableMem, uint64_t &freeMem, uint64_t &totalMem);
    std::list<std::string> GetDightStrArr(const std::string &target);
    void SetAppEventHandle(std::shared_ptr<AppEventHandler> handler);
    std::string GetAllProcessIonInfo();
    std::string GetCacheProcInfo(const std::string &procFilePath);
    std::map<int, int> ExtractMemValue(const std::string &infoStr);
    std::map<int, int> GetIonValueFromKernel();
    bool IsPhone();
    bool IsTablet();
    bool IsPC();
    bool IsWatch();
    void TryToGetDeviceType();
    bool ConvertToULL(const std::string &str, uint64_t &value);
    bool ConvertToInt(const std::string &str, int &value);
    uint64_t GetDegital(const std::string &data, const std::string &startStr, const std::string &endStr);
    bool IsDeviceLowmem();
    bool AppendMetadata(const std::string &newPath);
    int32_t GetForegroundOsAccountLocalId();
    int GetFreeRomMb();
    bool LongerThanAWeek(time_t t1, time_t t2);
    bool IsApp(pid_t pid);
    bool ShouldDumpRs();
    bool IsOversea();
    std::string GetParentPath(const std::string &path);
    uint64_t ExtractDumpTraceTime(const std::string &eventId);
    std::pair<std::string, std::vector<std::string>> FreezeDumpTrace(uint64_t faultTime, const std::string &bundleName,
                                                                     bool isTelemetry = false);
    std::string GetTimeStrByEventId(const std::string &eventId);
    void CollectHiperf(const std::string &fileName, const std::vector<pid_t> &pids);
    std::string RealPath(const std::string &path);
    std::string MaskFilename(const std::string &filename);
    std::string AnonymizePath(const std::string &path);
    std::string AnonymizeInfo(const std::string &detailFdInfo);
    bool GetElementInfo(const std::string &line, std::vector<std::string> &elements, char interval);
    int GetProcessGpuMemory(pid_t pid);
    void UpdateGpuMemoryList();
    bool ShouldDetectAfterInterval(time_t lastDetectorTime, int intervalTime);
    int GetSlabNormal();
    std::string GenerateMetaInfoJson(const MetaInfoParams &params);
    bool CreateMetaInfoFile(const MetaInfoParams &params);
    std::string GetProductModelStr();
    std::string GetSysVersionStr();
    bool IsProcessRelatedLine(const std::string &line, size_t pos, const std::string &processName, char interval);
    bool IsProcessRelatedLine(const std::string &line, size_t pos, int pid, char interval);
    void UnfreezingProcess(pid_t pid);
    void RemoveOldProcFilePaths();
    static bool IsValidBundleName(const std::string &bundleName);
    std::string GetSandBoxCachePath(const std::string &bundleName);
    std::string GetBundleQuotaPath(const std::string &bundleName);
    std::string GetSubtypeByPpid(pid_t ppid);
    std::string GetExtensionSubtype(AppExecFwk::ExtensionAbilityType type);
    std::string GetSubtype(pid_t pid, pid_t ppid);
    void DumpSingleDetailFromIoctl(pid_t pid, uint32_t cmdType, uint32_t infoType, std::ofstream &os);
    std::string SendLeakCmdInfo(pid_t pid, uint32_t cmdType, uint32_t infoType,
                                uint32_t dfxLimit = 0, uint32_t appKillHardThreshold = 0);
    int ExtractKbValue(const std::string &content, const std::string &prefix);
    std::string JoinPidsStr(const std::vector<int32_t> &pids);
    std::string GetAppRunningUniqueIdByPid(pid_t pid);
    uint64_t GetDirSize(const std::string &dirPath);
    bool GetFilesInDir(const std::string &dirPath, std::vector<std::string> &fileList);
    void NotifyWarningToApp(int32_t pid, AppExecFwk::LeakType leakType, unsigned long leakSize,
                            const AppExecFwk::LeakDetailInfo &leakDetailInfo = AppExecFwk::LeakDetailInfo{});
    void DumpRssKillerSmaps(pid_t pid, std::string &smapsInfo);
    void FreeRssKillerSmaps(pid_t pid);
    std::string GetXattr(const std::string &path, const std::string &name);
    bool ReadResourceObserver(const std::string &xattrPath, std::string &runningId, uint64_t &atLeakType);
    int ReadAppTelemetryThreshold(const std::string &bundleName, AppExecFwk::AppTelemetryLeakType atlt);
    std::string GetXattrNameByATLT(AppExecFwk::AppTelemetryLeakType atlt);
    std::string GetLeakTypeName(uint64_t leakType, uint64_t subLeakType = 0);
    void CollectAppTelemetry(int32_t pid, AppExecFwk::AppTelemetryLeakType atlt, const GpuHookSize &hookSize = {});
    std::string SerializeHookSizeToJson(const GpuHookSize &hookSizeMap);
    bool ForkProcessToDumpSmaps(pid_t pid, CommonDumpType dumpType, const std::string &detectorTypeName);
    bool ForkProcessAndDump(pid_t pid, CommonDumpType dumpType, const std::string &detectorTypeName);
    int ExecuteChildProcess(pid_t pid, CommonDumpType dumpType);
    bool DumpRawheapInfo(OHOS::AppExecFwk::JsHeapDumpInfo &info);
    bool DumpHeapMemory(OHOS::AppExecFwk::MemDumpInfo &info);
    bool SkipForegroundDetectlimit();
    int GetWinIdByPid(pid_t pid);
    int ParseWinIdFromOutput(const std::string &output, pid_t pid);
    bool DumpUiTree(int fd, int &winId);
    bool WaitChildWithTimeout(pid_t childPid, int &status, const char *caller);
    void RunGetWinIdChild(int pipefd, int pipeReadFd);
    void GetAppTelemetryThresholdAndType(const std::string &bundleName, uint64_t &type, int *threshold = nullptr);
    void DumpAppPageInfo(pid_t pid, const std::string &processName, std::ofstream &fout) const;
    int32_t Int64ToInt32(int64_t value);
    template <typename T>
    bool PostAppEvent(const T &info)
    {
        if (appEventHandler_ == nullptr) {
            return false;
        }
        appEventHandler_->PostEvent(info);
        return true;
    }

protected:
    std::map<std::string, std::pair<std::string, time_t>> procFilePathMap_;
    ffrt::mutex procDumpMtx_;
    std::shared_ptr<AppEventHandler> appEventHandler_{nullptr};
    std::string deviceType_{"unknown"};
    State memLeakState_{UNKNOWN};
    State memTestState_{UNKNOWN};
    State fdLeakState_{UNKNOWN};
    State fdTestState_{UNKNOWN};
    State threadLeakState_{UNKNOWN};
    State threadTestState_{UNKNOWN};
    State developOrBetaModeState_{UNKNOWN};
    State isHmKernel_{UNKNOWN};
    std::map<pid_t, int32_t> gpuMemoryList_;
    ffrt::mutex gpuMemoryListMtx_;
    int totalRamSize_{0};
    std::atomic<bool> isDumping_{false};
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif  // DFR_UTIL_H
