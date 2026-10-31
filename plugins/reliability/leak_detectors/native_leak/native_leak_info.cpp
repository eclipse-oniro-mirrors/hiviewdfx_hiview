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
 * Author: zhengkaijie
 * Create: 2023-11-16
 */

#include "native_leak_info.h"
#include "hiview_logger.h"
#include "fault_common.h"
#include "fault_detector_manager.h"
#include "hitrace_meter.h"
#include "third_party_config.h"
#include "xpower_manager_client.h"
#include "telemetry_manager.h"
#include "old_log_cleaner.h"
#include "hisysevent.h"

using namespace std;
using namespace OHOS::XPowerManager;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakInfo");

namespace {
constexpr time_t SAMPLE_INTERVAL_SHORT = 100;
constexpr time_t SAMPLE_INTERVAL_NORMAL = 200;
constexpr time_t SAMPLE_INTERVAL_LONG = 400;
constexpr time_t SAMPLE_INTERVAL_DEFAULT = 200;

constexpr int SMALL_THRESHOLD_LINE = 80 * KB_PER_MB;
constexpr int BIG_THRESHOLD_LINE = 200 * KB_PER_MB;

constexpr int INTERVAL_RATIO_TEST = 10;
constexpr int INTERVAL_RATIO_DOUBLE = 2;
constexpr int SOFT_KILL_OVERLOAD_RATIO = 2; // totalMem over 2x soft-kill threshold means overload

constexpr int RS_OFFSET = -100 * KB_PER_MB;
constexpr int SMALL_LARGE_DIVIDER = 20 * KB_PER_MB;
constexpr int INFO_WARN_SMALL_DIVIDER = 6 * KB_PER_MB;
constexpr int WARN_ERROR_SMALL_DIVIDER = 10 * KB_PER_MB;
constexpr int ERROR_CONTROL_SMALL_DIVIDER = 20 * KB_PER_MB;
constexpr int INFO_WARN_LARGE_DIVIDER = 50 * KB_PER_MB;
constexpr int WARN_ERROR_LARGE_DIVIDER = 100 * KB_PER_MB;
constexpr int ERROR_CONTROL_LARGE_DIVIDER = 200 * KB_PER_MB;
constexpr int BYTE_TO_KB_SHIFT = 10;
constexpr float INFO_WARN_PCT_DIVIDER = 1.3;      // 130%
constexpr float WARN_ERROR_PCT_DIVIDER = 1.5;     // 150%
constexpr float RATE_OF_ONE_POINT_FIVE_GB = 1.5;  // Default rate when the localLimit value is over 1.5 GB
constexpr float ERROR_CONTROL_PCT_DIVIDER = 2.0;  // 200%
constexpr float KILL_DETECT_TRANS_PERCENT = 1.3;  // 1.3
constexpr int NATIVE_THRESHOLD_512M = 512 * KB_PER_MB;
constexpr int NATIVE_THRESHOLD_1024M = 1024 * KB_PER_MB;
constexpr int NATIVE_THRESHOLD_1536M = 1536 * KB_PER_MB; // 1.5GB
constexpr int NATIVE_THRESHOLD_2048M = 2048 * KB_PER_MB;
constexpr int NATIVE_THRESHOLD_2560M = 2560 * KB_PER_MB; // 2.5GB
constexpr int WEIGHT_8 = 8;
constexpr int WEIGHT_12 = 12;
constexpr int WEIGHT_16 = 16;
constexpr int WEIGHT_32 = 32;
constexpr int OVER_CNT_LAST_PRINT = -10;
constexpr double MULTIPLIER = 81.67;
constexpr double EXPONENT = 0.45;
constexpr bool IS_FILE_OPEN_APPEND  = true;
constexpr int REPEAT_REPORT_INTERVAL = SEC_PER_DAY;
constexpr int64_t PROFILER_SIZE_BASELINE = 1 * BYTE_PER_MB;

constexpr char SAMPLE_GRADE_SIGNAL[] = { 'N', 'I', 'W', 'E', 'C', 'C' };
const std::map<std::string, QuotaAction> ACTION_MAP = {
    { "warning",  ACTION_WARNING },
    { "softKill", ACTION_SOFT_KILL },
    { "kill",     ACTION_KILL },
    { "clean",     ACTION_CLEAN },
};
const std::unordered_map<ResourceQuotaCtl::ConditionInfo, QuotaCondition> CONDITION_MAP = {
    { ResourceQuotaCtl::CONDITION_FOREGROUND, CONDITION_FOREGROUND },
    { ResourceQuotaCtl::CONDITION_BACKGROUND, CONDITION_BACKGROUND },
    { ResourceQuotaCtl::CONDITION_STANDBY,    CONDITION_STANDBY }
};
const map<string, string> KILL_TYPE_MAP = {
    {"PSS_MEMORY", "mem"},
    {"GPU_LEAK", "gpu"},
    {"ION_LEAK", "ion"},
};
}

ffrt::mutex NativeLeakInfo::reportedMtx_;
map<string, map<string, vector<time_t>>> NativeLeakInfo::reportedMap_;
ffrt::mutex NativeLeakInfo::localMtx_;
atomic<int> NativeLeakInfo::thresholdUpdateCnt_ { 1 };
int NativeLeakInfo::vmExtraThreshold_ { 0 };
unordered_map<string, unordered_map<QuotaAction, Quota>> NativeLeakInfo::localThresholds_; // <name, threshold>
ffrt::mutex NativeLeakInfo::processActionMtx_;
unordered_map<string, uint32_t> NativeLeakInfo::processActionList_;
ffrt::mutex NativeLeakInfo::dumpedTraceMtx_;
map<string, time_t> NativeLeakInfo::dumpedTraceMap_;

NativeLeakInfo::NativeLeakInfo(pid_t pid, string name)
    : MemoryLeakInfoBase(pid, name)
{
    InitInfo(name);
    GetAppRunningId();
    GetUid();
}

NativeLeakInfo::NativeLeakInfo(pid_t pid, string name, std::string processType)
    : MemoryLeakInfoBase(pid, name), processType_(processType)
{
    InitInfo(name);
    GetAppRunningId();
    GetUid();
}

NativeLeakInfo::~NativeLeakInfo() {}

void NativeLeakInfo::InitInfo(string &name)
{
    SetStartTime();
    dumperInfo_.detectorType = NATIVE_LEAK_DETECTOR;
    if (name == "render_service") {
        lastSR_.offset = RS_OFFSET;
    }
}

void NativeLeakInfo::FillQuotaInfo(Quota &quota, const ProcessResDetails &detail, uint32_t &processActions,
    const string name)
{
    // process_quotas->mem->resource_quota
    quota.value = static_cast<int>(detail.resource);
    auto actionIt = ACTION_MAP.find(detail.action);
    if (actionIt != ACTION_MAP.end()) {
        // process_quotas->mem->action
        quota.action = actionIt->second;
        processActions |= actionIt->second;
        if (detail.action == "clean") {
            quota.preCleanProcess = detail.preCleanProcess;
        }
    } else {
        HIVIEW_LOGE("process:%{public}s action unknown:%{public}s", name.c_str(), detail.action.c_str());
    }
    // process_quotas->mem->condition
    for (const auto &[conditionInfo, conditionMask] : CONDITION_MAP) {
        if (count(detail.condition.begin(), detail.condition.end(), conditionInfo) != 0) {
            quota.conditions |= conditionMask;
        }
    }
}

void NativeLeakInfo::UpdateQuota()
{
    map<string, vector<ProcessResDetails>> processQuota;
    ResourceQuotaManager::Instance().GetProcessQuota(RESTYPE_MEM_STR, processQuota);
    localMtx_.lock();
    for (const auto &elem : processQuota) {
        const string &name = elem.first;
        const vector<ProcessResDetails> &processResDetails = elem.second;
        unordered_map<QuotaAction, Quota> quotas;
        uint32_t processActions = 0;
        for (const auto &detail : processResDetails) {
            if (detail.resType != "mem") {
                continue;
            }
            Quota quota;
            FillQuotaInfo(quota, detail, processActions, name);
            quotas[quota.action] = move(quota);
        }
        if (quotas.size() > 0) {
            localThresholds_[name] = move(quotas);
            processActionMtx_.lock();
            processActionList_[name] = move(processActions);
            processActionMtx_.unlock();
        }
    }
    thresholdUpdateCnt_.fetch_add(1);
    localMtx_.unlock();
    NativeLeakInfo::GetThirdAppLimit();
    TryPrintQuotas();
}

void NativeLeakInfo::GetThirdAppLimit()
{
    map<string, int> thirdThersholdList;
    ThirdPartyConfig::GetInstance().GetThreshold(thirdThersholdList);

    localMtx_.lock();
    for (const auto &it : thirdThersholdList) {
        if (localThresholds_.count(it.first) != 0) {
            HIVIEW_LOGD("process:%{public}s has alreadly had threshold", it.first.c_str());
            continue;
        }
        Quota quota;
        unordered_map<QuotaAction, Quota> quotas;
        quota.value = static_cast<int>(ZoomThirdAppLimit(it.second));
        quota.action = ACTION_WARNING;
        quota.conditions |= CONDITION_BACKGROUND;
        quotas[ACTION_WARNING] = move(quota);
        localThresholds_[it.first] = move(quotas);

        HIVIEW_LOGD("process:%{public}s's softline:%{public}d, set its threshold: 2G",
            it.first.c_str(), quota.value);
        processActionMtx_.lock();
        processActionList_[it.first] = ACTION_WARNING;
        processActionMtx_.unlock();
    }
    thresholdUpdateCnt_.fetch_add(1);
    localMtx_.unlock();
}

uint64_t NativeLeakInfo::ZoomThirdAppLimit(uint32_t value)
{
    int64_t tmp = static_cast<int64_t>(value);
    return static_cast<uint64_t>((tmp / NATIVE_THRESHOLD_512M + 1) * NATIVE_THRESHOLD_512M);
}

void NativeLeakInfo::TryPrintQuotas()
{
    if (!DfrUtil::GetInstance().IsMemTestEnable()) {
        HIVIEW_LOGI("PrintQuotas is disable unless enable memleak test mode");
        return;
    }

    localMtx_.lock();
    const size_t len = localThresholds_.size();
    HIVIEW_LOGI("localThresholds size is %{public}zu", len);
    size_t index = 0;
    for (const auto &elem : localThresholds_) {
        index++;
        const string &name = elem.first;
        const unordered_map<QuotaAction, Quota> &quotas = elem.second;
        const size_t length = quotas.size();
        HIVIEW_LOGI("index:%{public}zu process:%{public}s has %{public}zu quota(s)", index, name.c_str(), length);
        int32_t num = 0;
        for (auto quota : quotas) {
            num++;
            HIVIEW_LOGI("quotas[%{public}d] vaule:%{public}d, action:%{public}" PRIu32 ",condictions:"
                "%{public}" PRIu32 ", preCleanProcess: %{public}s", num, quota.second.value, quota.second.action,
                quota.second.conditions, quota.second.preCleanProcess.c_str());
        }
    }
    localMtx_.unlock();
}

void NativeLeakInfo::SetVMMemory(int vmExtraThreshold)
{
    vmExtraThreshold_ = vmExtraThreshold * KB_PER_MB;
}

string NativeLeakInfo::GetPreCleanProcess()
{
    std::lock_guard<ffrt::mutex> lock(localMtx_);
    auto itProc = localThresholds_.find(processName_);
    if (itProc == localThresholds_.end()) {
        return "";
    }
    auto itQuota = itProc->second.find(ACTION_CLEAN);
    return itQuota == itProc->second.end() ? "" : itQuota->second.preCleanProcess;
}

int NativeLeakInfo::GetLocalLimit()
{
    if (updateCnt_ == thresholdUpdateCnt_.load()) {
        return localLimit_;
    }
    updateCnt_ = thresholdUpdateCnt_.load();
    int ret = INT_MAX;
    unordered_map<QuotaAction, Quota> quotas;
    localMtx_.lock();
    if (localThresholds_.count(processName_) != 0) {
        quotas = localThresholds_[processName_];
    } else if (localThresholds_.count(processType_) != 0) {
        quotas = localThresholds_[processType_];
    } else {
        SetProcessAction(ACTION_WARNING);
        HIVIEW_LOGE("There is no default threshold, current process: %{public}s, action:%{public}d,"
            " processType: %{public}s", processName_.c_str(), processAction_.load(), processType_.c_str());
    }
    localMtx_.unlock();

    uint32_t allAction = GetProcessAllAction();
    hardKillThreshold_ = (allAction & ACTION_KILL) ? quotas[ACTION_KILL].value : INT_MAX;
    softKillThreshold_ = (allAction & ACTION_SOFT_KILL) ? quotas[ACTION_SOFT_KILL].value : INT_MAX;
    if (allAction & ACTION_WARNING) {
        ret = quotas[ACTION_WARNING].value;
        SetProcessAction(ACTION_WARNING);
    } else if (allAction & ACTION_SOFT_KILL) {
        ret = quotas[ACTION_SOFT_KILL].value;
        SetProcessAction(ACTION_SOFT_KILL);
    } else if (allAction & ACTION_KILL) {
        ret = static_cast<int>(quotas[ACTION_KILL].value / KILL_DETECT_TRANS_PERCENT);
        SetProcessAction(ACTION_KILL);
    } else if (allAction & ACTION_CLEAN) {
        ret = quotas[ACTION_CLEAN].value;
        SetProcessAction(ACTION_CLEAN);
    } else {
        ret = NATIVE_THRESHOLD_2048M;
    }
    HIVIEW_LOGD("current process: %{public}s threshold:%{public}d action:%{public}d of allaction:%{public}u,"
        " processType: %{public}s",
        processName_.c_str(), ret, processAction_.load(), allAction, processType_.c_str());
    if (processName_ == "stratovirt" || processName_ == "virtiofsd") {
        if (vmExtraThreshold_ == 0) {
            HIVIEW_LOGW("vmExtraThreshold_ is 0, check if got hisysevent SA_VM_SET_MEMORY");
        }
        ret += vmExtraThreshold_;
    }
    localLimit_ = ret;
    return ret;
}

void NativeLeakInfo::SetAppLimit(int appLimit)
{
    appLimit_ = appLimit;
}

int NativeLeakInfo::GetAppLimit()
{
    return appLimit_;
}

int NativeLeakInfo::GetMemoryLimit()
{
    if (appLimit_ != INT_MAX) {
        return appLimit_;
    }
    int tmp = GetLocalLimit();
    return tmp;
}

bool NativeLeakInfo::IsInThresholdList() const
{
    std::lock_guard<ffrt::mutex> lock(localMtx_);
    return localThresholds_.count(processName_) != 0;
}

void NativeLeakInfo::SetIsAppendSmapsFile(bool flag)
{
    isAppendSmapsFile_ = flag;
}

bool NativeLeakInfo::GetIsAppendSmapsFile() const
{
    return isAppendSmapsFile_;
}

void NativeLeakInfo::SetIsDumpHiprofilerDrop(bool flag)
{
    isDumpHiprofilerDrop_ = flag;
}

bool NativeLeakInfo::GetIsDumpHiprofilerDrop() const
{
    return isDumpHiprofilerDrop_;
}

bool NativeLeakInfo::GetIsEverAppTelemetry() const
{
    return isEverAppTelemetry_;
}

SubLeakType NativeLeakInfo::GetTelemetrySubLeakType() const
{
    return telemetrySubLeakType_;
}

void NativeLeakInfo::SetTelemetrySubLeakType(SubLeakType subLeakType)
{
    telemetrySubLeakType_ = subLeakType;
}

void NativeLeakInfo::SetIsEverAppTelemetry(bool flag)
{
    isEverAppTelemetry_ = flag;
}

string NativeLeakInfo::GetSampleFileName() const
{
    string fileSampleName = "memleak-native-" + processName_ + "-" + to_string(pid_) + "-sample.txt";
    return fileSampleName;
}

string NativeLeakInfo::GetSampleFilePath()
{
    if (fileSamplePath_.empty()) {
        fileSamplePath_ = string(MEMORY_LEAK_PATH) + "/" + GetSampleFileName();
    }
    return fileSamplePath_;
}

string NativeLeakInfo::GetProfilerName() const
{
    string eventType;
    if (eventName_ == "ION_LEAK" || eventName_ == "GPU_LEAK") {
        eventType = "kernel";
    } else {
        eventType = "native";
    }
    string fileLogName = "memleak-" + eventType + "-" + processName_ + "-" + to_string(pid_) + "-" +
        leakedTime_ + ".txt";
    return fileLogName;
}

string NativeLeakInfo::GetProfilerPath() const
{
    return string(MEMORY_LEAK_PATH) + "/" + GetProfilerName();
}

string NativeLeakInfo::GetCopyProfilerName() const
{
    string eventType;
    if (eventName_ == "ION_LEAK" || eventName_ == "GPU_LEAK") {
        eventType = "kernel";
    } else {
        eventType = "native";
    }
    string fileLogName = "memleak-" + eventType + "-hiapp-" + processName_ + "-" + to_string(pid_) + "-" +
                         leakedTime_ + ".txt";
    return fileLogName;
}

string NativeLeakInfo::GetCopyProfilerPath() const
{
    return string(MEMORY_LEAK_PATH) + "/" + GetCopyProfilerName();
}

string NativeLeakInfo::GetRsMemPath()
{
    string fileRsMemPath = string(MEMORY_LEAK_PATH) + "/memleak-temp-" + processName_ +
        "-" + to_string(pid_) + "-rsMem.txt";
    return fileRsMemPath;
}

string NativeLeakInfo::GetRsGpuPath() const
{
    string fileRsGpuPath = string(MEMORY_LEAK_PATH) + "/memleak-temp-" + processName_ +
        "-" + to_string(pid_) + "-rsGpu.txt";
    return fileRsGpuPath;
}

string NativeLeakInfo::GetSmapsName() const
{
    string fileSmapsName = "memleak-native-" + processName_ + "-" + to_string(pid_) + "-smaps.txt";
    return fileSmapsName;
}

string NativeLeakInfo::GetGwpAsanName() const
{
    string fileSmapsName = "memleak-gwpasan-" + processName_ + "-" + to_string(pid_) + "-log-" + leakedTime_ + ".txt";
    return fileSmapsName;
}

string NativeLeakInfo::GetKernelPath()
{
    return string(MEMORY_LEAK_PATH) + "/memleak-kernel-" + processName_ + "-0-" + leakedTime_ + ".txt";
}

string NativeLeakInfo::GetKernelFileToApp() const
{
    return string(MEMORY_LEAK_PATH) + "/memleak-kernel-hiapp-" + processName_ + "-0-" + leakedTime_ + ".txt";
}

string NativeLeakInfo::GetSmapsPath() const
{
    if (processSourceInfo_.type == SYSLOAD_DETECT) {
        return string(SYSLOAD_DETECTOR_LEAK_PATH) +  "/" + GetSmapsName();
    }
    return string(MEMORY_LEAK_PATH) + "/" + GetSmapsName();
}

string NativeLeakInfo::GetGwpAsanPath() const
{
    return string(MEMORY_LEAK_PATH) + "/" + GetGwpAsanName();
}

string NativeLeakInfo::GetSmapsNameToApp() const
{
    string fileSmapsName = "memleak-native-hiapp-" + processName_ + "-" + to_string(pid_) + "-smaps.txt";
    return fileSmapsName;
}

string NativeLeakInfo::GetSmapsPathToApp() const
{
    return string(MEMORY_LEAK_PATH) + "/" + GetSmapsNameToApp();
}

string NativeLeakInfo::GetMediaMemName() const
{
    return "memleak-media-" + processName_ + "-" + to_string(pid_) + "-log-" + leakedTime_ + ".txt";
}

string NativeLeakInfo::GetMediaMemPath() const
{
    return string(MEMORY_LEAK_PATH) + "/" + GetMediaMemName();
}

string NativeLeakInfo::GetAvcMemName() const
{
    return "memleak-avc-" + processName_ + "-" + to_string(pid_) + "-log-" + leakedTime_ + ".txt";
}

string NativeLeakInfo::GetAvcMemPath() const
{
    return string(MEMORY_LEAK_PATH) + "/" + GetAvcMemName();
}

void NativeLeakInfo::SetSpecialPath(CommonDumpPathList &pathMap)
{
    for (auto &item : pathMap) {
        if (item.first & RENDER_SERVICE_MEM) {
            item.second = GetRsMemPath();
        } else if (item.first & GPU_PROCESS_INFO) {
            item.second = GetKernelPath();
        } else if (item.first & PROFILER) {
            item.second = GetProfilerPath();
        } else if (item.first & ION_PROCESS_INFO) {
            item.second = GetKernelPath();
        } else if (item.first & MEDIA_MEM) {
            item.second = GetMediaMemPath();
        } else if (item.first & AVC_MEM) {
            item.second = GetAvcMemPath();
        }
    }
}

string NativeLeakInfo::GetDumpFunction()
{
    const DumpFunctionMap &dumpFunctionMap = NativeLeakConfig::GetInstance().GetNativeConfig().dumpFunctionMap;
    string dumpFunction;
    map<string, DumpLogType> str2DumpLogType = {
        {"NO_LOG", NO_LOG},
        {"BASIC_LOG", BASIC_LOG},
        {"BASIC_AND_EXTERNAL_LOG", BASIC_AND_EXTERNAL_LOG},
        {"ION_LOG", ION_LOG},
        {"GPU_LOG", GPU_LOG},
        {"PSS_LOG", PSS_LOG},
        {"MEDIA_LOG", MEDIA_LOG},
        {"AV_CODEC_LOG", AV_CODEC_LOG},
    };

    for (const auto &it : dumpFunctionMap.defaultDumpFunctionMap) {
        if (allDumpLogType_ & str2DumpLogType.at(it.first)) {
            dumpFunction.append(it.second).append(",");
        }
    }
    if (dumpFunctionMap.specialDumpFunctionMap.count(processName_)) {
        dumpFunction.append(dumpFunctionMap.specialDumpFunctionMap.at(processName_));
    }
    HIVIEW_LOGI("process: %{public}s, dumpFunction: %{public}s", processName_.c_str(), dumpFunction.c_str());
    return dumpFunction;
}

DumperInfo NativeLeakInfo::GetDumperInfo()
{
    dumperInfo_.pid = pid_;
    dumperInfo_.processName = processName_;

    string dumpFunction = GetDumpFunction();
    dumperInfo_.dumpFunctions = dumpFunction;
    dumperInfo_.pathMap = DfrUtil::GetInstance().ParseDumpFunction(dumpFunction, GetSmapsPath());
    SetSpecialPath(dumperInfo_.pathMap);
    return dumperInfo_;
}

void NativeLeakInfo::GetLogPath(vector<string> &logPath)
{
    logPath.push_back("\"" + GetKernelFileToApp() + "\"");
    logPath.push_back("\"" + GetSmapsPathToApp() + "\"");
    logPath.push_back("\"" + GetCopyProfilerPath() + "\"");
}

bool NativeLeakInfo::IsEverReported(LeakGrade grade, unsigned int interval)
{
    std::lock_guard<ffrt::mutex> lock(reportedMtx_);
    auto processIt = reportedMap_.find(processName_);
    if (processIt == reportedMap_.end()) {
        return false;
    }
    // compare with same eventName, leakGrade
    auto eventIt = processIt->second.find(GetReportEventName());
    if (eventIt == processIt->second.end()) {
        return false;
    }
    time_t lastStamp = eventIt->second[grade];
    if (lastStamp <= 0) {
        return false;
    }

    time_t now = time(nullptr);
    if (now - lastLogStamp_ > SEC_PER_HOUR) {
        HIVIEW_LOGI("process: %{public}s has reported %{public}s at %{public}s", processName_.c_str(),
            LEAK_GRADE_STRING[grade], DfrUtil::GetInstance().TimeStampToStr(lastStamp).c_str());
        lastLogStamp_ = now;
    }
    return now <= lastStamp + static_cast<time_t>(interval);
}

bool NativeLeakInfo::IsXperfEventEverReported(unsigned int interval, LeakGrade leakGrade)
{
    reportedMtx_.lock();
    auto processIt = reportedMap_.find(processName_);
    if (processIt == reportedMap_.end()) {
        reportedMtx_.unlock();
        return false;
    }
    const auto &eventMap = processIt->second;
    time_t now = time(nullptr);
    bool isReported = false;
    time_t lastStampInfo = 0;
    time_t lastStampError = 0;
    time_t lastControlGradeStamp = 0;
    // 1.xperf event not care eventName 2.leak_error compare with leak_error, leak_control
    for (const auto &event : eventMap) {
        const std::vector<time_t> &stamps = event.second;
        lastStampInfo = stamps[LEAK_INFO];
        lastStampError = stamps[LEAK_ERROR];
        lastControlGradeStamp = stamps[LEAK_CONTROL];
        if (leakGrade == LEAK_INFO && lastStampInfo > 0 && now <= lastStampInfo + static_cast<time_t>(interval)) {
            isReported = true;
            HIVIEW_LOGI("process: %{public}s has reported %{public}s at %{public}s", processName_.c_str(),
                LEAK_GRADE_STRING[LEAK_INFO], DfrUtil::GetInstance().TimeStampToStr(lastStampInfo).c_str());
            break;
        }
        if (leakGrade == LEAK_ERROR && lastStampError > 0 && now <= lastStampError + static_cast<time_t>(interval)) {
            isReported = true;
            HIVIEW_LOGI("process: %{public}s has reported %{public}s at %{public}s", processName_.c_str(),
                LEAK_GRADE_STRING[LEAK_ERROR], DfrUtil::GetInstance().TimeStampToStr(lastStampError).c_str());
            break;
        }
        if (leakGrade == LEAK_CONTROL && lastControlGradeStamp > 0 &&
            now <= lastControlGradeStamp + static_cast<time_t>(interval)) {
            isReported = true;
            HIVIEW_LOGI("process: %{public}s has reported %{public}s at %{public}s", processName_.c_str(),
                LEAK_GRADE_STRING[LEAK_CONTROL], DfrUtil::GetInstance().TimeStampToStr(lastControlGradeStamp).c_str());
            break;
        }
    }
    reportedMtx_.unlock();
    return isReported;
}

void NativeLeakInfo::RecordReport()
{
    reportedMtx_.lock();
    std::string reportEventName = GetReportEventName();
    if (reportedMap_.count(processName_) == 0) {
        reportedMap_[processName_] = std::map<std::string, std::vector<time_t>>();
    }

    auto &eventMap = reportedMap_[processName_];
    if (eventMap.count(reportEventName) == 0) {
        eventMap[reportEventName] = std::vector<time_t>(LEAK_MAX, 0);
    }

    eventMap[reportEventName][leakGrade_.load()] = time(nullptr);
    reportedMtx_.unlock();
}

KillingState NativeLeakInfo::ShouldBeKilled()
{
    if (!CheckIfProcessAlive()) {
        HIVIEW_LOGI("process:%{public}s is died", processName_.c_str());
        return SHOULD_NOT_KILL;
    }
    if (!NativeLeakConfig::GetInstance().GetKillEnable() &&
        !NativeLeakConfig::GetInstance().GetServiceLeakControlParam().enableForceKill) {
        HIVIEW_LOGI("killEnable and forceKillEnable is disabled, donot kill process: %{public}s pid: %{public}d",
            processName_.c_str(), pid_);
        return SHOULD_NOT_KILL;
    }
    if (NativeLeakConfig::GetInstance().GetServiceLeakControlParam().enableForceKill &&
        DfrUtil::GetInstance().IsApp(pid_)) {
        HIVIEW_LOGI("forceKill is enable, but donot kill app process: %{public}s pid: %{public}d",
            processName_.c_str(), pid_);
        return SHOULD_NOT_KILL;
    }
    LeakGrade grade = leakGrade_.load();
    QuotaAction action = processAction_.load();
    if (OHOS::system::GetParameter("const.dfx.sub_health_recovery.enable", "") == "true" && grade >= LEAK_ERROR) {
        HIVIEW_LOGI(
            "process: %{public}s pid: %{public}d, leakGrade: %{public}s, sub_health_recovery is enable,try to kill",
            processName_.c_str(), pid_, LEAK_GRADE_STRING[grade]);
        return SHOULD_KILL;
    }
    if (grade < LEAK_CONTROL) {
        HIVIEW_LOGD("process: %{public}s pid: %{public}d, leakGrade: %{public}s is not enough, can not kill",
            processName_.c_str(), pid_, LEAK_GRADE_STRING[grade]);
        return SHOULD_NOT_KILL;
    }
    if (action & ACTION_WARNING) {
        HIVIEW_LOGD("process:%{public}s is App, and its action is soft kill", processName_.c_str());
        return SHOULD_NOT_KILL;
    }
    if (ShouldWaitKill()) {
        HIVIEW_LOGI("the process for kill: %{public}s is foreground: should not be killed!",
                    processName_.c_str());
        return SHOULD_WAIT_KILL;
    }
    return SHOULD_KILL;
}

bool NativeLeakInfo::ShouldWaitKill()
{
    localMtx_.lock();
    string temp = localThresholds_.count(processName_) == 0 ? processType_ : processName_;
    QuotaAction action = processAction_.load();
    if (FaultDetectorManager::CheckIfAppForeGround(processName_) &&
        !(localThresholds_[temp][action].conditions & CONDITION_FOREGROUND)) {
        localMtx_.unlock();
        HIVIEW_LOGI("process:%{public}s is foreground, add process into waitkillList", processName_.c_str());
        return true;
    }
    localMtx_.unlock();
    return false;
}

void NativeLeakInfo::PrintSampleHead(ofstream &fout)
{
    fout << "/*************************************************************" << endl;
    fout << "/*                    ***** READ ME *****                    *" << endl;
    fout << "/*************************************************************" << endl;
    fout << "/*    RSS: Resident Set Size                                 *" << endl;
    fout << "/*    PSS: Proportional Set Size                             *" << endl;
    fout << "/*    PSS = RSS + Offset                                     *" << endl;
    fout << "/*    TotalPSS = PSS + SwapPSS                               *" << endl;
    fout << "/*    TotalMem = TotalPSS + AvcMem + MediaMem + GPU + ION    *" << endl;
    fout << "/*                   ***** Two Modes *****                   *" << endl;
    fout << "/*    Estimate Mode: RSS & SwapPSS is real                   *" << endl;
    fout << "/*    Real Mode(Realtime with *): everything is real         *" << endl;
    fout << "/*                   ***** OTHER MEM *****                   *" << endl;
    fout << "/*    GPU: GPU mem of process                                *" << endl;
    fout << "/*    ION: ION mem of process                                *" << endl;
    fout << "/*    MediaMem: apply mem through media_service              *" << endl;
    fout << "/*    AvcMem: apply mem through avcodec_service              *" << endl;
    fout << "/*    ~ means negligible memory(safe to ignore in analysis)  *" << endl;
    fout << "/*************************************************************" << endl;
    fout << "/*                   ***** Attention *****                   *" << endl;
    fout << "/*    Formulas about TotalMem and sub-items may change,      *" << endl;
    fout << "/*    please reference current annotation formula            *" << endl;
    fout << "/*************************************************************" << endl << endl;

    fout << "pid:\t" << GetPid() << endl;
    fout << "processName:\t" << GetProcessName() << endl;
    fout << "SoftThreshold:\t" << GetMemoryLimit() << "(KB)" << endl;
    if (hardKillThreshold_ != INT_MAX) {
        fout << "HardThreshold:\t" << hardKillThreshold_ << "(KB)" << endl;
    }
    fout << left;
    fout << endl;
    fout << setw(WEIGHT_8)  << "Index";
    fout << setw(WEIGHT_12) << "RSS(KB)";
    fout << setw(WEIGHT_12) << "Offset(KB)";
    fout << setw(WEIGHT_12) << "PSS(KB)";
    fout << setw(WEIGHT_16) << "SwapPSS(KB)";
    fout << setw(WEIGHT_16) << "TotalPSS(KB)";
    fout << setw(WEIGHT_16) << "MediaMem(KB)";
    fout << setw(WEIGHT_16) << "AvcMem(KB)";
    fout << setw(WEIGHT_16) << "ION(KB)";
    fout << setw(WEIGHT_16) << "GPU(KB)";
    fout << setw(WEIGHT_16) << "TotalMem(KB)";
    fout << setw(WEIGHT_8)  << "Level";
    fout << setw(WEIGHT_16) << "RunningTime(s)";
    fout << "Realtime" << endl;
}

void NativeLeakInfo::PrintSample(std::ofstream &fout, const SampleRecord &sr)
{
    string mediaMemStr = GetMediaMem() ? to_string(GetMediaMem()) : "~";
    string avcMemStr = GetAvcMem() ? to_string(GetAvcMem()) : "~";
    string gpuMemStr = sr.gpu ? to_string(sr.gpu) : "~";
    string ionMemStr = sr.ion ? to_string(sr.ion) : "~";
    fout << left;
    fout << setw(WEIGHT_8)  << sr.index;
    fout << setw(WEIGHT_12) << sr.rss;
    fout << setw(WEIGHT_12) << sr.offset;
    fout << setw(WEIGHT_12) << sr.pss;
    fout << setw(WEIGHT_16) << sr.swappss;
    fout << setw(WEIGHT_16) << sr.totalPss;
    fout << setw(WEIGHT_16) << mediaMemStr;
    fout << setw(WEIGHT_16) << avcMemStr;
    fout << setw(WEIGHT_16) << ionMemStr;
    fout << setw(WEIGHT_16) << gpuMemStr;
    fout << setw(WEIGHT_16) << sr.totalMem;
    fout << setw(WEIGHT_8)  << SAMPLE_GRADE_SIGNAL[sr.level];
    fout << setw(WEIGHT_16) << sr.time;
    fout << sr.realTime;
    fout << endl;
}

void NativeLeakInfo::SetRealMemory(uint64_t memory)
{
    realMemoryInfo_.realMemory = memory;
    realMemoryInfo_.happenTime = DfrUtil::GetInstance().GetRealTime();
    if (memory > realMemoryInfo_.topMemory) {
        realMemoryInfo_.topMemory = memory;
    }
}

RealMemoryInfo NativeLeakInfo::GetRealMemory() const
{
    return realMemoryInfo_;
}

int32_t NativeLeakInfo::GetTotalMem() const
{
    return totalMem_;
}

void NativeLeakInfo::SetTotalMem(int32_t totalMem)
{
    totalMem_ = totalMem;
}

ProcessAnomalyInfo NativeLeakInfo::GetProcessInfoToKill()
{
    ProcessAnomalyInfo processInfoForKill;
    OHOS::XPowerManager::ProcessInfo procInfo;
    if (ProcessMgr::Instance().GetProcessInfoByName(processName_, procInfo) && procInfo.pid == pid_) {
        processInfoForKill.type = procInfo.subProcessType;
    } else {
        HIVIEW_LOGE("cannot get process info by name: %{public}s, continue", processName_.c_str());
        processInfoForKill.type = UNSPECIFIED_PROCESS_TYPE;
    }
    unordered_set<string> invalidNameList = { "appspawn", "apppool" };
    processInfoForKill.processName = invalidNameList.count(processName_) != 0 ?
        DfrUtil::GetInstance().GetProcessName(pid_) : processName_;
    localMtx_.lock();
    string temp = localThresholds_.count(processName_) == 0 ? processType_ : processName_;
    QuotaAction action = processAction_.load();
    processInfoForKill.resource = static_cast<int64_t>(localThresholds_[temp][action].value);
    processInfoForKill.uid = DfrUtil::GetInstance().GetProcessUid(pid_);
    processInfoForKill.pid = pid_;
    if (processSourceInfo_.type == STANDBY_DETECT) {
        processInfoForKill.resType = "standby_clean";
        processInfoForKill.condition.emplace_back(ResourceQuotaCtl::CONDITION_STANDBY);
    } else if (KILL_TYPE_MAP.count(GetEventName()) != 0) {
        processInfoForKill.resType = KILL_TYPE_MAP.at(GetEventName());
    } else {
        processInfoForKill.resType = "mem";
    }
    processInfoForKill.isDefaultBaseline = (localThresholds_.count(processName_) == 0);
    bool memcgReport = processSourceInfo_.type == MEMMGR_EVENT_DETECT;
    processInfoForKill.isMemcgReport = memcgReport;
    SampleRecord lastSR = GetLastSampleRecord();
    processInfoForKill.totalMem = memcgReport ? totalMem_ : lastSR.totalMem;
    FillProcessMemInfo(processInfoForKill, lastSR);
    if (action == ACTION_SOFT_KILL) {
        processInfoForKill.action = GetEventName() == "PSS_MEMORY" ? "softKill" : "kill";
    } else if (action == ACTION_KILL) {
        processInfoForKill.action = "kill";
    }
    if (localThresholds_[temp][action].conditions == 0) {
        processInfoForKill.condition.emplace_back(0);
        localMtx_.unlock();
        return processInfoForKill;
    }
    if (localThresholds_[temp][action].conditions & CONDITION_FOREGROUND) {
        processInfoForKill.condition.emplace_back(ResourceQuotaCtl::CONDITION_FOREGROUND);
    } else if (localThresholds_[temp][action].conditions & CONDITION_BACKGROUND) {
        processInfoForKill.condition.emplace_back(ResourceQuotaCtl::CONDITION_BACKGROUND);
    }
    localMtx_.unlock();
    return processInfoForKill;
}

void NativeLeakInfo::FillProcessMemInfo(ProcessAnomalyInfo &processInfoForKill, const SampleRecord &lastSR)
{
    processInfoForKill.totalPss = lastSR.totalPss;
    processInfoForKill.ion = lastSR.ion;
    processInfoForKill.gpu = lastSR.gpu;
}

void NativeLeakInfo::SetEventName(std::string eventName)
{
    eventName_ = eventName;
}

std::string NativeLeakInfo::GetEventName()
{
    return eventName_;
}

bool NativeLeakInfo::ShouldSample(time_t now)
{
    if (now - lastSampleStamp_ < sampleInterval_) {
        return false;
    }
    lastSampleStamp_ = now;
    return true;
}

int NativeLeakInfo::GetSmallThresholdByGrade(int softBaseline, LeakGrade grade)
{
    int ret = INT_MAX;
    switch (grade) {
        case LEAK_INFO:
            ret = softBaseline;
            break;
        case LEAK_WARNING:
            ret = softBaseline + INFO_WARN_SMALL_DIVIDER;
            break;
        case LEAK_ERROR:
            ret = softBaseline + WARN_ERROR_SMALL_DIVIDER;
            break;
        case LEAK_CONTROL:
            ret = softBaseline + ERROR_CONTROL_SMALL_DIVIDER;
            break;
        default:
            break;
    }
    return ret;
}

int NativeLeakInfo::GetLargeThresholdByGrade(int softBaseline, LeakGrade grade)
{
    int ret = INT_MAX;
    int thresholdAbs = INT_MAX;
    int thresholdRel = INT_MAX;
    switch (grade) {
        case LEAK_INFO:
            thresholdAbs = softBaseline;
            thresholdRel = softBaseline;
            break;
        case LEAK_WARNING:
            thresholdAbs = softBaseline + INFO_WARN_LARGE_DIVIDER;
            thresholdRel = softBaseline * INFO_WARN_PCT_DIVIDER;
            break;
        case LEAK_ERROR:
            thresholdAbs = softBaseline + WARN_ERROR_LARGE_DIVIDER;
            thresholdRel = softBaseline * WARN_ERROR_PCT_DIVIDER;
            break;
        case LEAK_CONTROL:
            thresholdAbs = softBaseline + ERROR_CONTROL_LARGE_DIVIDER;
            thresholdRel = softBaseline * ERROR_CONTROL_PCT_DIVIDER;
            break;
        default:
            break;
    }
    if (processSourceInfo_.type == XPERF_EVENT_DETECT) {
        return thresholdRel;
    }
    ret = max(thresholdAbs, thresholdRel);
    return ret;
}

int NativeLeakInfo::GetNativeControlThreshold(int softBaseline)
{
    if (softBaseline < NATIVE_THRESHOLD_512M) {
        HIVIEW_LOGD("process:%{public}s:%{public}d's softline:%{public}d, set its control threshold: 1G",
            processName_.c_str(), pid_, softBaseline);
        return NATIVE_THRESHOLD_1024M;
    }

    if ((softBaseline * ERROR_CONTROL_PCT_DIVIDER) < NATIVE_THRESHOLD_2048M) {
        HIVIEW_LOGD("process:%{public}s:%{public}d's softline:%{public}d, set its control threshold: 2G",
            processName_.c_str(), pid_, softBaseline);
        return NATIVE_THRESHOLD_2048M;
    }

    return (softBaseline * ERROR_CONTROL_PCT_DIVIDER);
}

int NativeLeakInfo::GetThresholdByGrade(LeakGrade grade)
{
    int ret = INT_MAX;
    int temp = GetMemoryLimit();
    if (temp == INT_MAX) {
        HIVIEW_LOGE("GetMemoryLimit failed");
        return INT_MAX;
    }
    int softBaseline = temp;
    if (grade == LEAK_CONTROL && !IsApp()) {
        return GetNativeControlThreshold(softBaseline);
    }
    if (softBaseline <= SMALL_LARGE_DIVIDER) {
        return GetSmallThresholdByGrade(softBaseline, grade);
    }
    return GetLargeThresholdByGrade(softBaseline, grade);
}

int NativeLeakInfo::GetCurThreshold()
{
    LeakGrade grade = leakGrade_.load();
    if (grade == NO_LEAK) {
        grade = LEAK_INFO;
    }
    return GetThresholdByGrade(grade);
}

LeakGrade NativeLeakInfo::GetNextGrade(LeakGrade leakGrade)
{
    LeakGrade nextGrade;
    switch (leakGrade) {
        case NO_LEAK:
        case LEAK_INFO:
            nextGrade = LEAK_WARNING;
            break;
        case LEAK_WARNING:
            nextGrade = LEAK_ERROR;
            break;
        case LEAK_ERROR:
            nextGrade = LEAK_CONTROL;
            break;
        case LEAK_CONTROL:
        default:
            nextGrade = LEAK_CONTROL;
            HIVIEW_LOGD("process:%{public}s:%{public}d's curGrade:%{public}s is too big",
                processName_.c_str(), pid_, LEAK_GRADE_STRING[leakGrade_.load()]);
            break;
    }
    return nextGrade;
}

int NativeLeakInfo::GetNextThreshold()
{
    LeakGrade nextGrade = GetNextGrade(leakGrade_.load());

    return GetThresholdByGrade(nextGrade);
}

time_t NativeLeakInfo::GetBaseInterval()
{
    if (!IsInThresholdList()) {
        return SAMPLE_INTERVAL_DEFAULT;
    }

    int baseThreshold = GetThresholdByGrade(LEAK_WARNING);
    if (baseThreshold < SMALL_THRESHOLD_LINE) {
        return SAMPLE_INTERVAL_LONG;
    } else if (baseThreshold < BIG_THRESHOLD_LINE) {
        return SAMPLE_INTERVAL_NORMAL;
    }
    return SAMPLE_INTERVAL_SHORT;
}

void NativeLeakInfo::UpdateSampleInterval()
{
    time_t baseInterval = GetBaseInterval();
    time_t finalInterval;
    int lastTotal = lastSR_.totalMem;
    int curThreshold = GetCurThreshold();
    int nextThreshold = GetNextThreshold();
    int slowLine = ((nextThreshold - curThreshold) / 2) + curThreshold;

    if (isdumping_.load()) {
        finalInterval = baseInterval * INTERVAL_RATIO_DOUBLE;
    } else if (leakGrade_.load() >= LEAK_CONTROL) {
        finalInterval = baseInterval * INTERVAL_RATIO_DOUBLE;
    } else if (lastTotal < slowLine) {
        finalInterval = baseInterval * INTERVAL_RATIO_DOUBLE;
    } else if (lastTotal < nextThreshold) {
        finalInterval = baseInterval;
    } else {
        finalInterval = baseInterval / INTERVAL_RATIO_DOUBLE;
    }

    if (DfrUtil::GetInstance().IsMemTestEnable()) {
        finalInterval /= INTERVAL_RATIO_TEST;
    }
    sampleInterval_ = finalInterval;
}

bool NativeLeakInfo::IsOverHardKillThreshold()
{
    if (hardKillThreshold_ != INT_MAX && lastSR_.totalMem > hardKillThreshold_) {
        HIVIEW_LOGI("process:%{public}s:%{public}d, totalMem: %{public}d is over kill hardthreshold: %{public}d",
            processName_.c_str(), pid_, lastSR_.totalMem, hardKillThreshold_);
        return true;
    }
    return false;
}

bool NativeLeakInfo::IsOverSoftKillThreshold()
{
    if (softKillThreshold_ != INT_MAX && (lastSR_.totalMem / SOFT_KILL_OVERLOAD_RATIO) > softKillThreshold_) {
        HIVIEW_LOGI("process:%{public}s:%{public}d, totalMem: %{public}d is double over kill softthreshold: %{public}d",
            processName_.c_str(), pid_, lastSR_.totalMem, softKillThreshold_);
        SetLeakGrade(LEAK_CONTROL);
        SetProcessAction(ACTION_SOFT_KILL);
        return true;
    }
    return false;
}

bool NativeLeakInfo::IsOverHardKillThresholdReally()
{
    if (!IsOverHardKillThreshold()) {
        return false;
    }
    GetPss();
    if (!IsOverHardKillThreshold()) {
        return false;
    }
    SetLeakGrade(LEAK_CONTROL);
    SetProcessAction(ACTION_KILL);
    return true;
}

int NativeLeakInfo::CalculateFittedValue(int localLimit)
{
    return pow(localLimit / KB_PER_MB, EXPONENT) * MULTIPLIER;
}

void NativeLeakInfo::DecideEventType(int totalMem)
{
    HIVIEW_LOGI("process: %{public}s, totalMem: %{public}d", processName_.c_str(), totalMem);
    int localLimit = GetLocalLimit();
    if (localLimit > NATIVE_THRESHOLD_1536M) {
        if (totalMem > localLimit * RATE_OF_ONE_POINT_FIVE_GB) {
            ReportMemoryLeakEvent();
        }
        return;
    }
    if ((totalMem / KB_PER_MB) > CalculateFittedValue(localLimit)) {
        ReportMemoryLeakEvent();
    }
    // default: report to memory_leak_wp
}

bool NativeLeakInfo::GetPss()
{
    string traceStr(__func__);
    traceStr.append(":").append(processName_).append("_").append(to_string(pid_));
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    HIVIEW_LOGI("process:%{public}s:%{public}d ready to get real PSS", processName_.c_str(), pid_);
    SampleRecord newSR;
    if (!DfrUtil::GetInstance().CallSmapsRollup(pid_, newSR.pss, newSR.swappss, newSR.rss)) {
        HIVIEW_LOGE("%{public}s:%{public}d get pss failed", processName_.c_str(), pid_);
        return false;
    }
    newSR.gpu = DfrUtil::GetInstance().GetProcessGpuMemory(pid_);
    newSR.ion = GetLatestIon();
    AddRealMemoryRecord(newSR);
    return true;
}

void NativeLeakInfo::AddRealMemoryRecord(SampleRecord &newSR)
{
    SampleRecord lastSR = lastSR_;
    newSR.index = lastSR.index + 1;
    newSR.offset = newSR.pss - newSR.rss;
    newSR.totalPss = newSR.pss + newSR.swappss;
    newSR.totalMem = newSR.totalPss + GetAvcMem() + GetMediaMem() + newSR.gpu + newSR.ion;
    int targetThreshold = GetNextThreshold();
    if (IsApp() && newSR.totalMem <= NATIVE_THRESHOLD_2560M &&
        FaultDetectorManager::CheckIfAppForeGround(GetProcessName()) &&
        !DfrUtil::GetInstance().SkipForegroundDetectlimit()) {
        newSR.level = NO_LEAK;
        newSR.overCnt = 0;
    } else if (newSR.totalMem > targetThreshold) {
        newSR.level = GetNextGrade(leakGrade_.load());
        newSR.overCnt = lastSR.overCnt >= 1 ? lastSR.overCnt + 1 : 1;
    } else {
        newSR.level = lastSR.level;
        newSR.overCnt = lastSR.overCnt <= -1 ? lastSR.overCnt - 1 : -1;
    }
    newSR.time = DfrUtil::GetInstance().GetRunningMonotonicTime();
    newSR.realTime = "*" + DfrUtil::GetInstance().GetRealTime();
    SetRealMemory(newSR.totalMem);
    AddMemoryRecord(newSR);
    UpdateSampleInterval();
    DecideReportEvent(newSR);
}

void NativeLeakInfo::DecideReportEvent(SampleRecord &newSR)
{
    DecideEventType(newSR.totalMem);
    std::map<int, const string> eventMap = {
        {newSR.gpu, "GPU_LEAK"},
        {newSR.ion, "ION_LEAK"},
        {newSR.totalPss, "PSS_MEMORY"},
        {GetMediaMem(), "MEDIA_MEMORY"},
        {GetAvcMem(), "AVC_MEMORY"}
    };
    auto maxEntry = eventMap.rbegin();
    SetEventName(maxEntry->second);
}

bool NativeLeakInfo::GetFakePss()
{
    string traceStr(__func__);
    traceStr.append(":").append(processName_).append("_").append(to_string(pid_));
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    int rss = static_cast<int>(DfrUtil::GetInstance().GetProcessRss(pid_));
    int swappss = DfrUtil::GetInstance().GetSwapPssByPid(pid_);
    if (swappss == -1 || rss == 0) {
        HIVIEW_LOGE("get fake pss failed, process: %{public}s:%{public}d", processName_.c_str(), pid_);
        return false;
    }
    SampleRecord lastSR = lastSR_;
    SampleRecord newSR;
    newSR.index = lastSR.index + 1;
    newSR.rss = rss;
    newSR.swappss = swappss;
    newSR.gpu = DfrUtil::GetInstance().GetProcessGpuMemory(pid_);
    newSR.ion = GetLatestIon();
    newSR.offset = lastSR.offset;
    newSR.pss = rss + newSR.offset;
    newSR.totalPss = newSR.pss + newSR.swappss;
    newSR.totalMem = newSR.totalPss + GetAvcMem() + GetMediaMem() + newSR.gpu + newSR.ion;
    int targetThreshold = GetNextThreshold();
    if (IsApp() && newSR.totalMem <= NATIVE_THRESHOLD_2560M &&
        FaultDetectorManager::CheckIfAppForeGround(GetProcessName()) &&
        !DfrUtil::GetInstance().SkipForegroundDetectlimit()) {
        newSR.level = NO_LEAK;
        newSR.overCnt = 0;
    } else if (newSR.totalMem > targetThreshold) {
        newSR.level = GetNextGrade(leakGrade_.load());
        newSR.overCnt = lastSR.overCnt >= 1 ? lastSR.overCnt + 1 : 1;
    } else {
        newSR.level = lastSR.level;
        newSR.overCnt = lastSR.overCnt <= -1 ? lastSR.overCnt - 1 : -1;
    }
    newSR.time = DfrUtil::GetInstance().GetRunningMonotonicTime();
    newSR.realTime = DfrUtil::GetInstance().GetRealTime();
    AddMemoryRecord(newSR);
    UpdateSampleInterval();
    return true;
}

void NativeLeakInfo::AddMemoryRecord(const SampleRecord &record)
{
    lastSR_ = record;
    if (record.overCnt <= OVER_CNT_LAST_PRINT) {
        HIVIEW_LOGD("%{public}s:%{public}d, overCnt:%{public}d is too small, do not print",
            processName_.c_str(), pid_, record.overCnt);
        return;
    }
    if (!DfrUtil::GetInstance().IsMemTestEnable() && !isEverPrintSample_ && record.overCnt < 0) {
        HIVIEW_LOGD("%{public}s:%{public}d pss always less than threshold, do not print", processName_.c_str(), pid_);
        return;
    }

    ofstream fout;
    if (!isEverPrintSample_) {
        OldLogCleaner::GetInstance().GetLogFile(SAMPLE, this, fout);
        if (!fout.is_open()) {
            HIVIEW_LOGE("failed to open file: %{public}s", GetSampleFilePath().c_str());
            return;
        }
        isEverPrintSample_ = true;
        PrintSampleHead(fout);
        fout.close();
    }
    OldLogCleaner::GetInstance().GetLogFile(SAMPLE, this, fout, IS_FILE_OPEN_APPEND);
    if (!fout.is_open()) {
        HIVIEW_LOGE("failed to append file: %{public}s", GetSampleFilePath().c_str());
        return;
    }
    PrintSample(fout, record);
    fout.close();
}

SampleRecord NativeLeakInfo::GetLastSampleRecord()
{
    return lastSR_;
}

bool NativeLeakInfo::SetLeakGradeByPss()
{
    int lastTotal = lastSR_.totalMem;
    LeakGrade newGrade = NO_LEAK;
    if (lastTotal > GetThresholdByGrade(LEAK_CONTROL)) {
        newGrade = LEAK_CONTROL;
    } else if (lastTotal > GetThresholdByGrade(LEAK_ERROR)) {
        newGrade = LEAK_ERROR;
    } else if (lastTotal > GetThresholdByGrade(LEAK_WARNING)) {
        newGrade = LEAK_WARNING;
    }
    LeakGrade curGrade = leakGrade_.load();
    if (newGrade <= curGrade) {
        HIVIEW_LOGE("process: %{public}s:%{public}d lastGrade:%{public}s, newGrade:%{public}s",
            processName_.c_str(), pid_, LEAK_GRADE_STRING[curGrade], LEAK_GRADE_STRING[newGrade]);
        return false;
    }
    HIVIEW_LOGI("process: %{public}s:%{public}d pss:%{public}d, lastGrade:%{public}s, newGrade:%{public}s",
        processName_.c_str(), pid_, lastTotal, LEAK_GRADE_STRING[curGrade], LEAK_GRADE_STRING[newGrade]);
    leakGrade_.store(newGrade);
    return true;
}

QuotaAction NativeLeakInfo::GetProcessAction()
{
    return processAction_.load();
}

void NativeLeakInfo::SetProcessAction(QuotaAction action)
{
    processAction_.store(action);
}

uint32_t NativeLeakInfo::GetProcessAllAction()
{
    uint32_t processAction = 0;
    processActionMtx_.lock();
    if (processActionList_.count(processName_) != 0) {
        processAction = processActionList_[processName_];
        processActionMtx_.unlock();
        return processAction;
    }
    if (processActionList_.count(processType_) != 0) {
        processAction = processActionList_[processType_];
        processActionMtx_.unlock();
        return processAction;
    }
    // none action
    processActionMtx_.unlock();
    return NO_ACTION;
}

bool NativeLeakInfo::IsUseDefaultThreshold()
{
    processActionMtx_.lock();
    if (processActionList_.count(processName_) != 0) {
        processActionMtx_.unlock();
        return false;
    }
    processActionMtx_.unlock();
    return true;
}

void NativeLeakInfo::SetAvcMem(int32_t splitMemory)
{
    avcMem_.store(splitMemory);
}

int32_t NativeLeakInfo::GetAvcMem()
{
    return avcMem_.load();
}

void NativeLeakInfo::SetMediaMem(int32_t splitMemory)
{
    mediaMem_.store(splitMemory);
}

int32_t NativeLeakInfo::GetMediaMem()
{
    return mediaMem_.load();
}

uint64_t NativeLeakInfo::GetTelemetryType()
{
    if (telemetryType_ > 0) {
        return telemetryType_;
    }
    if (GetTelemetry().CouldTelemetry(EXT_PSS_LEAK_TYPE, GetBundleName())
        && GetLeakSource() == LeakSource::PSS) {
        telemetryType_ = EXT_PSS_LEAK_TYPE;
    } else if (GetTelemetry().CouldTelemetry(RSS_LEAK_TYPE, GetBundleName())
               && GetLeakSource() == LeakSource::RSS) {
        telemetryType_ = RSS_LEAK_TYPE;
    } else {
        map<string, uint64_t> telemetryTypeMap {
            { "PSS_MEMORY", NATIVE_LEAK_TYPE },
            { "GPU_LEAK", GPU_LEAK_TYPE },
        };
        telemetryType_ = telemetryTypeMap[eventName_];
    }
    return telemetryType_;
}

bool NativeLeakInfo::CouldTelemetry()
{
    uint64_t telemetryType = GetTelemetryType();
    return GetTelemetry().CouldTelemetry(telemetryType, GetBundleName());
}

bool NativeLeakInfo::InTelemetry()
{
    uint64_t telemetryType = GetTelemetryType();
    return GetTelemetry().InTelemetry(telemetryType, GetBundleName());
}

struct EventQuota NativeLeakInfo::GetEventQuota()
{
    uint64_t telemetryType = GetTelemetryType();
    return GetTelemetry().GetEventQuota(telemetryType, GetBundleName());
}

void NativeLeakInfo::ConsumeQuota()
{
    uint64_t telemetryType = GetTelemetryType();
    GetTelemetry().ConsumeQuota(telemetryType, GetBundleName());
}

bool NativeLeakInfo::ShouldDumpTrace(const string &bundleName, unsigned int intervalSec)
{
    dumpedTraceMtx_.lock();
    time_t now = DfrUtil::GetInstance().GetRunningMonotonicTime();
    for (auto it = dumpedTraceMap_.begin(); it != dumpedTraceMap_.end();) {
        if (now > it->second + static_cast<time_t>(intervalSec)) {
            it = dumpedTraceMap_.erase(it);
        } else {
            ++it;
        }
    }
    bool shouldDump = (dumpedTraceMap_.find(bundleName) == dumpedTraceMap_.end());
    dumpedTraceMtx_.unlock();
    return shouldDump;
}

void NativeLeakInfo::RecordDumpTrace(const string &bundleName)
{
    dumpedTraceMtx_.lock();
    dumpedTraceMap_[bundleName] = DfrUtil::GetInstance().GetRunningMonotonicTime();
    dumpedTraceMtx_.unlock();
}

bool NativeLeakInfo::ShouldDump()
{
    bool isAllowDump = DfrUtil::GetInstance().IsBetaOrDevelopMode();
    LeakGrade leakGrade = GetLeakGrade();
    if (!isAllowDump && leakGrade < LEAK_ERROR) {
        return false;
    }
    if (isAllowDump && leakGrade < LEAK_WARNING) {
        return false;
    }
    return true;
}

void NativeLeakInfo::SetAllDumpLogType(uint32_t allDumpLogType)
{
    allDumpLogType_ = allDumpLogType;
}

uint32_t NativeLeakInfo::GetAllDumpLogType()
{
    return allDumpLogType_;
}

void NativeLeakInfo::ClearHiappLog()
{
    DfrUtil::GetInstance().RemoveFile(GetKernelFileToApp());
    DfrUtil::GetInstance().RemoveFile(GetSmapsPathToApp());
    DfrUtil::GetInstance().RemoveFile(GetCopyProfilerPath());
}

void NativeLeakInfo::SetProcessSourceInfo(ProcessSourceInfo processSourceInfo)
{
    processSourceInfo_ = processSourceInfo;
}

ProcessSourceInfo NativeLeakInfo::GetProcessSourceInfo()
{
    return processSourceInfo_;
}

bool NativeLeakInfo::IsInServiceLeakWhiteList(int &threshold)
{
    localMtx_.lock();
    auto processIt = localThresholds_.find(processName_);
    if (processIt == localThresholds_.end()) {
        localMtx_.unlock();
        return false;
    }
    const auto &actionMap = processIt->second;
    // quota standby action is clean
    auto quotaIt = actionMap.find(ACTION_CLEAN);
    if (quotaIt == actionMap.end()) {
        localMtx_.unlock();
        return false;
    }
    const Quota &quota = quotaIt->second;
    if (quota.conditions & CONDITION_STANDBY) {
        threshold = quota.value;
        localMtx_.unlock();
        return true;
    }
    localMtx_.unlock();
    return false;
}

bool NativeLeakInfo::ServiceLeakCheck(int threshold)
{
    if (!GetPss()) {
        return false;
    }
    uint64_t totalMem = GetRealMemory().realMemory;
    if (totalMem <= static_cast<uint64_t>(threshold)) {
        return false;
    }
    HIVIEW_LOGI("processName: %{public}s, pid: %{public}d, totalMem: %{public}ld, threshold: %{public}d",
        processName_.c_str(), pid_, totalMem, threshold);
    // Quota mem->condition contain standby, action actually is clean, but send the ability manager service
    // a kill action
    SetProcessAction(ACTION_KILL);
    return true;
}

void NativeLeakInfo::SetLatestIon(int value)
{
    latestIon_.store(value);
}

int NativeLeakInfo::GetLatestIon() const
{
    return latestIon_.load();
}

string NativeLeakInfo::GetShouldKillWithProcess()
{
    return shouldKillWithProcess_;
}

void NativeLeakInfo::SetShouldKillWithProcess(const string &shouldKillWithProcess)
{
    shouldKillWithProcess_ = shouldKillWithProcess;
}

#ifdef MEMORY_GROWTH_ANALYSER_ENABLE
AnalyzeSample NativeLeakInfo::GetLastAnalyzeSample()
{
    return lastAS_;
}

void NativeLeakInfo::UpdateAnalyzeSample(const AnalyzeSample &newAS)
{
    lastAS_ = newAS;
}
#endif

bool NativeLeakInfo::CheckProfilerFinish()
{
    if (GetDumpLogType() != BASIC_AND_EXTERNAL_LOG) {
        // no need check
        return true;
    }
    const string &logFilePath = GetProfilerPath();
    const string &realLogFile = DfrUtil::GetInstance().RealPath(logFilePath);
    const string &logProfilerPath = GetCopyProfilerPath();
    const string &realProfilerPath = DfrUtil::GetInstance().RealPath(logProfilerPath);
    if (realLogFile.empty()) {
        HIVIEW_LOGE("failed realpath file: %{public}s", logFilePath.c_str());
        return false;
    }
    struct stat profilerFileInfo;
    if (stat(realLogFile.c_str(), &profilerFileInfo) != 0) {
        HIVIEW_LOGE("get snapShot file stat failed");
        return false;
    }
    HIVIEW_LOGI("profilerFile size: %{public}ld", profilerFileInfo.st_size);
    if (profilerFileInfo.st_size < PROFILER_SIZE_BASELINE) {
        return false;
    }
    if (ShouldBeKilled() >= SHOULD_WAIT_KILL && IsApp()) {
        if (!DfrUtil::GetInstance().CopyFileNoReal(realLogFile, realProfilerPath)) {
            HIVIEW_LOGW("failed to copy profiler file from %{public}s to %{public}s",
                        realLogFile.c_str(), realProfilerPath.c_str());
        }
    }
    return true;
}

FaultStateType NativeLeakInfo::GetNextStateForDump()
{
    bool profilerFinish = CheckProfilerFinish();
    HIVIEW_LOGI("CheckProfilerFinish result is: %{public}s", profilerFinish ? "true" : "false");
    if (GetIsDumpHiprofilerDrop() ||
            (GetProcessName() != RENDER_SERVICE_PROCESS && !profilerFinish)) {
        if (ShouldBeKilled() >= SHOULD_WAIT_KILL) {
            return PROC_REMOVAL_STATE;
        } else {
            return PROC_FINISHED_STATE;
        }
    }
    if (IsEverReported(leakGrade_.load(), REPEAT_REPORT_INTERVAL)) {
        return PROC_REMOVAL_STATE;
    }
    return PROC_REPORT_STATE;
}

FaultStateType NativeLeakInfo::GetNextStateForReport()
{
    LeakGrade grade = leakGrade_.load();
    bool isSubHealth = OHOS::system::GetParameter("const.dfx.sub_health_recovery.enable", "") == "true" &&
                grade >= LEAK_ERROR;
    if (IsApp() && isSubHealth) {
        return PROC_APP_REPORT_STATE;
    }
    if (isSubHealth) {
        return PROC_REMOVAL_STATE;
    }
    if (IsApp() && grade >= LEAK_CONTROL) {
        return PROC_APP_REPORT_STATE;
    }
    // for service processes
    // kill if leak grade >= LEAK_CONTROL during common detect;
    // do not kill if in blocklist during service leak detect, otherwise kill;
    if (!IsApp()) {
        if (GetProcessSourceInfo().type != SERVICE_LEAK_DETECT) {
            if (grade >= LEAK_CONTROL) {
                return PROC_REMOVAL_STATE;
            }
            return PROC_FINISHED_STATE;
        }
        if (InServiceLeakBlackList()) {
            return PROC_FINISHED_STATE;
        }
        return PROC_REMOVAL_STATE;
    }
    if (grade >= LEAK_CONTROL) {
        return PROC_REMOVAL_STATE;
    }
    return PROC_FINISHED_STATE;
}

/*
This function is the main function for state transition.
must condation is ready, then init the transition rules.
condition:
1、state < dumpState and process died ---every time needed，directly return finished

2、in wating list ---Required when newly leak detected
3、is ever report ---Required when newly leak detected
4、leak grade ---According to actual needs

ever report:
     yes:   dump_kill-> removal -> appReportKill -> reportOverLimit -> finish
     no:    dump -> report -> appReport -> removal -> appReportKill -> reportOverLimit -> finish

*/

FaultStateType NativeLeakInfo::MaySwitchToRemovalState()
{
    if (!CheckIfProcessAlive()) {
        return PROC_FINISHED_STATE;
    }
    return PROC_REMOVAL_STATE;
}

FaultStateType NativeLeakInfo::GetNextState()
{
    FaultStateType curState = GetState();
    if (curState < PROC_DUMP_STATE && !CheckIfProcessAlive()) {
        return PROC_FINISHED_STATE;
    }
    switch (curState) {
        // dump
        case PROC_DUMP_STATE: {
            return GetNextStateForDump();
        }
        // dumpkill
        case PROC_DUMP_KILL_STATE: {
            return MaySwitchToRemovalState();
        }
        // report
        case PROC_REPORT_STATE: {
            return GetNextStateForReport();
        }
        case PROC_REPORT_OVERLIMIT_STATE: {
            return PROC_FINISHED_STATE;
        }
        // appReport
        case PROC_APP_REPORT_STATE: {
            LeakGrade grade = leakGrade_.load();
            if (grade >= LEAK_CONTROL) {
                return MaySwitchToRemovalState();
            }
            if (OHOS::system::GetParameter("const.dfx.sub_health_recovery.enable", "") == "true" &&
                    grade >= LEAK_ERROR) {
                return MaySwitchToRemovalState();
            }
            return PROC_FINISHED_STATE;
        }
        case PROC_APP_REPORT_KILL_STATE: {
            return PROC_REPORT_OVERLIMIT_STATE;
        }
        // remove
        case PROC_REMOVAL_STATE: {
            if (CheckIfProcessAlive() || (GetProcessSourceInfo().type == SERVICE_LEAK_DETECT)) {
                return PROC_FINISHED_STATE;
            }
            return PROC_APP_REPORT_KILL_STATE;
        }
        default:
            return PROC_FINISHED_STATE;
    }
}

bool NativeLeakInfo::InServiceLeakBlackList()
{
    string blockListStr = NativeLeakConfig::GetInstance().GetServiceLeakControlParam().blockList;
    vector<string> notNeedKillList = DfrUtil::GetInstance().Split(blockListStr, ',');
    bool inBlackList = false;
    for (const auto& blackName : notNeedKillList) {
        // processName is truncated
        if (blackName.find(processName_) != string::npos) {
            inBlackList = true;
            break;
        }
    }
    return inBlackList;
}

void NativeLeakInfo::DumpArkTsRawHeapInfo(std::string telemetryType)
{
    HIVIEW_LOGI("begin dump arkts rawheap info");
    OHOS::AppExecFwk::JsHeapDumpInfo info;
    info.pid = static_cast<uint32_t>(pid_);
    info.needSnapshot = true;
    info.tid = static_cast<uint32_t>(pid_);
    info.needBinary = true;
    if (!DfrUtil::GetInstance().DumpRawheapInfo(info)) {
        HIVIEW_LOGE("DumpRawheapInfo failed");
        return;
    }
    int32_t ret = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, EVENT_NAME_ARK_STATS, HiSysEvent::EventType::FAULT,
        "PID", pid_,
        "TID", pid_,
        "PROCESS_NAME", processName_,
        "APP_RUNNING_ID", GetAppRunningId(),
        "TYPE", telemetryType);
    if (ret != 0) {
        HIVIEW_LOGW("HiSysEventWrite failed, ret=%{public}d", ret);
    }
    RecordReport();
    SetEverSendTelemetryEvent(true);
}

void NativeLeakInfo::FillExtPssInfo()
{
    extPssInfo_ = DetailInfoParser::GetInstance().GetExtPssParseInfo(pid_, "EXT_PSS_LEAK", SmapsStatType::PSS_TYPE);
}

DetailExtPssInfo NativeLeakInfo::GetExtPssInfo()
{
    return extPssInfo_;
}

uint32_t NativeLeakInfo::GetMainPidByRenderProc()
{
    // check if render process by processName
    const std::string suffix = ":render";
    bool isRenderValid = processName_.length() > suffix.length() &&
        processName_.substr(processName_.length() - suffix.length()) == suffix;
    if (!isRenderValid) {
        HIVIEW_LOGI("process: %{public}s:%{public}d, is not render process", processName_.c_str(), pid_);
        return static_cast<uint32_t>(pid_);
    }
    // renderProc name -> mainProc name -> mainPid
    std::string mainProcName = processName_.substr(0, processName_.length() - suffix.length());
    uint32_t mainPid = NativeLeakDetector::GetInstance().GetPidByProcessNameWithoutLock(mainProcName);
    HIVIEW_LOGD("renderPid=%{public}d, renderProcName=%{public}s, mainPid=%{public}d, mainProcName=%{public}s",
        pid_, processName_.c_str(), mainPid, mainProcName.c_str());
    if (mainPid > 0) {
        return mainPid;
    }
    return static_cast<uint32_t>(pid_);
}

void NativeLeakInfo::DumpRawHeapMemoryForThirdParty(std::string telemetryType, AppExecFwk::MemDumpType dumpType)
{
    HIVIEW_LOGI("%{public}s, begin dump %{public}d rawheap memory", telemetryType.c_str(), dumpType);
    OHOS::AppExecFwk::MemDumpInfo info;
    info.needDump = true;
    info.needRaw = true;
    info.needGc = false;
    if (dumpType == AppExecFwk::MemDumpType::ARKWEB_JS) {
        info.renderPid = static_cast<uint32_t>(pid_);
        info.pid = GetMainPidByRenderProc();
    } else if (dumpType == AppExecFwk::MemDumpType::JSVM) {
        info.pid = static_cast<uint32_t>(pid_);
    } else if (dumpType == AppExecFwk::MemDumpType::KMP_KOTLIN) {
        // kmp dump use default param
        info.needDump = false;
        info.needRaw = false;
        info.needGc = false;
        info.pid = static_cast<uint32_t>(pid_);
    } else {
        return;
    }
    info.dumpType = dumpType;
    DfrUtil::GetInstance().DumpHeapMemory(info);
    HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, EVENT_NAME_ARK_STATS, HiSysEvent::EventType::FAULT,
        "PID", info.pid,
        "TID", info.pid,
        "PROCESS_NAME", processName_,
        "APP_RUNNING_ID", GetAppRunningId(),
        "TYPE", telemetryType);
    RecordReport();
    SetEverSendTelemetryEvent(true);
}

LeakSource NativeLeakInfo::GetLeakSource() const
{
    return leakSource_;
}

void NativeLeakInfo::SetLeakSource(LeakSource source)
{
    leakSource_ = source;
}
} // namespace HiviewDFX
} // namespace OHOS
