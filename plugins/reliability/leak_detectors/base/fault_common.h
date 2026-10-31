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

#ifndef DFR_FAULT_COMMON_H
#define DFR_FAULT_COMMON_H

#include <fstream>
#include <sys/types.h>
#include <hiview_logger.h>
#include <string>
#include "telemetry_types.h"

namespace OHOS {
namespace HiviewDFX {
inline constexpr size_t MAX_MONITOR_PROCESS_NUMS = 10;
inline constexpr int BYTE_PER_KB = 1024;
inline constexpr int KB_PER_MB = 1024;
inline constexpr int BYTE_PER_MB = BYTE_PER_KB * KB_PER_MB;
inline constexpr int MB_PER_GB = 1024;
inline constexpr int KB_PER_GB = KB_PER_MB * MB_PER_GB;
// ASCII printable character range: 0x20(space) ~ 0x7E(~)
inline constexpr unsigned char ASCII_PRINTABLE_MIN = 0x20;
inline constexpr unsigned char ASCII_PRINTABLE_MAX = 0x7E;
inline constexpr int MIN_APP_USERID = 10000;
inline constexpr uint64_t REPORT_MEMORY_LEAK_LIMIT = 1.5 * KB_PER_GB;
inline constexpr int WAIT_TO_UNFREEZE_PROCESS = 1;
inline constexpr int MAX_RECORD_NUM = 30;

inline constexpr uint32_t LONG_WEIGHT = 16;
inline constexpr uint32_t SHORT_WEIGHT = 8;

inline constexpr int REQ_DMA_INFO_SIZE = 4096;
inline constexpr int HMCAP_THREAD_NAME_SIZE = 16;
inline constexpr int ION_INFO_SIZE = 4096;

//  not open temporary
inline constexpr const char* const STR_SYSTEM_CAMERA_QUICK_KILL = "SystemCameraQuickKill";
inline constexpr const char* const STR_OTHER_CAMERA_QUICK_KILL = "OtherCameraQuickKill";
inline constexpr const char* const STR_ZSWAPD_PRESSURE = "ZswapdPressure";
inline constexpr const char* const STR_BIG_MEMORY_KILL = "BigMemoryKill";
inline constexpr const char* const STR_GPU_LEAK_KILL = "GpuLeakKill";
inline constexpr const char* const STR_APP_LIMIT_KILL = "AppLimitKill";
inline constexpr const char* const STR_ACCOUNT_COLD_SWITCH_KILL = "AccountColdSwitchKill";
inline constexpr const char* const STR_GPU_MEMORY_READ_ERR = "GpuMemoryReadErr";
inline constexpr const char* const STR_DMA_MEMORY_READ_ERR = "DmaMemoryReadErr";
inline constexpr const char* const STR_DMABUF_QUICK_KILL = "DmaKiller";
inline constexpr const char* const STR_MSDP_BOTTOM_N_QUICK_KILL = "MsdpBottomNQuickKill";
inline constexpr const char* const EVENT_USER_NOT_CARE_CHARGE_SLEEP = "common.event.USER_NOT_CARE_CHARGE_SLEEP";
inline constexpr const char* const PERMISSION_PSPS = "ohos.permission.PERCEIVE_SMART_POWER_SCENARIO";
inline constexpr const char* const DOMAIN_KERNEL_VENDOR = "KERNEL_VENDOR";
inline constexpr const char* const EVENT_NAME_VMA_LEAK = "VMA_LEAK";
inline constexpr const char* const EVENT_NAME_FILE_STRUCT = "FILE_STRUCT_OVER_LIMIT";
inline constexpr const char* const EVENT_NAME_CMA = "CMA";
inline constexpr const char* const EVENT_NAME_SLUB_OBJ = "SLUB_OBJ";
inline constexpr const char* const EVENT_NAME_LOWMEM = "LOWMEM";
inline constexpr const char* const EVENT_NAME_SERVICE_WARNING = "SERVICE_WARNING";
inline constexpr const char* const EVENT_NAME_THREAD_BLOCK3S = "THREAD_BLOCK_3S";
inline constexpr const char* const EVENT_NAME_LDK_SLUB_OBJ = "LDK_SLUB_OBJ";
inline constexpr const char* const EVENT_NAME_ARK_STATS = "ARK_STATS_DUMP";
inline constexpr const char* const EVENT_NAME_SET_MEMORY = "SA_VM_SET_MEMORY";
inline constexpr const char* const EVENT_NAME_PROCESS_KILL = "PROCESS_KILL";
inline constexpr const char* const EVENT_NAME_ASHMEM_OVER_LIMIT = "ASHMEM_PHYSICAL_SIZE_OVER_LIMIT";
inline constexpr const char* const EVENT_NAME_KERNEL_ZONE = "KERN_ZONE_FREE_LOW";
inline constexpr const char* const KILL_EXCESSIVE_PROCESS = "KILL_EXCESSIVE_PROCESS";
inline constexpr const char* const EVENT_NAME_COOBJ_STATUS = "COOBJ_STATUS";
inline constexpr const char* const EVENT_NAME_FW_MEM_ANOMALY = "FW_MEM_ANOMALY";
inline constexpr const char* const EVENT_NAME_ARK_WEB_V8_OOM = "PAGE_MEM_LEAK";
inline constexpr const char* const EVENT_NAME_CJ_OOM = "CJ_OOM";
inline constexpr const char* const EVENT_NAME_JSVM_OOM = "JSVM_OOM";
inline constexpr const char* const NAME_AV_CODEC = "AVCodec";
inline constexpr const char* const NAME_MEDIA_SERVICE = "media_service";
inline constexpr const char* const PROCESS_MEMORY_REPORT = "PROCESS_MEMORY_REPORT";
inline constexpr const char* const EVENT_NAME_SYSLOAD_DETECTOR = "SYSLOAD_DETECTOR_INFO";
inline constexpr const char* const NAME_FILE_STRUCT = "file-struct";
inline constexpr const char* const EVENT_MEMMGR = "memmgrservice";
inline constexpr const char* const EVENT_FORK_DUMP = "FORK_DUMP_SERVICE";
inline constexpr const char* const EVENT_UI_TREE = "UI_TREE";
// Telemetry path and property constants are now in telemetry_types.h
inline constexpr const char* const BBOX_PATH = "/dev/bbox";
inline constexpr const char* const MEMORY_LEAK_ENABLE_PROPERTY = "hiview.memleakcheck";
inline constexpr const char* const MEMORY_LEAK_TEST_PROPERTY = "hiview.memleak.test";
inline constexpr const char* const NATIVE_LEAK_DETECT_PROPERTY = "hiview.nativeleak.foreground.detect";
inline constexpr const char* const FD_LEAK_ENABLE_PROPERTY = "hiview.fdleakcheck";
inline constexpr const char* const FD_LEAK_TEST_PROPERTY = "hiview.fdleak.test";
inline constexpr const char* const THREAD_LEAK_ENABLE_PROPERTY = "hiview.threadleakcheck";
inline constexpr const char* const THREAD_LEAK_TEST_PROPERTY = "hiview.threadleak.test";
inline constexpr const char* const HILOG_BUFFER_SIZE_PROPERTY = "persist.sys.hilog.buffersize.core";
inline constexpr const char* const DIGIT = "0123456789";
inline constexpr const char* const APP_OUT_SANDBOX_DIR_PREFIX = "/data/app/el2/";
inline constexpr const char* const KEY_HIVIEW_USER_TYPE = "const.logsystem.versiontype";
inline constexpr const char* const KEY_HIVIEW_DEVELOP_MODE = "persist.hiview.leak_detector";
inline constexpr const char* const KEY_DEVICE_TYPE = "const.product.devicetype";
inline constexpr const char* const METADATA_PATH = "/system/etc/arkcompiler/metadata.json";
inline constexpr const char* const GLOBAL_REGION = "const.global.region";
inline constexpr const char* const REGION_CN = "CN";
inline constexpr const char* const EVENT_LOG_PATH = "/data/log/eventlog";
// memory leak dir
inline constexpr const char* const RESOURCE_LEAK_PATH = "/data/log/reliability/resource_leak";
inline constexpr const char* const MEMORY_LEAK_PATH = "/data/log/reliability/resource_leak/memory_leak";
inline constexpr const char* const THREAD_LEAK_PATH = "/data/log/reliability/resource_leak/thread_leak";
inline constexpr const char* const FILE_DESCRIPTOR_LEAK_PATH = "/data/log/reliability/resource_leak/fd_leak/";
inline constexpr const char* const SYSLOAD_DETECTOR_LEAK_PATH = "/data/log/sysres/mem";
inline constexpr const char* const SYSLOAD_DETECTOR_MEM_PATH = "/data/log/sysres/mem";
inline constexpr const char* const SYSLOAD_DETECTOR_CPU_PATH = "/data/log/sysres/cpu";
inline constexpr const char* const SYSLOAD_DETECTOR_KMSG_PATH = "/data/log/sysres/kmsg";
inline constexpr const char* const SYSLOAD_DETECTOR_SUMMARY_PATH = "/data/log/sysres/summary";
inline constexpr const char* const SYSLOAD_DETECTOR_SYSLOG_PATH = "/data/log/sysres/syslog";
inline constexpr const char* const EVENT_NAME_MEMORY_OVER_WARNING = "RENDER_MEMORY_OVER_WARNING";
inline constexpr const char* const EVENT_NAME_MEMORY_OVER_ERROR = "RENDER_MEMORY_OVER_ERROR";
inline constexpr const char* const NAME_VMA_LEAK = "vma_leak";
inline constexpr const char* const NAME_KERNEL_ZONE = "kernel_zone";
inline constexpr const char* const NAME_GPU_RS_LEAK = "gpu_rs";
inline constexpr const char* const NAME_FD_RS_LEAK = "fd_rs";
inline constexpr const char* const RENDER_SERVICE_PROCESS = "render_service";
inline constexpr const char* const LOGGER_MEMCHECK_MEMINFO = "LOGGER_MEMCHECK_MEMINFO";
inline constexpr const char* const LOGGER_MEMCHECK_SAMPLE_NMD_INFO = "LOGGER_MEMCHECK_SAMPLE_NMD_INFO";
inline constexpr const char* const LOGGER_MEMCHECK_ASHMEM_INFO = "LOGGER_MEMCHECK_ASHMEM_INFO";
inline constexpr const char* const LOGGER_MEMCHECK_ION_INFO = "LOGGER_MEMCHECK_ION_INFO";
inline constexpr const char* const LOGGER_MEMCHECK_GPUMEM_INFO = "LOGGER_MEMCHECK_GPUMEM_INFO";
inline constexpr const char* const LOGGER_MEMCHECK_RS_DUMP = "LOGGER_MEMCHECK_RS_DUMP";
inline constexpr const char* const LOGGER_MEMCHECK_MEMTRACK_HEAD = "/************* memtrack info *****************/";
inline constexpr const char* const LOGGER_MEMCHECK_MEMTRACK_END = "/************* end memtrack **************/";
inline constexpr const char* const MEM_INFO_PATH = "/proc/meminfo";
inline constexpr const char* const DMAHEAP_INFO_PATH = "/proc/dmaheap_process_info";
inline constexpr const char* const ASHMEM_INFO_PATH = "/proc/ashmem_process_info";
inline constexpr const char* const GPU_INFO_PATH = "/proc/gpumem_process_info";
inline constexpr const char* const MEM_SLICE_PATH = "/proc/sys/vm/memctrl/mem_slice";
inline constexpr const char* const GPU_MEMORY_PATH = "/proc/gpu_memory";
inline constexpr const char* const SLUB_INFO_PATH = "/proc/sys/vm/memctrl/slab_leakage_threshold";
inline constexpr const char *const PROC_SYSVIEW = "/proc/sysview";
inline constexpr const char* const PROC_SLABINFO_PATH = "/proc/slabinfo";
inline constexpr const char* const HMFS_ORPHAN_NODES_DETAIL_INFO_PATH =
    "/sys/fs/hmfs/userdata/orphan_nodes_detail_info";
inline constexpr const char* const COMPRESS_HISTORY_PATH = "/data/service/el1/public/hiview/compress_history";
inline constexpr const char* const DEFAULT_BUNDLENAME = "unknow_bundleName";
inline constexpr const char* const HM_KERNEL_SKIP_KERNEL_TYPE = "vmalloc";
inline constexpr const char* const LEAKED_FD_TOP_10 = "Leaked fd Top 10:\n";
inline constexpr const char* const TOP_DIR_TYPE_10 = "Top Dir Type 10:\n";
inline constexpr const char* const SYSLOAD_PATH = "/dev/sysload";
inline constexpr const char* const ION_TYPE = "ion";
inline constexpr const char* const PARAM_PHONE_FOCUS_MODE = "persist.phone_focus.mode.status";
inline constexpr const char* const OOM_TYPE = "OOMDump";
// event of memory leak
inline constexpr const char* const PSS_MEMORY_STR = "PSS_MEMORY";
inline constexpr const char* const GPU_LEAK_STR = "GPU_LEAK";
inline constexpr const char* const ION_LEAK_STR = "ION_LEAK";
inline constexpr const char* const RSS_LEAK_STR = "RSS_LEAK";
// process subtype
inline constexpr const char *const ASSET_ACCELERATION_TYPE = "asset_acceleration_type";
inline constexpr const char *const SERVICE_TYPE = "service_type";
inline constexpr const char *const EXTENSION_DEFAULT = "extension_default";
inline constexpr const char *const NATIVE_DEFAULT = "native_default";
inline constexpr const char *const APP_DEFAULT = "app_default";

// profiler tag
// GPU Memory
inline constexpr const char* const RES_GPU_VK_STR = "RES_GPU_VK";
inline constexpr const char* const RES_GPU_GLES_IMAGE_STR = "RES_GPU_GLES_IMAGE";
inline constexpr const char* const RES_GPU_GLES_BUFFER_STR = "RES_GPU_GLES_BUFFER";
inline constexpr const char* const RES_GPU_CL_IMAGE_STR = "RES_GPU_CL_IMAGE";
inline constexpr const char* const RES_GPU_CL_BUFFER_STR = "RES_GPU_CL_BUFFER";

// sysevent calling process check
constexpr int HIVIEW_UID = 1201;
constexpr int PC_VM_UID = 7700;
constexpr int KERNEL_UID = 0;
constexpr int RENDER_SERVICE_UID = 1003;
constexpr int FOUNDATION_UID = 5523;
constexpr int MEMMGR_UID = 1111;
constexpr int SERVICE_UID = 10000;
constexpr int SYSMGR_PID = 2;
constexpr int HIDUMPER_SERVICE_UID = 1212;

inline constexpr const char* const REPORT_EVENT_TYPE[] = {
    "MEMORY_LEAK",
    "MEMORY_LEAK_WP",
};

inline constexpr const char* const PROCESS_TYPE_STRING[] = {
    "NOT_OVER_BASELINE",
    "ABNORMAL_PROCESS",
    "NATIVE",
    "APP",
    "GPURS",
};

inline constexpr const char* const NOTIFY_TYPE_STRING[] = {
    "JS_LEAK",
    "LEAK_STACK",
    "ARKWEB_LEAK",
    "JSVM_LEAK",
    "KMP_LEAK"
};

inline constexpr const char* const LEAK_GRADE_STRING[] = {
    "NO_LEAK",
    "INFO",
    "WARNING",
    "ERROR",
    "CONTROL",
};

inline constexpr const char* const STATE_TO_STR[] = {
    "[ProcIdleState]",
    "[ProcSampleState]",
    "[ProcJudgeState]",
    "[ProcDumpState]",
    "[ProcDumpKillState]",
    "[ProcTelemetryState]",
    "[ProcReportState]",
    "[ProcReportOverLimitState]",
    "[ProcAppReportState]",
    "[ProcAppReportKillState]",
    "[ProcPredictionState]",
    "[ProcPreventionState]",
    "[ProcToleranceState]",
    "[ProcRemovalState]",
    "[ProcCreateState]",
    "[ProcDiedState]",
    "[ProcFinishedState]"
};

typedef enum ErrCode {
    SUCCESSED = 0,
    FAILURE,
    NULL_POINTER,

    // fd leak dump error
    ERROR_WRITE_PARCEL,
    ERROR_READ_PARCEL,
    ERROR_GET_SYSTEM_ABILITY_MANAGER,
    ERROR_GET_DUMPER_SERVICE,
    ERROR_ADD_DEATH_RECIPIENT,
    ERR_NO_MEMORY
} ErrorCode;

enum FaultStateType {
    PROC_IDLE_STATE = 0,
    PROC_SAMPLE_STATE,
    PROC_JUDGE_STATE,
    PROC_DUMP_STATE,
    PROC_DUMP_KILL_STATE,
    PROC_TELEMETRY_STATE,
    PROC_REPORT_STATE,
    PROC_REPORT_OVERLIMIT_STATE,
    PROC_APP_REPORT_STATE,
    PROC_APP_REPORT_KILL_STATE,
    PROC_PREDICTION_STATE,
    PROC_PREVENTION_STATE,
    PROC_TORLERANCE_STATE,
    PROC_REMOVAL_STATE,
    PROC_CREATE_STATE,
    PROC_DIED_STATE,
    PROC_FINISHED_STATE,
    PROC_COUNTS
};

enum LeakGrade {
    NO_LEAK = 0,
    LEAK_INFO,
    LEAK_WARNING,
    LEAK_ERROR,
    LEAK_CONTROL,
    LEAK_MAX
};

// TelemetrySource enum is now in telemetry_types.h

inline constexpr const char* const TELEMETRY_SOURCE_NAME[] = {
    "SYSTEM_TELEMETRY",
    "APP_TELEMETRY",
    "unknowLog"
};

enum DumpLogType {
    NO_LOG = 0,
    BASIC_LOG = 1,
    BASIC_AND_EXTERNAL_LOG = 1 << 1,
    ION_LOG = 1 << 2,
    GPU_LOG = 1 << 3,
    PSS_LOG = 1 << 4,
    MEDIA_LOG = 1 << 5,
    AV_CODEC_LOG = 1 << 6,
};

enum LeakSource {
    PSS = 0,
    RSS
};

struct AppResourceLimtEvent {
    pid_t pid;
    int threshold;
    bool hasDebugLog;
};

struct NotifyProcessInfo {
    pid_t pid { 0 };
    pid_t tid { 0 };
    pid_t uid { 0 };
    std::string processName;
    std::string hapVersion;
    int limitSize { INT_MAX }; // ARKTS heap limit
    int liveObjectSize { 0 }; // now heap used
    std::string heapdumpType;
    std::string rawheapFile;
    std::string staticRawheapFile;
    std::string appRunningId;
    std::string eventConfig;
    bool willReportAppeventImmediately { false };
    bool hasGotRawheap { false };
    bool hasGotStaticRawheap { false };
    std::string arktsType; // "dynamic" for dynamic app, "static" for static app
};

// use for commonKey
enum ConfigValueType {
    TYPE_INT,
    TYPE_BOOL,
    TYPE_INT64,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_VALUE_UNKNOWN
};

inline const std::unordered_map<std::string, ConfigValueType> StringToTypeMap = {
    { "TYPE_INT", TYPE_INT },
    { "TYPE_BOOL", TYPE_BOOL },
    { "TYPE_INT64", TYPE_INT64 },
    { "TYPE_FLOAT", TYPE_FLOAT },
    { "TYPE_DOUBLE", TYPE_DOUBLE },
    { "TYPE_STRING", TYPE_STRING },
    { "TYPE_VALUE_UNKNOWN", TYPE_VALUE_UNKNOWN }
};

struct ConfigValueStru {
    ConfigValueType type;
    union {
        int intValue;
        bool boolValue;
        int64_t int64Value;
        float floatValue;
        double doubleValue;
    };
    std::string stringValue;
};

// mapping kernel struct
enum MenuType {
    MENU_UNKNOWN = 0,
    MENU_DMA,
    MENU_GPU,
    MENU_MAX
};

enum DataType {
    TYPE_DATA_TYPE_UNKNOWN = 0,
    TYPE_PER_PID_SIZE,
    TYPE_ALL_PID_INFO,
    TYPE_DATA_TYPE_MAX
};

struct ProcIonMap {
    int pid { 0 };
    char procName[HMCAP_THREAD_NAME_SIZE];
    int startIndex {0};
    int endIndex { 0 };
    int entryCnt { 0 };
    unsigned long long totalSize;
};

struct FileParams {
    // common info
    std::string procName;
    std::string fileName;
    std::string logPath;
    // fout mode
    std::ios_base::openmode foutMode;
    // fd flags and mode
    unsigned int openFlags;
    mode_t openMode;
};

struct IonInfo {
    unsigned int magic { 0 };
    enum MenuType menu;
    enum DataType type;
    unsigned int reqMemSize { 0 };
    struct ProcIonMap pidEntry;
    char allProcIonInfo[ION_INFO_SIZE];
};

enum KillingState {
    SHOULD_NOT_KILL = 0,
    SHOULD_WAIT_KILL,
    SHOULD_KILL,
};

enum CmdType {
    CMD_NONE = 0,
    CMD_SET_MEM_TYPE_LIMIT,
    CMD_GET_MEM_TYPE_VALUE,
    CMD_GET_MEM_DETAIL_INFO,
    CMD_MAX
};

struct CmdInfo {
    uint32_t magic { 0 };
    uint32_t pid { 0 };
    uint32_t cmdType { CMD_NONE };
    uint32_t infoType { 0 };
    uint32_t dfxLimit { 0 };
    uint32_t appKillHardThreshold { 0 };
    int32_t copyErr { 0 };
    uint64_t offset { 0 };
    uint64_t size { 0 };
    uint64_t timestamp { 0 };
    char data[0];
};

struct MetaInfoParams {
    std::string eventName;
    std::string processName;
    pid_t pid;
    std::string firstFrame;
    std::string secondFrame;
    std::string thirdFrame;
    std::string timeStamp;
    std::string basePath;
    std::string versionName;
    std::string versionCode;
    std::string foreground;
    std::string appRunningId;
};
} // HiviewDFX
} // namespace OHOS
#endif // DFR_FAULT_COMMON_H
