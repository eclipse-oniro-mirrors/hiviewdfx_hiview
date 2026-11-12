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

#include "kernel_leak_info.h"
#include "fault_common.h"
#include "fault_detector_manager.h"
#include "kernel_leak_config.h"
#include "hitrace_meter.h"
#include "telemetry_manager.h"

using namespace std;
using namespace OHOS::XPowerManager;

namespace {
enum {
    GPU_PROCESS_PID = 2
};

enum {
    ASHMEM_PROCESS_NAME = 0,
    ASHMEM_PROCESS_PID = 1,
    ASHMEM_VIRTUAL_SIZE = 6,
    ASHMEM_MAGIC = 8
};

enum {
    ION_PROCESS_NAME = 0,
    ION_PROCESS_PID = 1,
    ION_SIZE = 3,
    ION_MAGIC = 4
};

const map<string, int> ashmemElementPosition = {
    {"processName", ASHMEM_PROCESS_NAME},
    {"pid", ASHMEM_PROCESS_PID},
    {"virtualSize", ASHMEM_VIRTUAL_SIZE},
    {"magic", ASHMEM_MAGIC}
};

const map<string, int> ionElementPosition = {
    {"processName", ION_PROCESS_NAME},
    {"pid", ION_PROCESS_PID},
    {"virtualSize", ION_SIZE},
    {"magic", ION_MAGIC}
};

const unordered_set<string> shouldKillList = {
    "ion",
    "vma_leak",
    "gpu",
    "ashmem",
    "kernel_zone",
};

const std::unordered_set<std::string> reportTypeByCountVirtualSize = {
    "ashmem",
    "ion",
    "gpu"
};

constexpr int MAX_RECORD_NUM = 30;
constexpr int MB_PER_GB = 1024;
constexpr int B_PER_KB = 1024;
constexpr int KB_PER_MB = 1024;
constexpr int KB_PER_GB = 1024 * MB_PER_GB;
constexpr uint64_t REPORT_MEMORY_LEAK_LIMIT = 1.5 * KB_PER_GB;
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "KernelLeakInfo");

ffrt::mutex KernelLeakInfo::reportedMtx_;
std::map<std::string, std::vector<time_t>> KernelLeakInfo::reportedList_;
ffrt::mutex KernelLeakInfo::dumpedTraceMtx_;
std::map<std::string, time_t> KernelLeakInfo::dumpedTraceMap_;
ffrt::mutex KernelLeakInfo::localMtx_;
// <proc ecsstype, <(softkill hardkill), Quota>>  ashmem ion gpu
unordered_map<string, unordered_map<string, Quota>> KernelLeakInfo::localThresholds_;

KernelLeakInfo::KernelLeakInfo(string name)
    : MemoryLeakInfoBase(0, name)
{
    sampleMemory_.resize(MAX_RECORD_NUM);
    cpuTime_.resize(MAX_RECORD_NUM);
    realTime_.resize(MAX_RECORD_NUM);
    dumperInfo_.detectorType = KERNEL_LEAK_DETECTOR;
}

KernelLeakInfo::KernelLeakInfo(pid_t pid, string name)
    : MemoryLeakInfoBase(pid, name)
{
    sampleMemory_.resize(MAX_RECORD_NUM);
    cpuTime_.resize(MAX_RECORD_NUM);
    realTime_.resize(MAX_RECORD_NUM);
    dumperInfo_.detectorType = KERNEL_LEAK_DETECTOR;
    GetAppRunningId();
    GetUid();
}

KernelLeakInfo::~KernelLeakInfo() {}

void KernelLeakInfo::UpdateQuota(const string &processType)
{
    map<string, vector<ProcessResDetails>> processQuota;
    ResourceQuotaManager::Instance().GetProcessQuota(processType, processQuota);
    const size_t len = processQuota.size();
    HIVIEW_LOGD("processQuota size is %{public}zu", len);
    for (const auto &elem : processQuota) {
        string name = elem.first;
        if (name != APP_DEFAULT && name != NATIVE_DEFAULT) {
            continue;
        }
        HIVIEW_LOGD("current procecc name: %{public}s", name.c_str());
        const vector<ProcessResDetails> &processResDetails = elem.second;
        for (const auto &detail : processResDetails) {
            if (detail.resType != "ashmem" && detail.resType != "ion" && detail.resType != "gpu") {
                continue;
            }
            Quota quota;
            if (!PasteToQuota(quota, detail)) {
                HIVIEW_LOGE("Leak type:%{public}s action unknown:%{public}s",
                    detail.resType.c_str(), detail.action.c_str());
            }
            UpdateThresholds(name, detail.resType, quota);
        }
    }
    TryPrintQuotas();
}

void KernelLeakInfo::UpdateThresholds(const string &name, const string &resType, Quota &quota)
{
    std::lock_guard<ffrt::mutex> lock(localMtx_);
    if (name == APP_DEFAULT && quota.action == ACTION_SOFT_KILL) {
        localThresholds_[resType]["app_kill"] = move(quota);
    }
    if (name == APP_DEFAULT && quota.action == ACTION_KILL) {
        localThresholds_[resType]["app_hard_kill"] = move(quota);
    }
    if (name == NATIVE_DEFAULT) {
        localThresholds_[resType]["native_kill"] = move(quota);
    }
}

bool KernelLeakInfo::PasteToQuota(Quota &quota, const ProcessResDetails &detail)
{
    // killthreshold
    quota.value = static_cast<int>(detail.resource);
    // softkill or hardkill
    if (detail.action == "softKill") {
        quota.action = ACTION_SOFT_KILL;
    } else if (detail.action == "kill") {
        quota.action = ACTION_KILL;
    } else {
        return false;
    }
    // kill on foreground || kill on background
    if (count(detail.condition.begin(), detail.condition.end(), ResourceQuotaCtl::CONDITION_FOREGROUND) != 0) {
        quota.conditions |= CONDITION_FOREGROUND;
    }
    if (count(detail.condition.begin(), detail.condition.end(), ResourceQuotaCtl::CONDITION_BACKGROUND) != 0) {
        quota.conditions |= CONDITION_BACKGROUND;
    }
    return true;
}

void KernelLeakInfo::TryPrintQuotas()
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
        const unordered_map<string, Quota> &quotas = elem.second;
        const size_t length = quotas.size();
        HIVIEW_LOGI("index:%{public}zu process:%{public}s has %{public}zu quota(s)", index, name.c_str(), length);
        for (auto quota : quotas) {
            HIVIEW_LOGI("quotas[%{public}s] vaule:%{public}d, action:%{public}" PRIu32 ", condictions:"
                "%{public}" PRIu32, quota.first.c_str(), quota.second.value,
                quota.second.action, quota.second.conditions);
        }
    }
    localMtx_.unlock();
}

bool KernelLeakInfo::SetKillThreshold()
{
    int appKillThreshold = INT_MAX;
    int appHardKillThreshold = INT_MAX;
    int nativeKillThreshold = INT_MAX;
    localMtx_.lock();
    if (localThresholds_.count(processName_) != 0) {
        if (localThresholds_[processName_].count("app_kill") != 0) {
            appKillThreshold = localThresholds_[processName_]["app_kill"].value;
        }
        if (localThresholds_[processName_].count("app_hard_kill") != 0) {
            appHardKillThreshold = localThresholds_[processName_]["app_hard_kill"].value;
        }
        if (localThresholds_[processName_].count("native_kill") != 0) {
            nativeKillThreshold = localThresholds_[processName_]["native_kill"].value;
        }
    } else {
        HIVIEW_LOGD("There is no threshold from rss, please use local threshold");
        localMtx_.unlock();
        return false;
    }
    localMtx_.unlock();

    SetAppMemoryLimit(appKillThreshold);
    SetAppMemoryHardLimit(appHardKillThreshold);
    SetNativeMemoryLimit(nativeKillThreshold);
    return true;
}

int KernelLeakInfo::GetTopMemory() const
{
    auto it = max_element(sampleMemory_.begin(), sampleMemory_.end());
    if (it == sampleMemory_.end()) {
        return 0;
    }
    return *it;
}

void KernelLeakInfo::AddMemory(int memory)
{
    sampleMtx_.lock();

    sampleMemory_[tail_] = memory;
    cpuTime_[tail_] = DfrUtil::GetInstance().GetRunningMonotonicTime();
    realTime_[tail_] = DfrUtil::GetInstance().GetRealTime();
    tail_ = GetNextIndex(tail_);
    if (head_ == tail_) {
        head_ = GetNextIndex(head_);
    }

    sampleMtx_.unlock();
}

void KernelLeakInfo::PrintSample(ofstream &fout)
{
    sampleMtx_.lock();

    if (head_ == tail_) {
        sampleMtx_.unlock();
        return;
    }

    int index = 0;
    for (size_t i = head_; i != tail_; i = GetNextIndex(i), index++) {
        fout << setw(LONG_WEIGHT) << index;
        fout << setw(LONG_WEIGHT) << cpuTime_[i];
        fout << setw(LONG_WEIGHT) << sampleMemory_[i];
        fout << setw(LONG_WEIGHT) << realTime_[i] << endl;
    }

    sampleMtx_.unlock();
}

int KernelLeakInfo::GetLastSample()
{
    return sampleMemory_[GetPervIndex(tail_)];
}

void KernelLeakInfo::PrintSample(string &str)
{
    static string longWeight(LONG_WEIGHT, ' ');
    sampleMtx_.lock();

    if (head_ == tail_) {
        sampleMtx_.unlock();
        return;
    }

    int index = 0;
    for (size_t i = head_; i != tail_; i = GetNextIndex(i), index++) {
        str.append(to_string(index)).append(longWeight);
        str.append(to_string(cpuTime_[i])).append(longWeight);
        str.append(to_string(sampleMemory_[i])).append(longWeight);
        str.append(realTime_[i]).append("\n");
    }

    sampleMtx_.unlock();
}

size_t KernelLeakInfo::GetSampleLength()
{
    sampleMtx_.lock();
    if (head_ == tail_) {
        sampleMtx_.unlock();
        return 0;
    }
    if (head_ < tail_) {
        size_t tmp = tail_ - head_;
        sampleMtx_.unlock();
        return tmp;
    }
    sampleMtx_.unlock();
    return tail_ - head_ + MAX_RECORD_NUM;
}

void KernelLeakInfo::SetMeminfoName(const string &name)
{
    meminfoName_ = name;
}

const string &KernelLeakInfo::GetMeminfoName() const
{
    return meminfoName_;
}

void KernelLeakInfo::SetProcName(const string &name)
{
    procName_ = name;
}

const string &KernelLeakInfo::GetProcName() const
{
    return procName_;
}

void KernelLeakInfo::SetReportInterval(time_t interval)
{
    reportInterval_ = interval;
}

const string KernelLeakInfo::GetSystemHeapPath() const
{
    return "/sys/kernel/debug/dmaheap/heaps/system_heap";
}

const string KernelLeakInfo::GetIonNewProcPath() const
{
    return "/proc/process_dmabuf_info";
}

string KernelLeakInfo::GetSampleFilePath()
{
    return string(MEMORY_LEAK_PATH) + "/memleak-kernel-" + GetProcessNameForReport() + "-0-sample.txt";
}

string KernelLeakInfo::GetRsMemPath()
{
    return string(MEMORY_LEAK_PATH) + "/memleak-temp-" + GetProcessNameForReport() + "-0-rsMem.txt";
}

string KernelLeakInfo::GetLogName() const
{
    return "/memleak-kernel-" + GetProcessNameForReport() + "-0-" + leakedTime_ + ".txt";
}

string KernelLeakInfo::GetLogPath() const
{
    return string(MEMORY_LEAK_PATH) + GetLogName();
}

string KernelLeakInfo::GetProfilerName() const
{
    return "/memleak-kernel-" + GetProcessNameForReport() + "-" + to_string(GetProcessPidForReport()) + "-" +
        leakedTime_ + ".txt";
}

string KernelLeakInfo::GetProfilerPath() const
{
    return string(MEMORY_LEAK_PATH) + GetProfilerName();
}

string KernelLeakInfo::GetLogFileToApp()
{
    return string(MEMORY_LEAK_PATH) + "/memleak-kernel-hiapp-" + GetProcessNameForReport() + "-0-" +
        leakedTime_ + ".txt";
}

string KernelLeakInfo::GetProcessSmapsPath()
{
    return "/proc/" + to_string(GetProcessPidForReport()) + "/smaps";
}

void KernelLeakInfo::GetLogPath(std::vector<std::string> &logPath)
{
    logPath.push_back("\"" + GetLogFileToApp() + "\"");
    logPath.push_back("\"" + GetCopyProfilerPath() + "\"");
}

string KernelLeakInfo::GetProcFilePath()
{
    return DfrUtil::GetInstance().GetProcPath(meminfoName_, procName_);
}

string KernelLeakInfo::GetUiTreeName()
{
    return "/memleak-kernel-uitree-" + GetProcessNameForReport() + "-" + to_string(GetProcessPidForReport()) + ".txt";
}

string KernelLeakInfo::GetUiTreePath()
{
    return string(MEMORY_LEAK_PATH) + GetUiTreeName();
}

string KernelLeakInfo::GetRenameUiTreePath()
{
    return string(MEMORY_LEAK_PATH) + "/memleak-kernel-uitree-" + GetProcessNameForReport() + "-" +
           to_string(GetProcessPidForReport()) + "-" + leakedTime_ + ".txt";
}

string KernelLeakInfo::GetRsTreeName()
{
    return "/memleak-kernel-rstree-" + GetProcessNameForReport() + "-" + to_string(GetProcessPidForReport()) + "-" +
           leakedTime_ + ".txt";
}

string KernelLeakInfo::GetRsTreePath()
{
    return string(MEMORY_LEAK_PATH) + GetRsTreeName();
}

string KernelLeakInfo::GetSendRsTreePath()
{
    return rsTreePath_;
}

void KernelLeakInfo::SetSendRsTreePath(const string &rsTreePath)
{
    rsTreePath_ = rsTreePath;
}

bool KernelLeakInfo::IsMemoryLeak(size_t judgeLength)
{
    int topMemory = GetTopMemory();
    int hardLeakThreshold = GetHardMemoryLimit();
    if (topMemory >= hardLeakThreshold) {
        SetIsTriggerHardThreshold(true);
        SetLeakGrade(LEAK_ERROR);
        return true;
    }

    size_t sampleLength = GetSampleLength();
    if (sampleLength < judgeLength) {
        return false;
    }

    sampleMtx_.lock();

    int softThreshold = GetMemoryLimit();
    size_t cnt = 0;
    for (size_t i = GetPervIndex(tail_); cnt < judgeLength; i = GetPervIndex(i), cnt++) {
        if (sampleMemory_[i] <= softThreshold) {
            sampleMtx_.unlock();
            return false;
        }
    }
    sampleMtx_.unlock();
    return true;
}

bool KernelLeakInfo::IsEverReported()
{
    std::lock_guard<ffrt::mutex> lock(reportedMtx_);
    string name = processName_ + "_" + GetProcessNameForReport();
    auto it = reportedList_.find(name);
    if (it == reportedList_.end()) {
        return false;
    }
    // compare with same leakGrade
    auto &eventMap = it->second;
    int64_t lastReportTime = eventMap[leakGrade_.load()];
    if (lastReportTime < 0) {
        return false;
    }

    int64_t now = time(nullptr);
    if (now > lastReportTime + reportInterval_) {
        HIVIEW_LOGI("process: %{public}s should report, last reporttime is %{public}s", name.c_str(),
            DfrUtil::GetInstance().TimeStampToStr(lastReportTime).c_str());
    } else {
        HIVIEW_LOGI("process: %{public}s should not report, last reporttime is %{public}s", name.c_str(),
            DfrUtil::GetInstance().TimeStampToStr(lastReportTime).c_str());
    }
    return now <= lastReportTime + reportInterval_;
}

void KernelLeakInfo::RecordReport()
{
    std::lock_guard<ffrt::mutex> lock(reportedMtx_);
    if (processName_ == "gpu_rs") {
        HIVIEW_LOGI("gpu_rs should not record in blocklist");
        return;
    }
    string name = processName_ + "_" + GetProcessNameForReport();
    int64_t now = time(nullptr);
    auto &eventMap = reportedList_[name];
    if (eventMap.size() != LEAK_MAX) {
        eventMap = std::vector<time_t>(LEAK_MAX, 0);
    }
    LeakGrade grade = leakGrade_.load();
    if (eventMap[grade] > 0 && now <= eventMap[grade] + reportInterval_) {
        HIVIEW_LOGI("process %{public}s has already reported, skipping record time.", name.c_str());
        return;
    }
    HIVIEW_LOGI("kernel: save process %{public}s in blocklist_ at %{public}s", name.c_str(),
        DfrUtil::GetInstance().TimeStampToStr(now).c_str());
    eventMap[grade] = now;
}

DumperInfo KernelLeakInfo::GetDumperInfo()
{
    dumperInfo_.pid = GetProcessPidForReport();
    dumperInfo_.processName = GetProcessNameForReport();
    dumperInfo_.dumpFunctions = KernelLeakConfig::GetInstance().GetDumpFunction(processName_);
    if (dumperInfo_.processName == "render_service") {
        dumperInfo_.dumpFunctions.append(",rd");
    }
    dumperInfo_.pathMap = DfrUtil::GetInstance().ParseDumpFunction(dumperInfo_.dumpFunctions, GetLogPath());
    return dumperInfo_;
}

void KernelLeakInfo::HandleIonOverBaseProcessesList()
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    vector<OverBaseProcesses> overBaseProcessList;
    string infoStr = DfrUtil::GetInstance().GetAllProcessIonInfo();
    vector<string> pairList = DfrUtil::GetInstance().Split(infoStr, ',');
    for (const auto& item : pairList) {
        // pid:valueMem
        OverBaseProcesses newProcess;
        size_t pos = item.find(':');
        if (pos == string::npos) {
            continue;
        }
        string pidStr = item.substr(0, pos);
        if (!DfrUtil::GetInstance().SafeStoi(pidStr, newProcess.pid)) {
            continue;
        }
        string ionvalue = item.substr(pos + 1);
        if (!DfrUtil::GetInstance().ConvertToULL(ionvalue, newProcess.virtualSize)) {
            continue;
        }
        newProcess.virtualSize *= BYTE_PER_KB;

        newProcess.processName = DfrUtil::GetInstance().GetProcessName(newProcess.pid);
        overBaseProcessList.push_back(newProcess);
    }
    HandleProcessList(overBaseProcessList);
}

void KernelLeakInfo::GetMaxProcessFromProcFile(const string &finBuffer)
{
    if (appMemoryLimit_.load() == INT_MAX && nativeMemoryLimit_.load() == INT_MAX) {
        return;
    }
    stringstream finBufferStream(finBuffer);
    if (processName_.empty()) {
        HIVIEW_LOGI("processName_ is empty");
        return;
    }
    if (processName_ == "ashmem") {
        HIVIEW_LOGI("GET INTO ASHMEM FILE INFO DUMP");
        CountOverBaseProcess(finBufferStream, ashmemElementPosition, '\t');
    }
    if (processName_ == "ion") {
        HIVIEW_LOGI("GET INTO ION FILE INFO DUMP");
        HandleIonOverBaseProcessesList();
    }
    if (processName_ == "gpu") {
        HIVIEW_LOGI("GET INTO GPU FILE INFO DUMP");
        string startStr = "summary";
        CountGpuOverBaseProcess(finBufferStream, startStr);
    }
    OverBaseProcesses processInfo = GetProcessForReport();
    processInfo.uid = DfrUtil::GetInstance().GetProcessUid(processInfo.pid);
    SetAppRunningId(DfrUtil::GetInstance().GetAppRunningUniqueIdByPid(processInfo.pid));
    SetProcessForReport(processInfo);
}

void KernelLeakInfo::CountGpuOverBaseProcess(stringstream &finBufferStream, const string startStr)
{
    string line;
    vector<OverBaseProcesses> overBaseProcessList;
    while (getline(finBufferStream, line)) {
        auto idx = line.find(startStr);
        if (idx == string::npos) {
            continue;
        }
        OverBaseProcesses newProcess;
        if (!FetchGpuOverProcess(line, newProcess)) {
            HIVIEW_LOGI("GPU: FetchGpuOverProcess failed");
            continue;
        }
        SetProcessType(newProcess);
        overBaseProcessList.push_back(newProcess);
    }
    HandleProcessList(overBaseProcessList);
}

void KernelLeakInfo::CountGpuRsOverBaseProcess(stringstream &finBufferStream, const string startStr)
{
    string line;
    OverBaseProcesses overBaseProcess;
    vector<OverBaseProcesses> overBaseProcessList;

    while (finBufferStream >> line) {
        size_t pos = line.find(startStr);
        if (pos == string::npos) {
            continue;
        }
        if (!DfrUtil::GetInstance().SafeStoi(line.substr(0, pos), overBaseProcess.pid)) {
            HIVIEW_LOGE("safastoi failed. line: %{public}s", line.c_str());
            continue;
        }
        overBaseProcess.processName = DfrUtil::GetInstance().GetProcessName(overBaseProcess.pid);
        if (!DfrUtil::GetInstance().ConvertToULL(line.substr(pos + 1), overBaseProcess.virtualSize)) {
            continue;
        }
        overBaseProcess.processType = GPU_RS;
        overBaseProcessList.push_back(overBaseProcess);
    }
    HandleProcessList(overBaseProcessList);
}

bool KernelLeakInfo::FetchGpuOverProcess(const string line, OverBaseProcesses &overBaseProcess)
{
    string usedSummaryStr = "summary:";
    vector<string> elementInfo;
    if (!DfrUtil::GetInstance().GetElementInfo(line, elementInfo, ' ') ||
        (elementInfo.size() <= GPU_PROCESS_PID)) {
        return false;
    }
    if (!DfrUtil::GetInstance().SafeStoi(elementInfo[GPU_PROCESS_PID], overBaseProcess.pid)) {
        HIVIEW_LOGE("safestoi failed. line: %{public}s", line.c_str());
        return false;
    }
    overBaseProcess.processName = DfrUtil::GetInstance().GetProcessName(overBaseProcess.pid);
    auto indexUsedSummaryStart = line.find(usedSummaryStr);
    if (indexUsedSummaryStart == string::npos) {
        return false;
    }
    auto indexUsedSummayEnd = line.find(" ", indexUsedSummaryStart);
    if (indexUsedSummayEnd == string::npos) {
        indexUsedSummayEnd = line.size();
    }
    indexUsedSummaryStart += usedSummaryStr.size();
    if (indexUsedSummayEnd <= indexUsedSummaryStart) {
        return false;
    }
    string virtualSizeStr = line.substr(indexUsedSummaryStart, indexUsedSummayEnd - indexUsedSummaryStart);
    if (!DfrUtil::GetInstance().ConvertToULL(virtualSizeStr, overBaseProcess.virtualSize)) {
        return false;
    }
    return true;
}

void KernelLeakInfo::CountOverBaseProcess(stringstream &finBufferStream,
    const map<string, int> &elementPosition, const char interval)
{
    string line;
    string startLine = "magic";
    int startRead = false;
    OverBaseProcesses currentProcess;
    vector<OverBaseProcesses> processList;
    map<string, string> magicList;
    while (getline(finBufferStream, line) && !line.empty()) {
        if (line.find("Total") != string::npos || line.find("-----------") != string::npos) {
            continue;
        }
        if (startRead) {
            vector<string> elementInfo;
            if (!DfrUtil::GetInstance().GetElementInfo(line, elementInfo, interval) ||
                static_cast<int>(elementInfo.size()) <= elementPosition.at("magic")) {
                continue;
            }
            string processName = elementInfo[elementPosition.at("processName")];
            string processPid = elementInfo[elementPosition.at("pid")];
            string processVirtualSize = elementInfo[elementPosition.at("virtualSize")];
            string magic = elementInfo[elementPosition.at("magic")];
            OverBaseProcesses newProcess;
            newProcess.processName = processName;
            if (!DfrUtil::GetInstance().SafeStoi(processPid, newProcess.pid)) {
                HIVIEW_LOGE("safestoi failed. line: %{public}s", line.c_str());
                continue;
            }
            if (!DfrUtil::GetInstance().ConvertToULL(processVirtualSize, newProcess.virtualSize)) {
            continue;
            }
            newProcess.magic = magic;
            if (IsProcessNameAppeared(magicList, processList, newProcess, currentProcess)) {
                continue;
            }
            processList.push_back(currentProcess);
            currentProcess = newProcess;
        }
        if (line.find(startLine) != string::npos) {
            startRead = true;
        }
    }
    processList.push_back(currentProcess);
    HandleProcessList(processList);
}

bool KernelLeakInfo::IsProcessNameAppeared(map<string, string> &magicList, vector<OverBaseProcesses> &processList,
                                           OverBaseProcesses &newProcess, OverBaseProcesses &currentProcess)
{
    if (newProcess.processName == "render_service") {
        newProcess.magicType = RENDER_SERVICE_PROCESS_TYPE;
    }
    if (newProcess.processName == "composer_host") {
        newProcess.magicType = COMPOSER_HOST_PROCESS_TYPE;
    }
    if (!magicList.count(newProcess.magic)) {
        magicList[newProcess.magic] = newProcess.processName;
        if (currentProcess.processName != newProcess.processName) {
            return false;
        }
        currentProcess.virtualSize += newProcess.virtualSize;
        return true;
    }
    if (currentProcess.processName == magicList[newProcess.magic]) {
        if (currentProcess.magicType >= newProcess.magicType) {
            return true;
        }
        currentProcess.virtualSize -= newProcess.virtualSize;
        magicList[newProcess.magic] = newProcess.processName;
        return false;
    }
    for (auto itor = processList.begin(); itor != processList.end(); itor++) {
        if (itor->processName == magicList[newProcess.magic]) {
            if (itor->magicType >= newProcess.magicType) {
                return true;
            }
            itor->virtualSize -= newProcess.virtualSize;
            SetProcessType(*itor);
            magicList[newProcess.magic] = newProcess.processName;
            if (currentProcess.processName == newProcess.processName) {
                currentProcess.virtualSize += newProcess.virtualSize;
                return true;
            }
            return false;
        }
    }
    return true; // magic read secondly
}

void KernelLeakInfo::SetProcessType(OverBaseProcesses &overBaseProcess)
{
    int virtualSizeKB = static_cast<int>(overBaseProcess.virtualSize / BYTE_PER_KB);
    int memTotal = DfrUtil::GetInstance().GetRamSize();
    // not normal info
    if (virtualSizeKB / MB_PER_GB / KB_PER_MB > memTotal) {
        overBaseProcess.processType = ABNORMAL_PROCESS;
        return;
    }
    SetUid(overBaseProcess.pid);
    if (IsApp()) {
        if (virtualSizeKB > appMemoryLimit_.load()) {
            overBaseProcess.processType = APP_PROCESS;
        }
        if (virtualSizeKB > appMemoryHardLimit_.load()) {
            overBaseProcess.conditionType = KILLING_FOREGROUND_BACKGROUND;
        }
        return;
    }
    if (virtualSizeKB > nativeMemoryLimit_.load()) {
        overBaseProcess.processType = NATIVE_PROCESS;
        return;
    }
    overBaseProcess.processType = NOT_OVER_BASELINE;
}

void KernelLeakInfo::HandleProcessList(vector<OverBaseProcesses> &processList)
{
    vector<OverBaseProcesses> overBaseProcessList;
    if (!processList.size()) {
        return;
    }
    MayAddFreeStateProcess(processList);
    sort(processList.begin(), processList.end(), JudgeProcessSize);
    processInfoForReport_ = "\nAll the process info:\n";
    string processInfo;
    for (auto itor = processList.begin(); itor != processList.end(); itor++) {
        if (itor->pid == -1 || itor->virtualSize == 0) {
            continue;
        }
        SetProcessType(*itor);
        itor->processName = DfrUtil::GetInstance().GetProcessName(itor->pid);
        if (itor->processType > ABNORMAL_PROCESS) {
            overBaseProcessList.push_back(*itor);
        }
        processInfo = "processname: " + itor->processName +", pid: " + to_string(itor->pid) +
        ", type:" +  PROCESS_TYPE_STRING[itor->processType] + ", size: " + to_string(itor->virtualSize) + "\n";
        processInfoForReport_ += processInfo;
    }
    maxProcessForReport_ = processList[0];

    processInfo = "The max process is:" + maxProcessForReport_.processName + ", pid: " +
        to_string(maxProcessForReport_.pid) + " , size: " + to_string(maxProcessForReport_.virtualSize) + "\n\n";
    processInfoForReport_ += processInfo;
    HandleOverBaseProcessList(overBaseProcessList);
}

void KernelLeakInfo::MayAddFreeStateProcess(vector<OverBaseProcesses> &processList)
{
    if (processName_ != "ion") {
        return;
    }
    uint64_t totalVirtualSize = 0;
    for (auto itor = processList.begin(); itor != processList.end(); itor++) {
        if (itor->pid == -1 || itor->virtualSize == 0) {
            continue;
        }
        totalVirtualSize += itor->virtualSize;
    }
    int64_t lastSampleSize = DfrUtil::GetInstance().GetSizeInProcNode(MEM_INFO_PATH, meminfoName_);
    HIVIEW_LOGI("totalVirtualSize: %{public}lu, lastSampleSize: %{public}ld, meminfoName_: %{public}s",
        totalVirtualSize, lastSampleSize, meminfoName_.c_str());
    if (totalVirtualSize > static_cast<uint64_t>(INT64_MAX)) {
        HIVIEW_LOGE("TotalVirtualSize is over limit!");
        return;
    }

    // lastSampleSize(KB) totalVirtualSize(B)
    int64_t diffValueKb = lastSampleSize - static_cast<int64_t>(totalVirtualSize) / B_PER_KB;
    if (diffValueKb <= 0) {
        HIVIEW_LOGE("lastSampleSize less than totalVirtualSize, not exist freeState process");
        return;
    }

    int ionUsageThreshold = KernelLeakConfig::GetInstance().GetCommonKeyValue("ion_usage_threshold").intValue;
    if (diffValueKb / KB_PER_MB >= static_cast<int64_t>(ionUsageThreshold)) {
        existFreeState_ = true;
        HIVIEW_LOGI("diffValueKb: %{public}ld, will dump memtrack", diffValueKb);
    }
    OverBaseProcesses freeStateProcess;
    // if the sum of virtualSize less than tailMemory, exist freeState
    freeStateProcess.processName = processName_;
    freeStateProcess.processType = ABNORMAL_PROCESS;
    // virtualSize(B)
    freeStateProcess.virtualSize = static_cast<uint64_t>(diffValueKb * B_PER_KB);
    processList.push_back(freeStateProcess);
}

void KernelLeakInfo::DecideReportType()
{
    HIVIEW_LOGI("will report, processName_: %{public}s ", processName_.c_str());
    // ion, gpu, ashmem detect leak
    if (reportTypeByCountVirtualSize.count(processName_) != 0) {
        uint64_t virtualSize = maxProcessForReport_.virtualSize;
        uint32_t maxProcessForReportType = GetMaxProcessForReportType();
        HIVIEW_LOGI("virtualSize: %{public}lu, maxProcessForReport_.processType: %{public}u",
            virtualSize, maxProcessForReportType);
        if (maxProcessForReportType != ABNORMAL_PROCESS && virtualSize / BYTE_PER_KB < REPORT_MEMORY_LEAK_LIMIT) {
            ReportMemoryLeakWpEvent();
            return;
        }
        ReportMemoryLeakEvent();
    }
    if (processName_ == "gpu_rs" || processName_ == NAME_KERNEL_ZONE) {
        return;
    }
    // slab, cma, etc, detect leak, report memory_leak
    ReportMemoryLeakEvent();
}

bool KernelLeakInfo::ExistFreeState()
{
    return existFreeState_;
}

void KernelLeakInfo::HandleOverBaseProcessList(vector<OverBaseProcesses> &overBaseProcessList)
{
    if (overBaseProcessList.size() == 0) {
        processInfoForReport_ += "There is no process over kernel kill baseline.\n";
        HIVIEW_LOGI("%{public}s", processInfoForReport_.c_str());
        return;
    }
    string overProcessInfo = "\nover limit: ";
    sort(overBaseProcessList.begin(), overBaseProcessList.end(), JudgeProcessSizeWithProcessType);
    processInfoForReport_ += "Over base processes sort by kill rule: \n";
    for (auto itor : overBaseProcessList) {
        if (overProcessInfo.find(itor.processName) != string::npos) {
            continue;
        }
        overProcessInfo += itor.processName + ", ";
        processInfoForReport_ += "processname: " + itor.processName + ", pid: " + to_string(itor.pid) +
        ", type:" +  PROCESS_TYPE_STRING[itor.processType] + ", size: " + to_string(itor.virtualSize) + "\n";
    }
    maxProcessForKill_ = overBaseProcessList[0];
    maxProcessForReport_ = maxProcessForKill_;
    string processInfo = "The process for kill is:" + maxProcessForKill_.processName + ", pid: " +
        to_string(maxProcessForKill_.pid) + " , size: " + to_string(maxProcessForKill_.virtualSize) + "\n";
    processInfoForReport_ += processInfo + overProcessInfo;
    HIVIEW_LOGI("%{public}s", processInfoForReport_.c_str());
    if (maxProcessForKill_.pid != -1) {
        // only processName_ gpu, ashmem, ion, If other processes exist, add a judgment condition.
        ReportMemoryLeakEvent();
    }
    SetLeakGrade(LEAK_CONTROL);
}

string KernelLeakInfo::GetProcessInfoReport()
{
    return processInfoForReport_;
}

void KernelLeakInfo::SetProcessInfoReport(const string &info)
{
    processInfoForReport_ = info;
}

string KernelLeakInfo::GetDumpReason()
{
    return dumpReason_;
}

void KernelLeakInfo::SetDumpReason(const string &dumpReason)
{
    dumpReason_ = dumpReason;
}

string KernelLeakInfo::GetRsMemPathFromEvent()
{
    return rsMemPathInEvent_;
}

void KernelLeakInfo::SetRsMemPathFromEvent(const string &rsMemPath)
{
    rsMemPathInEvent_ = rsMemPath;
}

bool KernelLeakInfo::JudgeProcessSizeWithProcessType(const OverBaseProcesses &processFirst,
    const OverBaseProcesses &processSecond)
{
    if (processFirst.processType == processSecond.processType) {
        return processFirst.virtualSize > processSecond.virtualSize;
    }
    return processFirst.processType > processSecond.processType;
}

bool KernelLeakInfo::JudgeProcessSize(const OverBaseProcesses &processFirst, const OverBaseProcesses &processSecond)
{
    return processFirst.virtualSize > processSecond.virtualSize;
}

void KernelLeakInfo::SetProcessForKill(OverBaseProcesses &overBaseProcess)
{
    if (overBaseProcess.pid < 0) {
        HIVIEW_LOGW("SetProcessForKill failed, pid is invalid");
    }
    maxProcessForKill_ = overBaseProcess;
}

void KernelLeakInfo::SetProcessForReport(OverBaseProcesses &overBaseProcess)
{
    maxProcessForReport_ = overBaseProcess;
    SetPid(overBaseProcess.pid);
    SetUid(overBaseProcess.pid);
    GetAppRunningId();
    GetUid();
}

string KernelLeakInfo::GetProcessNameForReport() const
{
    return maxProcessForReport_.processName.empty() ? processName_ : maxProcessForReport_.processName;
}

string KernelLeakInfo::GetProcessNameForKill()
{
    return maxProcessForKill_.processName.empty() ? processName_ : maxProcessForKill_.processName;
}

pid_t KernelLeakInfo::GetProcessPidForKill()
{
    return maxProcessForKill_.pid;
}

pid_t KernelLeakInfo::GetProcessPidForReport() const
{
    return maxProcessForReport_.pid;
}

OverBaseProcesses KernelLeakInfo::GetProcessForReport()
{
    return maxProcessForReport_;
}

uint32_t KernelLeakInfo::GetMaxProcessForReportType()
{
    return maxProcessForReport_.processType;
}

uint64_t KernelLeakInfo::GetProcessSizeForReport()
{
    return maxProcessForReport_.virtualSize;
}

void KernelLeakInfo::PrintSampleHead(ofstream &fout)
{
    fout << "memoryName:" << GetProcessName() << endl;
    fout << "softThreshold:" << (GetMemoryLimit() / KB_PER_MB) << "(MB)" << endl;
    fout << "hardThreshold:" << (GetHardMemoryLimit() / KB_PER_MB) << "(MB)" << endl;
    fout << "topMemory:" <<  GetTopMemory() << "(KB)" << endl;
    fout << left;
    fout << endl;
    fout << setw(SHORT_WEIGHT) << "time(s)" << setw(LONG_WEIGHT) << "kernelMemory(KB)";
    fout << setw(SHORT_WEIGHT) << "realtime" << endl;
}

ProcessAnomalyInfo KernelLeakInfo::GetProcessInfoToKill()
{
    ProcessAnomalyInfo processInfoForKill;
    OHOS::XPowerManager::ProcessInfo procInfo;
    if (ProcessMgr::Instance().GetProcessInfoByName(maxProcessForKill_.processName, procInfo) &&
        procInfo.pid == maxProcessForKill_.pid) {
        processInfoForKill.type = procInfo.subProcessType;
    } else {
        HIVIEW_LOGE("cannot get process info by name: %{public}s, continue",
                    maxProcessForKill_.processName.c_str());
        processInfoForKill.type = UNSPECIFIED_PROCESS_TYPE;
    }
    processInfoForKill.resType = processName_;
    processInfoForKill.processName = maxProcessForKill_.processName;
    processInfoForKill.resource = static_cast<int64_t>(maxProcessForKill_.virtualSize);
    processInfoForKill.uid = DfrUtil::GetInstance().GetProcessUid(maxProcessForKill_.pid);
    processInfoForKill.pid = maxProcessForKill_.pid;
    processInfoForKill.action = "kill";
    if (maxProcessForKill_.conditionType == KILLING_FOREGROUND_BACKGROUND ||
        (processName_ == "vma_leak" &&
         KernelLeakConfig::GetInstance().IsInVmaKillList(maxProcessForKill_.processName))) {
        HIVIEW_LOGI("Current leak : %{public}s -- process: %{public}s:%{public}d "
                    "should be killed on foreground or background!", processName_.c_str(),
                    maxProcessForKill_.processName.c_str(), maxProcessForKill_.pid);
        processInfoForKill.condition.emplace_back(0);
    } else {
        HIVIEW_LOGI("Current leak : %{public}s should be killed on background!", processName_.c_str());
        processInfoForKill.condition.emplace_back(ResourceQuotaCtl::CONDITION_BACKGROUND);
    }
    return processInfoForKill;
}

KillingState KernelLeakInfo::ShouldBeKilled()
{
    if (maxProcessForKill_.pid == -1) {
        HIVIEW_LOGI("cannot get process info by name: %{public}s, should not be killed!",
                    maxProcessForKill_.processName.c_str());
        return SHOULD_NOT_KILL;
    }
    if (!KernelLeakConfig::GetInstance().GetKillEnable(processName_)) {
        HIVIEW_LOGI("killEnable is disabled, donot kill process: %{public}s pid: %{public}d",
            maxProcessForKill_.processName.c_str(), maxProcessForKill_.pid);
        return SHOULD_NOT_KILL;
    }
    if (processName_ == "vma_leak" &&
        KernelLeakConfig::GetInstance().IsInVmaKillList(maxProcessForKill_.processName)) {
        HIVIEW_LOGI("Current processName: %{public}s leakType: %{public}s should be killed!",
            maxProcessForKill_.processName.c_str(), processName_.c_str());
        return SHOULD_KILL;
    }
    if (shouldKillList.count(processName_) == 0) {
        HIVIEW_LOGI("Current leak type: %{public}s should not be killed!", processName_.c_str());
        return SHOULD_NOT_KILL;
    }
    if (!DfrUtil::GetInstance().IsApp(maxProcessForKill_.pid)) {
        HIVIEW_LOGI("process: %{public}s is native, should not be killed!", maxProcessForKill_.processName.c_str());
        return SHOULD_NOT_KILL;
    }
    if (FaultDetectorManager::CheckIfAppForeGround(GetProcessNameForKill()) &&
        !DfrUtil::GetInstance().IsDeviceLowmem()) {
        return SHOULD_WAIT_KILL;
    }
    if (FaultDetectorManager::CheckIfAppForeGround(GetProcessNameForKill()) &&
        maxProcessForKill_.conditionType != KILLING_FOREGROUND_BACKGROUND) {
        HIVIEW_LOGI("the process for kill: %{public}s is foreground: should not be killed!",
                    maxProcessForKill_.processName.c_str());
        return SHOULD_WAIT_KILL;
    }
    return SHOULD_KILL;
}

bool KernelLeakInfo::IsReportFromEvent()
{
    return isReportFromEvent_;
}

void KernelLeakInfo::SetIsReportFromEvent(bool isReportFromEvent)
{
    isReportFromEvent_ = isReportFromEvent;
}

string KernelLeakInfo::GetVersionForReport()
{
    SetUid(maxProcessForReport_.pid);
    return GetHapVersion();
}

int KernelLeakInfo::GetMemoryLimit() const
{
    return memoryLimit_.load();
}

void KernelLeakInfo::SetMemoryLimit(int value)
{
    memoryLimit_.store(value);
}

int KernelLeakInfo::GetHardMemoryLimit() const
{
    return hardMemoryLimit_.load();
}

void KernelLeakInfo::SetHardMemoryLimit(int value)
{
    hardMemoryLimit_.store(value);
}

int KernelLeakInfo::GetAppMemoryLimit() const
{
    return appMemoryLimit_.load();
}

void KernelLeakInfo::SetAppMemoryLimit(int value)
{
    appMemoryLimit_.store(value);
}

int KernelLeakInfo::GetAppMemoryHardLimit() const
{
    return appMemoryHardLimit_.load();
}

void KernelLeakInfo::SetAppMemoryHardLimit(int value)
{
    appMemoryHardLimit_.store(value);
}

int KernelLeakInfo::GetNativeMemoryLimit() const
{
    return nativeMemoryLimit_.load();
}

void KernelLeakInfo::SetNativeMemoryLimit(int value)
{
    nativeMemoryLimit_.store(value);
}

string KernelLeakInfo::GetProcMapsPath()
{
    return "/proc/" + to_string(maxProcessForReport_.pid) + "/maps";
}

string KernelLeakInfo::GetProcGpuPath()
{
    return "/proc/gpumem_process_info";
}

string KernelLeakInfo::GetMemtrackPath()
{
    return "/proc/memtrack";
}

bool KernelLeakInfo::IsProcInfoAbnormal()
{
    return maxProcessForReport_.processType == ABNORMAL_PROCESS;
}

void KernelLeakInfo::SetFirstFrame(const string &firstFrame)
{
    firstFrame_ = firstFrame;
}

string KernelLeakInfo::GetFirstFrame() const
{
    return firstFrame_;
}

std::string KernelLeakInfo::GetAppRunningId()
{
    if (appRunningId_ != "") {
        return appRunningId_;
    }
    appRunningId_ = DfrUtil::GetInstance().GetAppRunningUniqueIdByPid(GetProcessPidForReport());
    return appRunningId_;
}

bool KernelLeakInfo::CheckProcessAlive() const
{
    const pid_t maxPid = maxProcessForKill_.pid;
    return maxProcessForKill_.processName == DfrUtil::GetInstance().GetProcessName(maxPid);
}

/*
This function is the main function for state transition.
must condation is ready, then init the transition rules.
condition:
1、state < dumpState and process died ---every time needed，directly return finished

2、in wating list ---Required when newly leak detected
3、is ever report ---Required when newly leak detected
4、leak grade ---According to actual needs

kernelLeak not involving extra log

ever report:
     yes:   dump-> removal -> appReportKill -> reportOverLimit -> finish
     no:    dump -> report -> appReport -> removal -> appReportKill -> reportOverLimit -> finish

*/
FaultStateType KernelLeakInfo::GetNextState()
{
    FaultStateType curState = GetState();
    if (curState < PROC_DUMP_STATE && !CheckProcessAlive()) {
        return PROC_FINISHED_STATE;
    }
    switch (curState) {
        case PROC_IDLE_STATE: {
            if (IsEverReported() && ShouldBeKilled() == SHOULD_NOT_KILL) {
                return PROC_FINISHED_STATE;
            }
            return PROC_DUMP_STATE;
        }
        // dump
        case PROC_DUMP_STATE: {
            if (IsEverReported()) {
                return PROC_REMOVAL_STATE;
            }
            return PROC_REPORT_STATE;
        }
        // report
        case PROC_REPORT_STATE: {
            if (leakGrade_.load() >= LEAK_CONTROL) {
                return PROC_APP_REPORT_STATE;
            }
            return PROC_FINISHED_STATE;
        }
        // appReport
        case PROC_APP_REPORT_STATE: {
            return PROC_REMOVAL_STATE;
        }
        // remove
        case PROC_REMOVAL_STATE: {
            if (CheckProcessAlive()) {
                return PROC_FINISHED_STATE;
            } else {
                return PROC_APP_REPORT_KILL_STATE;
            }
        }
        case PROC_APP_REPORT_KILL_STATE: {
            return PROC_REPORT_OVERLIMIT_STATE;
        }
        case PROC_REPORT_OVERLIMIT_STATE: {
            return PROC_FINISHED_STATE;
        }
        default:
            return PROC_FINISHED_STATE;
    }
}

void KernelLeakInfo::SaveReportInfoBeforeKill()
{
    if (GetProcessName() == "ion") {
        SaveDmabufReportInfo();
    } else if (GetProcessName() == "gpu") {
        SaveGpuReportInfo();
    }
}

void KernelLeakInfo::SaveDmabufReportInfo()
{
    isReportInfoSaveSuccess_ = false;
    string nodePath = "/proc/" + to_string(GetProcessPidForKill()) + "/mm_dmabuf_info";
    std::ifstream file(nodePath);
    if (!file.is_open()) {
        HIVIEW_LOGE("can't open file : %{public}s", nodePath.c_str());
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    detailDmaBufInfo_ = DetailInfoParser::GetInstance().ParseIonProcessInfo(buffer.str());
    if (detailDmaBufInfo_.totalSize == 0) {
        HIVIEW_LOGE("totalsize is 0 in saving dmabuf report info.");
        return;
    }
    adjForReport_ = DfrUtil::GetInstance().GetProcessAdj(GetProcessPidForKill());
    if (adjForReport_ == INT_MAX) {
        HIVIEW_LOGE("can't get processAdj");
        return;
    }
    isReportInfoSaveSuccess_ = true;
}

void KernelLeakInfo::SaveGpuReportInfo()
{
    isReportInfoSaveSuccess_ = false;
    string buffer = DfrUtil::GetInstance().SendLeakCmdInfo(GetProcessPidForKill(),
        CMD_GET_MEM_DETAIL_INFO, MTYPE_USER_GPU);

    detailGpuInfo_ = DetailInfoParser::GetInstance().ParseGpuProcessInfo(buffer);
    adjForReport_ = DfrUtil::GetInstance().GetProcessAdj(GetProcessPidForKill());
    if (adjForReport_ == INT_MAX) {
        HIVIEW_LOGE("can't get processAdj");
        return;
    }
    isReportInfoSaveSuccess_ = true;
}

DetailDmaBufInfo KernelLeakInfo::GetDmabufReportInfo() const
{
    return detailDmaBufInfo_;
}

DetailGpuInfo KernelLeakInfo::GetGpuReportInfo() const
{
    return detailGpuInfo_;
}

int KernelLeakInfo::GetAdjForReport() const
{
    return adjForReport_;
}

bool KernelLeakInfo::GetIsReportInfoSaveSuccess() const
{
    return isReportInfoSaveSuccess_;
}

void KernelLeakInfo::SetRenderNode(int32_t renderNode)
{
    renderNode_ = renderNode;
}

int32_t KernelLeakInfo::GetRenderNode() const
{
    return renderNode_;
}

void KernelLeakInfo::SetMaxImageSize(int32_t maxImageSize)
{
    maxImageSize_ = maxImageSize;
}

int32_t KernelLeakInfo::GetMaxImageSize() const
{
    return maxImageSize_;
}

void KernelLeakInfo::SetMaxImageName(const std::string &maxImageName)
{
    maxImageName_ = maxImageName;
}

std::string KernelLeakInfo::GetMaxImageName() const
{
    return maxImageName_;
}

void KernelLeakInfo::SetMaxImageResolution(const std::string &maxImageResolution)
{
    maxImageResolution_ = maxImageResolution;
}

std::string KernelLeakInfo::GetMaxImageResolution() const
{
    return maxImageResolution_;
}

uint64_t KernelLeakInfo::GetTelemetryType()
{
    if (telemetryType_ > 0) {
        return telemetryType_;
    }
    const string &eventName = GetProcessName();
    map<string, uint64_t> telemetryTypeMap {
        { "ashmem", ASHMEM_LEAK_TYPE },
        { "ion", ION_LEAK_TYPE },
        { "gpu", GPU_LEAK_TYPE },
    };
    telemetryType_ = telemetryTypeMap[eventName];
    return telemetryType_;
}

bool KernelLeakInfo::CouldTelemetry()
{
    uint64_t telemetryType = GetTelemetryType();
    return GetTelemetry().CouldTelemetry(telemetryType, GetProcessNameForReport());
}

bool KernelLeakInfo::InTelemetry()
{
    uint64_t telemetryType = GetTelemetryType();
    return GetTelemetry().InTelemetry(telemetryType, GetProcessNameForReport());
}

struct EventQuota KernelLeakInfo::GetEventQuota()
{
    uint64_t telemetryType = GetTelemetryType();
    return GetTelemetry().GetEventQuota(telemetryType, GetProcessNameForReport());
}

void KernelLeakInfo::ConsumeQuota()
{
    uint64_t telemetryType = GetTelemetryType();
    GetTelemetry().ConsumeQuota(telemetryType, GetProcessNameForReport());
}

string KernelLeakInfo::GetCopyProfilerName() const
{
    string fileLogName = "memleak-kernel-hiapp-" + GetProcessNameForReport() + "-" +
                         to_string(pid_) + "-" + leakedTime_ + ".txt";
    return fileLogName;
}

string KernelLeakInfo::GetCopyProfilerPath() const
{
    return string(MEMORY_LEAK_PATH) + "/" + GetCopyProfilerName();
}
} // namespace HiviewDFX
} // namespace OHOS
