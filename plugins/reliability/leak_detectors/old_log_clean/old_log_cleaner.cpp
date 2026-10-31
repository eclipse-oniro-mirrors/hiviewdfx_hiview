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
 * Author: zhouhang
 * Create: 2024-8-21
 */

#include <climits>
#include <sys/sendfile.h>
#include "old_log_cleaner.h"
#include "dfr_util.h"
#include "hiview_logger.h"
#include "old_log_param_config.h"
#include "fd_leak_info.h"
#include "kernel_leak_info.h"
#include "native_leak_info.h"
#include "thread_leak_info.h"
#include "fault_common.h"

namespace {
constexpr int KEEP_SMAPS_FILES = 50;
constexpr int KEEP_PROFILER_FILES = 25;
constexpr int KEEP_SAMPLE_FILES = 50;
constexpr int EXCEED_SMAPS_NUM = 10;
constexpr int EXCEED_PROFILER_NUM = 10;
constexpr int EXCEED_SAMPLE_NUM = 10;

constexpr int KEEP_FD_LEAK_FILES = 50;
constexpr int EXCEED_FD_LEAK_NUM = 10;

constexpr int KEEP_THREAD_LEAK_FILES = 50;
constexpr int EXCEED_THREAD_LEAK_NUM = 10;

constexpr int KEEP_NOTIFY_LEAK_FILES = 3;
constexpr int EXCEED_NOTIFY_LEAK_NUM = 1;

constexpr int KEEP_LOWMEM_FILES = 2;
constexpr int EXCEED_LOWMEM_NUM = 1;

constexpr int KEEP_HIDUMPER_FILES = 8;
constexpr int EXCEED_HIDUMPER_NUM = 2;

constexpr int KEEP_SYSLOAD_SMAPS_FILES = 5;
constexpr int EXCEED_SYSLOAD_SMAPS_NUM = 3;

constexpr int KEEP_SYSLOAD_MEM_FILES = 15;
constexpr int EXCEED_SYSLOAD_MEM_NUM = 5;

constexpr int KEEP_SYSLOAD_CPU_FILES = 15;
constexpr int EXCEED_SYSLOAD_CPU_NUM = 5;

constexpr int KEEP_SYSLOAD_KMSG_FILES = 15;
constexpr int EXCEED_SYSLOAD_KMSG_NUM = 5;

constexpr int KEEP_SYSLOAD_SUMMARY_FILES = 15;
constexpr int EXCEED_SYSLOAD_SUMMARY_NUM = 5;

constexpr int KEEP_SYSLOAD_SYSLOG_FILES = 15;
constexpr int EXCEED_SYSLOAD_SYSLOG_NUM = 5;

constexpr int KEEP_KERNEL_LOG_FILES = 30;
constexpr int EXCEED_KERNEL_LOG_NUM = 20;

constexpr int KEEP_GWPASAN_LOG_FILES = 10;
constexpr int EXCEED_GWPASAN_LOG_NUM = 5;


constexpr int KEEP_UI_TREE_LOG_FILES = 15;
constexpr int EXCEED_UI_TREE_LOG_NUM = 5;

constexpr int KEEP_RS_TREE_LOG_FILES = 15;
constexpr int EXCEED_RS_TREE_LOG_NUM = 5;

constexpr int KEEP_MEDIA_LOG_FILES = 10;
constexpr int EXCEED_MEDIA_LOG_NUM = 5;

constexpr int KEEP_AVC_LOG_FILES = 10;
constexpr int EXCEED_AVC_LOG_NUM = 5;

constexpr int KEEP_VNODE_LOG_FILES = 10;
constexpr int EXCEED_VNODE_LOG_NUM = 5;

constexpr int PATH_FOLDER_OLD_SIZE = 300;
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "LeakOldLogCleaner");

OldLogCleaner::OldLogCleaner() {}

OldLogCleaner::~OldLogCleaner() {}

using namespace std;

using LogFileComparator = function<bool(const LogFile&, const LogFile&)>;
LogFileComparator comparator = [](const LogFile &lhs, const LogFile &rhs) {
    return rhs < lhs;
};

// log type mapping config
const std::map<LogType, LogTypeStru> logTypeStruMap = {
    {LogType::SMAPS, {"smapsLog", MEMORY_LEAK_PATH, "smaps", "memleak-native"}},
    {LogType::SAMPLE, {"sampleLog", MEMORY_LEAK_PATH, "sample", "memleak-native"}},
    {LogType::NATIVE_PROFILER, {"nativeProfilerLog", MEMORY_LEAK_PATH, "profiler", "memleak-native"}},
    {LogType::RAWHEAP, {"rawheapLog", MEMORY_LEAK_PATH, "rawheap", "memleak-js"}},
    {LogType::LOWMEM, {"lowmemLog", MEMORY_LEAK_PATH, "lowmem", ""}},
    {LogType::FD_LEAK_LOG, {"fdLog", FILE_DESCRIPTOR_LEAK_PATH, "log", "fdleak"}},
    {LogType::FD_PROFILER, {"fdProfilerLog", FILE_DESCRIPTOR_LEAK_PATH, "profiler", "fdleak"}},
    {LogType::THREAD_LEAK_LOG, {"threadLog", THREAD_LEAK_PATH, "log", "threadleak"}},
    {LogType::THREAD_PROFILER, {"threadProfilerLog", THREAD_LEAK_PATH, "profiler", "threadleak"}},
    /* hidumper-jsheap-xxx.rawheap and hidumper-jsheap-xxx both startWith hidumper,
     * have jsheap mark, it is necessary to match more features first.
     */
    {LogType::HIDUMPER_RAWHEAP, {"hidumperLog", MEMORY_LEAK_PATH, "heap", "hidumper"}},
    {LogType::SYSLOAD_DETECTOR_SMAPS, {"sysloadSmapsLog", SYSLOAD_DETECTOR_MEM_PATH, "smaps", "memleak-native"}},
    {LogType::SYSLOAD_DETECTOR_MEM, {"sysloadMemLog", SYSLOAD_DETECTOR_MEM_PATH, "MEM", "MEM"}},
    {LogType::SYSLOAD_DETECTOR_CPU, {"sysloadCpuLog", SYSLOAD_DETECTOR_CPU_PATH, "CPU", "CPU"}},
    {LogType::SYSLOAD_DETECTOR_KMSG, {"sysloadKmsgLog", SYSLOAD_DETECTOR_KMSG_PATH, "kmsg", "kmsg"}},
    {LogType::SYSLOAD_DETECTOR_SUMMARY, {"sysloadSummaryLog", SYSLOAD_DETECTOR_SUMMARY_PATH, "sysload", "sysload"}},
    {LogType::SYSLOAD_DETECTOR_SYSLOG, {"sysloadSysLog", SYSLOAD_DETECTOR_SYSLOG_PATH, "SYSLOG", "SYSLOG"}},
    {LogType::KERNEL_LOG, {"kernelLog", MEMORY_LEAK_PATH, "-0-", "memleak-kernel"}},
    {LogType::GWPASAN, {"gwpAsanLog", MEMORY_LEAK_PATH, "gwpasan", "memleak"}},
    {LogType::UI_TREE, {"uiTreeLog", MEMORY_LEAK_PATH, "uitree", "memleak-kernel"}},
    {LogType::RS_TREE, {"rsTreeLog", MEMORY_LEAK_PATH, "rstree", "memleak-kernel"}},
    {LogType::MEDIA, {"mediaLog", MEMORY_LEAK_PATH, "media", "memleak"}},
    {LogType::AVC, {"avcLog", MEMORY_LEAK_PATH, "avc", "memleak"}},
    {LogType::VNODE_LOG, {"vnodeLog", FILE_DESCRIPTOR_LEAK_PATH, "orphanvnode", "orphanvnode"}}
};

void OldLogCleaner::GetOldLogFilesByLogType(const LogTypeStru &logTypeStru, vector<LogFile> &fileList)
{
    auto dir = opendir(logTypeStru.path.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("failed to open dir: %{public}s, err: %{public}s", logTypeStru.path.c_str(),
            strerror(errno));
        return;
    }
    for (struct dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        string childNode = ent->d_name;
        string fullName = logTypeStru.path + "/" + childNode;
        // skip dir
        if (ShouldSkipFile(childNode, fullName, logTypeStru)) {
            continue;
        }
        // native profiler fileName not find profiler, if we unified style, this check can simplify
        PushProfiler2List(logTypeStru, fullName, childNode, fileList);
    }
    closedir(dir);
}

bool OldLogCleaner::ShouldSkipFile(const string &childNode, const string &fullName, const LogTypeStru &logTypeStru)
{
    if (childNode == "." || childNode == ".." || DfrUtil::GetInstance().IsDirectory(fullName)) {
        return true;
    }
    if (!logTypeStru.perfix.empty() && childNode.find(logTypeStru.perfix) == string::npos) {
        return true;
    }
    return false;
}

void OldLogCleaner::PushProfiler2List(const LogTypeStru &logTypeStru, const string &fullName,
                                      const string &childNode, vector<LogFile> &fileList)
{
    string mark = logTypeStru.mark;
    string key = logTypeStru.key;
    if (key != "nativeProfilerLog" && childNode.find(mark) != string::npos) {
        fileList.push_back(LogFile(fullName));
        return;
    }
    if (key == "nativeProfilerLog" && childNode.find("smaps") == string::npos &&
        childNode.find("sample") == string::npos) {
        fileList.push_back(LogFile(fullName));
    }
}

void OldLogCleaner::DoDeleteOldLogFiles(vector<LogFile> &fileList, int minKeep, int exceededNum, const LogType &type)
{
    int32_t deleteCount = 0;
    int countNeedRemove = static_cast<int>(fileList.size()) - (minKeep + exceededNum);
    if (countNeedRemove <= 0) {
        ResetFileLeakNums(type, static_cast<int>(fileList.size()));
        HIVIEW_LOGD("total size is : %{public}zu, logType is %{public}s", fileList.size(), LOG_TYPE_NAME[type]);
        fileList.clear();
        return;
    }
    sort(fileList.begin(), fileList.end(), comparator);
    for (auto it = fileList.rbegin(); it != fileList.rend(); ++it) {
        if (deleteCount >= (countNeedRemove + exceededNum)) {
            break;
        }
        if (access(it->path_.c_str(), F_OK) != 0) {
            HIVIEW_LOGE("access filePath failed: %{public}s. err: %{public}s", it->path_.c_str(), strerror(errno));
            continue;
        }
        if (IsInBlackList(it->path_)) {
            HIVIEW_LOGI("path is in blockList, path: %{public}s.", it->path_.c_str());
            continue;
        }
        if ((remove(it->path_.c_str()) != 0)) {
            HIVIEW_LOGE("remove filePath failed: %{public}s. err: %{public}s", it->path_.c_str(), strerror(errno));
            continue;
        }
        HIVIEW_LOGI("count-aging remove filePath: %{public}s.", it->path_.c_str());
        deleteCount++;
    }
    HIVIEW_LOGI("fileList size is: %{public}zu, remove count: %{public}" PRId32 ", logType is %{public}s",
                fileList.size(), deleteCount, LOG_TYPE_NAME[type]);
    ResetFileLeakNums(type, static_cast<int>(fileList.size() - deleteCount));
    fileList.clear();
}

void OldLogCleaner::AddToBlackList(const std::string &path)
{
    blockListMutex_.lock();
    blockList_.push_back(path);
    blockListMutex_.unlock();
}

bool OldLogCleaner::IsInBlackList(const string &path)
{
    bool ret;
    blockListMutex_.lock();
    ret = find(blockList_.begin(), blockList_.end(), path) != blockList_.end();
    blockListMutex_.unlock();
    return ret;
}

void OldLogCleaner::RemoveFromBlackList(const string &path)
{
    blockListMutex_.lock();
    auto it = find(blockList_.begin(), blockList_.end(), path);
    if (it != blockList_.end()) {
        blockList_.erase(it);
    }
    blockListMutex_.unlock();
}

LogType OldLogCleaner::GetLogType(const std::string &fileName, const std::string &filePath)
{
    for (const auto& [logType, stru] : logTypeStruMap) {
        if (!stru.path.empty() && filePath.find(stru.path) == std::string::npos) {
            continue;
        }
        if (!stru.perfix.empty() && fileName.find(stru.perfix) == std::string::npos) {
            continue;
        }
        if (fileName.find(stru.mark) != std::string::npos) {
            return logType;
        }
        // native profiler fileName not find profiler, if we unified style, this check can detete
        if (stru.key == "nativeProfilerLog" && fileName.find("smaps") == string::npos &&
            fileName.find("sample") == string::npos) {
                return NATIVE_PROFILER;
        }
    }
    return UNKNOWN_LOG_TYPE;
}

bool OldLogCleaner::IsSysloadType(const LogType &type)
{
    return type == SYSLOAD_DETECTOR_MEM || type == SYSLOAD_DETECTOR_CPU ||
           type == SYSLOAD_DETECTOR_KMSG || type == SYSLOAD_DETECTOR_SUMMARY ||
           type == SYSLOAD_DETECTOR_SYSLOG;
}

void OldLogCleaner::AddFileLeakNums(const LogType &type)
{
    if (type != UNKNOWN_LOG_TYPE) {
        fileNums_[type].fetch_add(1);
        HIVIEW_LOGD("fileNums_: %{public}d, type: %{public}s", fileNums_[type].load(), LOG_TYPE_NAME[type]);
    }
}

void OldLogCleaner::SubFileLeakNums(const LogType &type)
{
    if (type != UNKNOWN_LOG_TYPE) {
        fileNums_[type].fetch_sub(1);
        HIVIEW_LOGD("fileNums_: %{public}d, type: %{public}s", fileNums_[type].load(), LOG_TYPE_NAME[type]);
    }
}

void OldLogCleaner::ResetFileLeakNums(const LogType &type, int fileCount)
{
    if (type != UNKNOWN_LOG_TYPE) {
        fileNums_[type].store(fileCount);
        HIVIEW_LOGD("fileNums_: %{public}d, type: %{public}s", fileNums_[type].load(), LOG_TYPE_NAME[type]);
    }
}

void OldLogCleaner::MayRemoveOldLog(const LogType &type)
{
    if (type == UNKNOWN_LOG_TYPE) {
        return;
    }
    auto it = logTypeStruMap.find(type);
    if (it == logTypeStruMap.end()) {
        return;
    }
    const LogTypeStru &logTypeStru = it->second;
    const auto &categoryConfig = oldLogCategory_.oldLogStandard[logTypeStru.key];
    int currentFileNums = fileNums_[type].load();
    bool isSysloadType = IsSysloadType(type);
    if (currentFileNums > categoryConfig.fileKeepNums) {
        HIVIEW_LOGD("fileNums count: %{public}d, type: %{public}s", currentFileNums, logTypeStru.mark.c_str());
    }
    if (!isSysloadType && currentFileNums <= categoryConfig.fileKeepNums + categoryConfig.fileExceedNums) {
        return;
    }
    std::vector<LogFile> logFiles;
    GetOldLogFilesByLogType(logTypeStru, logFiles);
    DoDeleteOldLogFiles(logFiles, categoryConfig.fileKeepNums, categoryConfig.fileExceedNums, type);
}

void OldLogCleaner::GetMatchingFileCount(const string &path, bool firstCharMustDigit)
{
    auto dir = opendir(path.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("failed to open dir: %{public}s, err: %{public}s", path.c_str(), strerror(errno));
        return;
    }
    for (struct dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        string childNode = ent->d_name;
        string fullName = path + "/" + childNode;
        if (childNode == "." || childNode == "..") {
            continue;
        }
        if (DfrUtil::GetInstance().IsDirectory(fullName)) {
            continue;
        }
        if (firstCharMustDigit == !isdigit(childNode[0])) {
            continue;
        }
        LogType type = GetLogType(childNode, path);
        AddFileLeakNums(type);
    }
    closedir(dir);
}

void OldLogCleaner::ManageLogs(const string &procName, const string &fileName, const string &path)
{
    string targetFile;
    LogType type = GetLogType(fileName, path);
    if (DirectCleanNow(procName, fileName, targetFile)) {
        string parentPath = DfrUtil::GetInstance().GetParentPath(path);
        string fullPath = parentPath + "/" + targetFile;
        if (access(fullPath.c_str(), F_OK) != 0) {
            HIVIEW_LOGE("access filePath failed: %{public}s. err: %{public}s", targetFile.c_str(), strerror(errno));
        } else if ((remove(fullPath.c_str()) != 0)) {
            HIVIEW_LOGE("remove filePath failed: %{public}s. err: %{public}s", targetFile.c_str(), strerror(errno));
        } else {
            SubFileLeakNums(type);
            HIVIEW_LOGI("remove old file: %{public}s", targetFile.c_str());
        }
    }
    AddFileLeakNums(type);
    HIVIEW_LOGD("create new file: %{public}s", path.c_str());
    MayRemoveOldLog(type);

    // 触发基于文件夹大小的老化
    if (IsOlderTriggerType(type)) {
        MayRemoveOldLogByFolderSize(type);
    }
}

void OldLogCleaner::AgingSysloadLog(const string &faultType)
{
    struct SysLoadLogConfig {
        std::string path;
        std::string name;
    };

    std::vector<SysLoadLogConfig> logConfigs;
    if (faultType == "mem") {
        logConfigs.emplace_back(SysLoadLogConfig{SYSLOAD_DETECTOR_MEM_PATH, "MEM"});
    } else if (faultType == "cpu") {
        logConfigs.emplace_back(SysLoadLogConfig{SYSLOAD_DETECTOR_CPU_PATH, "CPU"});
    } else {
        HIVIEW_LOGE("faultType that do not belong to sysload");
        return;
    }

    // Adding fixed log rotation for kmsg sysload, and syslog
    logConfigs.emplace_back(SysLoadLogConfig{SYSLOAD_DETECTOR_KMSG_PATH, "kmsg"});
    logConfigs.emplace_back(SysLoadLogConfig{SYSLOAD_DETECTOR_SUMMARY_PATH, "sysload"});
    logConfigs.emplace_back(SysLoadLogConfig{SYSLOAD_DETECTOR_SYSLOG_PATH, "SYSLOG"});

    for (const auto &config : logConfigs) {
        ManageLogs("", config.name, config.path);
    }
}

bool OldLogCleaner::DirectCleanNow(const string &procName, const string &fileName, string &needCleanFileName)
{
    bool needCleanNow = false;
    if (fileName.find("sample") != string::npos) {
        vector<string> directCleanList = DfrUtil::GetInstance().Split(oldLogCategory_.directCleanType, ',');
        if (find(directCleanList.begin(), directCleanList.end(), "TMPSample") != directCleanList.end()) {
            int currentPid = ExtractPid(fileName);
            if (currentPid == -1) {
                HIVIEW_LOGE("get pid failed, fileName: %{public}s", fileName.c_str());
                return false;
            }
            procPidMapMutex_.lock();
            auto it = procPidMap_.find(procName);
            if (it == procPidMap_.end()) {
                procPidMap_[procName] = currentPid;
                procPidMapMutex_.unlock();
                return false;
            }
            if (it->second == currentPid) {
                // the same file
                procPidMapMutex_.unlock();
                return false;
            }
            // file with the same procName but different pid
            needCleanFileName = "memleak-native-" + procName + "-" + to_string(it->second) + "-sample.txt";
            procPidMap_[procName] = currentPid;
            procPidMapMutex_.unlock();
            needCleanNow = true;
        }
    }
    return needCleanNow;
}

void OldLogCleaner::RemoveAllLeakFiles()
{
    DfrUtil::GetInstance().RemoveAllFilesInDir(MEMORY_LEAK_PATH);
    DfrUtil::GetInstance().RemoveAllFilesInDir(FILE_DESCRIPTOR_LEAK_PATH);
    DfrUtil::GetInstance().RemoveAllFilesInDir(THREAD_LEAK_PATH);
    DfrUtil::GetInstance().RemoveAllFilesInDir(SYSLOAD_DETECTOR_MEM_PATH);
    DfrUtil::GetInstance().RemoveAllFilesInDir(SYSLOAD_DETECTOR_CPU_PATH);
    DfrUtil::GetInstance().RemoveAllFilesInDir(SYSLOAD_DETECTOR_KMSG_PATH);
    DfrUtil::GetInstance().RemoveAllFilesInDir(SYSLOAD_DETECTOR_SUMMARY_PATH);
    DfrUtil::GetInstance().RemoveAllFilesInDir(SYSLOAD_DETECTOR_SYSLOG_PATH);
}

void OldLogCleaner::InitAllFileNums(bool clearFiles)
{
    if (clearFiles) {
        RemoveAllLeakFiles();
        return;
    }
    // smaps, sample, hiprofiler, lowmem, rawheap, syslog
    GetMatchingFileCount(MEMORY_LEAK_PATH, false);
    GetMatchingFileCount(FILE_DESCRIPTOR_LEAK_PATH, false);
    GetMatchingFileCount(THREAD_LEAK_PATH, false);
    GetMatchingFileCount(SYSLOAD_DETECTOR_MEM_PATH, false);
    GetMatchingFileCount(SYSLOAD_DETECTOR_CPU_PATH, false);
    GetMatchingFileCount(SYSLOAD_DETECTOR_SYSLOG_PATH, false);
}

int OldLogCleaner::ExtractPid(const string &fileName)
{
    size_t lastSample = fileName.find("-sample");
    if (lastSample == string::npos) {
        return -1;
    }
    size_t lastDash = fileName.rfind('-', lastSample - 1);
    if (lastDash == string::npos) {
        return -1;
    }
    int value;
    return DfrUtil::GetInstance().SafeStoi(fileName.substr(lastDash + 1, lastSample - lastDash - 1), value) ?
        value : -1;
}

void OldLogCleaner::OldlogParamCheck(OldLogCategory &category)
{
    if (!category.directCleanType.empty()) {
        HIVIEW_LOGI("init old log category success!");
        return;
    }
    category.directCleanType = "TMPSample";
    category.oldLogStandard = {
        {"sampleLog",           {KEEP_SAMPLE_FILES, EXCEED_SAMPLE_NUM}},
        {"smapsLog",            {KEEP_SMAPS_FILES, EXCEED_SMAPS_NUM}},
        {"nativeProfilerLog",   {KEEP_PROFILER_FILES, EXCEED_PROFILER_NUM}},
        {"rawheapLog",          {KEEP_NOTIFY_LEAK_FILES, EXCEED_NOTIFY_LEAK_NUM}},
        {"lowmemLog",           {KEEP_LOWMEM_FILES, EXCEED_LOWMEM_NUM}},
        {"fdLog",               {KEEP_FD_LEAK_FILES, EXCEED_FD_LEAK_NUM}},
        {"fdProfilerLog",       {KEEP_PROFILER_FILES, EXCEED_PROFILER_NUM}},
        {"threadLog",           {KEEP_THREAD_LEAK_FILES, EXCEED_THREAD_LEAK_NUM}},
        {"threadProfilerLog",   {KEEP_PROFILER_FILES, EXCEED_PROFILER_NUM}},
        {"hidumperLog",         {KEEP_HIDUMPER_FILES, EXCEED_HIDUMPER_NUM}},
        {"sysloadSmapsLog",     {KEEP_SYSLOAD_SMAPS_FILES, EXCEED_SYSLOAD_SMAPS_NUM}},
        {"sysloadMemLog",       {KEEP_SYSLOAD_MEM_FILES, EXCEED_SYSLOAD_MEM_NUM}},
        {"sysloadCpuLog",       {KEEP_SYSLOAD_CPU_FILES, EXCEED_SYSLOAD_CPU_NUM}},
        {"sysloadKmsgLog",      {KEEP_SYSLOAD_KMSG_FILES, EXCEED_SYSLOAD_KMSG_NUM}},
        {"sysloadSummaryLog",   {KEEP_SYSLOAD_SUMMARY_FILES, EXCEED_SYSLOAD_SUMMARY_NUM}},
        {"sysloadSysLog",       {KEEP_SYSLOAD_SYSLOG_FILES, EXCEED_SYSLOAD_SYSLOG_NUM}},
        {"kernelLog",           {KEEP_KERNEL_LOG_FILES, EXCEED_KERNEL_LOG_NUM}},
        {"gwpAsanLog",           {KEEP_GWPASAN_LOG_FILES, EXCEED_GWPASAN_LOG_NUM}},
        {"uiTreeLog",           {KEEP_UI_TREE_LOG_FILES, EXCEED_UI_TREE_LOG_NUM}},
        {"rsTreeLog",           {KEEP_RS_TREE_LOG_FILES, EXCEED_RS_TREE_LOG_NUM}},
        {"mediaLog",            {KEEP_MEDIA_LOG_FILES, EXCEED_MEDIA_LOG_NUM}},
        {"avcLog",              {KEEP_AVC_LOG_FILES, EXCEED_AVC_LOG_NUM}},
        {"vnodeLog",             {KEEP_VNODE_LOG_FILES, EXCEED_VNODE_LOG_NUM}}
    };
    HIVIEW_LOGI("use default old log param!");
}

void OldLogCleaner::InitOldLogCleanerParam()
{
    for (auto &num : fileNums_) {
        num.store(0);
    }
    oldLogCategory_ = OldLogParamConfig::GetInstance().GetOldLogCategory();
    if (oldLogCategory_.folderOldLog.resourceLeakDirMaxSizeMB <= 0) {
        oldLogCategory_.folderOldLog.resourceLeakDirMaxSizeMB = PATH_FOLDER_OLD_SIZE;
    }
    OldlogParamCheck(oldLogCategory_);
}

void OldLogCleaner::Start()
{
    InitOldLogCleanerParam();
    InitAllFileNums(false);
    TimeAgingTypes_ = {LogType::RAWHEAP, LogType::HIDUMPER_RAWHEAP};
    // set rawheapAgingThreshold
    int agingHours = DfrUtil::GetInstance().IsBetaVersion() ?
        oldLogCategory_.rawheapBetaAgingHours : oldLogCategory_.rawheapNonbetaAgingHours;
    rawheapAgingThresholdSec_ = agingHours * MIN_PER_HOUR * SEC_PER_MIN;
}

bool OldLogCleaner::CopyFile(const string &src, const string &dst, const string &procName,
                             const string &fileName)
{
    bool ret = CopyFileFast(src, dst);
    ManageLogs(procName, fileName, dst);
    return ret;
}

bool OldLogCleaner::CopyFileFast(const string &src, const string &dst)
{
    const string &realSrc = DfrUtil::GetInstance().RealPath(src);
    if (realSrc.empty()) {
        HIVIEW_LOGE("failed src realpath, path: %{public}s", src.c_str());
        return false;
    }
    const string &realDst = DfrUtil::GetInstance().RealPath(dst);
    if (realDst.empty()) {
        HIVIEW_LOGE("failed dst realpath, path: %{public}s", dst.c_str());
        return false;
    }
    int fdIn = open(realSrc.c_str(), O_RDONLY);
    if (fdIn < 0) {
        HIVIEW_LOGE("failed to open realSrc file: %{public}s", realSrc.c_str());
        return false;
    }
    fdsan_exchange_owner_tag(fdIn, 0, FDSAN_TAG);
    int fdOut = open(realDst.c_str(), O_CREAT | O_NOFOLLOW | O_RDWR, DEFAULT_LOG_FILE_MODE);
    if (fdOut < 0) {
        HIVIEW_LOGE("failed to open realDst file: %{public}s", realDst.c_str());
        fdsan_close_with_tag(fdIn, FDSAN_TAG);
        return false;
    }
    fdsan_exchange_owner_tag(fdOut, 0, FDSAN_TAG);
    struct stat st;
    uint64_t totalLen = stat(realSrc.c_str(), &st) ? 0 : static_cast<uint64_t>(st.st_size);
    uint64_t copyTotalLen = 0;
    while (copyTotalLen < totalLen) {
        ssize_t copyLen = sendfile(fdOut, fdIn, nullptr, totalLen - copyTotalLen);
        if (copyLen <= 0) {
            break;
        }
        copyTotalLen += static_cast<uint64_t>(copyLen);
    }
    fdsan_close_with_tag(fdIn, FDSAN_TAG);
    fdsan_close_with_tag(fdOut, FDSAN_TAG);
    bool ret = copyTotalLen == totalLen;
    return ret;
}

void OldLogCleaner::GetFileParams(LogType logtype, FaultInfoBase *faultInfo, FileParams &fileParams)
{
    switch (logtype) {
        case FD_LEAK_LOG:
            GetFileParamsForFdLeakLog(faultInfo, fileParams);
            return;
        case FD_PROFILER:
            GetFileParamsForFdProfiler(faultInfo, fileParams);
            return;
        case SMAPS:
            GetFileParamsForSmaps(faultInfo, fileParams);
            return;
        case SAMPLE:
            GetFileParamsForSample(faultInfo, fileParams);
            return;
        case NATIVE_PROFILER:
            GetFileParamsForNativeProfiler(faultInfo, fileParams);
            return;
        case THREAD_LEAK_LOG:
            GetFileParamsForThreadLeakLog(faultInfo, fileParams);
            return;
        case THREAD_PROFILER:
            GetFileParamsForThreadProfiler(faultInfo, fileParams);
            return;
        case KERNEL_LEAK_PROFILER:
            GetFileParamsForVamLeakProfiler(faultInfo, fileParams);
            return;
        case GWPASAN:
            GetFileParamsForGwpAsan(faultInfo, fileParams);
            return;
        case UI_TREE:
            GetFileParamsForUiTree(faultInfo, fileParams);
            return;
        case RS_TREE:
            GetFileParamsForRsTree(faultInfo, fileParams);
            return;
        case MEDIA:
            GetFileParamsForMedia(faultInfo, fileParams);
            return;
        case AVC:
            GetFileParamsForAvc(faultInfo, fileParams);
            return;
        default:
            return;
    }
}

void OldLogCleaner::GetFileParamsForFdLeakLog(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    FdLeakInfo *fdInfo = static_cast<FdLeakInfo *>(faultInfo);
    fileParams.fileName = fdInfo->GetFdLogName();
    fileParams.procName = fdInfo->GetProcessName();
    fileParams.logPath = fdInfo->GetFdLogPath();
    fileParams.foutMode = ios::trunc;
}

void OldLogCleaner::GetFileParamsForFdProfiler(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    FdLeakInfo *fdInfo = static_cast<FdLeakInfo *>(faultInfo);
    fileParams.fileName = fdInfo->GetProfilerName();
    fileParams.procName = fdInfo->GetProcessName();
    fileParams.logPath = fdInfo->GetProfilerPath();
    fileParams.openFlags = O_CREAT | O_RDWR;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

void OldLogCleaner::GetFileParamsForSmaps(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    NativeLeakInfo *nativeInfo = static_cast<NativeLeakInfo *>(faultInfo);
    fileParams.fileName = nativeInfo->GetSmapsName();
    fileParams.procName = nativeInfo->GetProcessName();
    fileParams.logPath = nativeInfo->GetSmapsPath();
    fileParams.foutMode = ios::out;
    fileParams.openFlags = O_WRONLY | O_APPEND;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

void OldLogCleaner::GetFileParamsForSample(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    NativeLeakInfo *nativeInfo = static_cast<NativeLeakInfo *>(faultInfo);
    fileParams.fileName = nativeInfo->GetSampleFileName();
    fileParams.procName = nativeInfo->GetProcessName();
    fileParams.logPath = nativeInfo->GetSampleFilePath();
    fileParams.foutMode = ios::out;
    fileParams.openFlags = O_WRONLY | O_APPEND;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

void OldLogCleaner::GetFileParamsForNativeProfiler(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    NativeLeakInfo *nativeInfo = static_cast<NativeLeakInfo *>(faultInfo);
    fileParams.fileName = nativeInfo->GetProfilerName();
    fileParams.procName = nativeInfo->GetProcessName();
    fileParams.logPath = nativeInfo->GetProfilerPath();
    fileParams.foutMode = ios::out;
    fileParams.openFlags = O_CREAT | O_RDWR;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

void OldLogCleaner::GetFileParamsForThreadLeakLog(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    ThreadLeakInfo *threadInfo = static_cast<ThreadLeakInfo *>(faultInfo);
    fileParams.fileName = threadInfo->GetLogName();
    fileParams.procName = threadInfo->GetProcessName();
    fileParams.logPath = threadInfo->GetLogPath();
    fileParams.foutMode = ios::trunc;
}

void OldLogCleaner::GetFileParamsForThreadProfiler(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    ThreadLeakInfo *threadInfo = static_cast<ThreadLeakInfo *>(faultInfo);
    fileParams.fileName = threadInfo->GetProfilerName();
    fileParams.procName = threadInfo->GetProcessName();
    fileParams.logPath = threadInfo->GetProfilerPath();
    fileParams.openFlags = O_CREAT | O_RDWR;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

void OldLogCleaner::GetFileParamsForVamLeakProfiler(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    KernelLeakInfo *kernelInfo = static_cast<KernelLeakInfo *>(faultInfo);
    fileParams.fileName = kernelInfo->GetProfilerName();
    fileParams.procName = kernelInfo->GetProcessName();
    fileParams.logPath = kernelInfo->GetProfilerPath();
    fileParams.openFlags = O_CREAT | O_RDWR;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

void OldLogCleaner::GetFileParamsForGwpAsan(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    NativeLeakInfo *nativeInfo = static_cast<NativeLeakInfo *>(faultInfo);
    fileParams.fileName = nativeInfo->GetGwpAsanName();
    fileParams.procName = nativeInfo->GetProcessName();
    fileParams.logPath = nativeInfo->GetGwpAsanPath();
    fileParams.foutMode = ios::out;
    fileParams.openFlags = O_CREAT | O_RDWR;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}


void OldLogCleaner::GetFileParamsForUiTree(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    KernelLeakInfo *kernelInfo = static_cast<KernelLeakInfo *>(faultInfo);
    fileParams.fileName = kernelInfo->GetUiTreeName();
    fileParams.procName = kernelInfo->GetProcessName();
    fileParams.logPath = kernelInfo->GetUiTreePath();
    fileParams.foutMode = ios::out;
    fileParams.openFlags = O_CREAT | O_RDWR;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

void OldLogCleaner::GetFileParamsForRsTree(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    KernelLeakInfo *kernelInfo = static_cast<KernelLeakInfo *>(faultInfo);
    fileParams.fileName = kernelInfo->GetRsTreeName();
    fileParams.procName = kernelInfo->GetProcessName();
    fileParams.logPath = kernelInfo->GetRsTreePath();
    fileParams.openFlags = O_CREAT | O_RDWR;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

void OldLogCleaner::GetFileParamsForMedia(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    NativeLeakInfo *nativeInfo = static_cast<NativeLeakInfo *>(faultInfo);
    fileParams.fileName = nativeInfo->GetMediaMemName();
    fileParams.procName = nativeInfo->GetProcessName();
    fileParams.logPath = nativeInfo->GetMediaMemPath();
    fileParams.foutMode = ios::out;
    fileParams.openFlags = O_CREAT | O_RDWR;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

void OldLogCleaner::GetFileParamsForAvc(FaultInfoBase *faultInfo, FileParams &fileParams)
{
    NativeLeakInfo *nativeInfo = static_cast<NativeLeakInfo *>(faultInfo);
    fileParams.fileName = nativeInfo->GetAvcMemName();
    fileParams.procName = nativeInfo->GetProcessName();
    fileParams.logPath = nativeInfo->GetAvcMemPath();
    fileParams.foutMode = ios::out;
    fileParams.openFlags = O_CREAT | O_RDWR;
    fileParams.openMode = DEFAULT_LOG_FILE_MODE;
}

bool OldLogCleaner::IsOlderTriggerType(const LogType &type)
{
    // exclude RAWHEAP in FolderSizeOlder
    return type == LogType::NATIVE_PROFILER ||
           type == LogType::FD_PROFILER ||
           type == LogType::THREAD_PROFILER ||
           type == LogType::KERNEL_LEAK_PROFILER;
}

void OldLogCleaner::GetLogFileList(const vector<string> &filePathList, vector<LogFile> &fileList)
{
    for (const auto &filePath : filePathList) {
        LogFile logFile(filePath);
        fileList.push_back(logFile);
    }
}

uint64_t OldLogCleaner::CalcFolderSizeExcludingRawheap()
{
    uint64_t totalSize = DfrUtil::GetInstance().GetDirSize(RESOURCE_LEAK_PATH);
    uint64_t rawheapSize = 0;
    std::vector<std::string> filePathList;
    if (!DfrUtil::GetInstance().GetFilesInDir(MEMORY_LEAK_PATH, filePathList)) {
        return totalSize;
    }
    for (const auto &filePath : filePathList) {
        string fileName = filePath.substr(filePath.find_last_of('/') + 1);
        string dirPath = filePath.substr(0, filePath.find_last_of('/'));
        LogType logType = GetLogType(fileName, dirPath);
        if (logType != LogType::RAWHEAP && logType != LogType::HIDUMPER_RAWHEAP) {
            continue;
        }
        struct stat statBuf {};
        if (stat(filePath.c_str(), &statBuf) == 0) {
            rawheapSize += static_cast<uint64_t>(statBuf.st_size);
        }
    }
    return totalSize >= rawheapSize ? totalSize - rawheapSize : 0;
}

bool OldLogCleaner::MayOverFolderSize(uint64_t &currentSize, uint64_t &currentSizeMB)
{
    if (!oldLogCategory_.folderOldLog.enable) {
        HIVIEW_LOGD("Folder based aging is disabled");
        return false;
    }

    currentSize = CalcFolderSizeExcludingRawheap();
    currentSizeMB = currentSize / BYTE_PER_MB;
    if (currentSizeMB <= static_cast<uint64_t>(oldLogCategory_.folderOldLog.resourceLeakDirMaxSizeMB)) {
        HIVIEW_LOGD("Resource leak dir size (excl. RAWHEAP): %{public}lu MB, below limit: %{public}d MB",
                    currentSizeMB, oldLogCategory_.folderOldLog.resourceLeakDirMaxSizeMB);
        return false;
    }
    return true;
}

void OldLogCleaner::GetSortedFileListInResourceLeakDir(std::vector<LogFile> &fileList)
{
    vector<string> filePathList;
    if (!DfrUtil::GetInstance().GetFilesInDir(RESOURCE_LEAK_PATH, filePathList)) {
        HIVIEW_LOGE("Failed to get files in resource leak dir");
        return;
    }
    GetLogFileList(filePathList, fileList);
    std::sort(fileList.begin(), fileList.end(), comparator);
}

bool OldLogCleaner::RemoveFolderAgingFile(LogFile &logFile, LogType fileType, uint64_t &removedSize)
{
    struct stat statBuf {};
    if (stat(logFile.path_.c_str(), &statBuf) != 0) {
        HIVIEW_LOGE("Failed to stat file: %{public}s", logFile.path_.c_str());
        return false;
    }
    if (remove(logFile.path_.c_str()) != 0) {
        HIVIEW_LOGE("Failed to remove file: %{public}s", logFile.path_.c_str());
        return false;
    }
    removedSize += static_cast<uint64_t>(statBuf.st_size);
    SubFileLeakNums(fileType);
    HIVIEW_LOGD("folder-aging Removed old file: %{public}s, size: %{public}lu",
                logFile.path_.c_str(), static_cast<uint64_t>(statBuf.st_size));
    return true;
}

void OldLogCleaner::MayRemoveOldLogByFolderSize(const LogType &triggerType)
{
    uint64_t currentSize = 0;
    uint64_t currentSizeMB = 0;
    if (!MayOverFolderSize(currentSize, currentSizeMB)) {
        return;
    }
    HIVIEW_LOGI("Resource leak dir size: %{public}lu MB, exceeds limit: %{public}d MB, start aging",
                currentSizeMB, oldLogCategory_.folderOldLog.resourceLeakDirMaxSizeMB);

    vector<LogFile> fileList;
    GetSortedFileListInResourceLeakDir(fileList);

    uint64_t removedSize = 0;
    int removedCount = 0;

    for (auto it = fileList.rbegin(); it != fileList.rend(); ++it) {
        uint64_t remainingSizeMB = (currentSize - removedSize) / BYTE_PER_MB;
        if (remainingSizeMB <= static_cast<uint64_t>(oldLogCategory_.folderOldLog.resourceLeakDirMaxSizeMB)) {
            break;
        }

        LogType fileType = GetLogType(it->fileName_, it->dirPath_);
        if (fileType == LogType::RAWHEAP || fileType == LogType::HIDUMPER_RAWHEAP) {
            HIVIEW_LOGD("Skip %{public}s file in folder-based aging: %{public}s",
                        LOG_TYPE_NAME[fileType], it->path_.c_str());
            continue;
        }

        if (RemoveFolderAgingFile(*it, fileType, removedSize)) {
            removedCount++;
        }
    }

    uint64_t remainSizeMB = (currentSize - removedSize) / BYTE_PER_MB;
    HIVIEW_LOGI("folder-aging completed, removed %{public}d files, %{public}lu bytes, current size: %{public}lu MB",
                removedCount, removedSize, remainSizeMB);
}

void OldLogCleaner::TimeAgingForRawheapType(LogType logType)
{
    auto it = logTypeStruMap.find(logType);
    if (it == logTypeStruMap.end()) {
        HIVIEW_LOGE("Failed to find log type config for type %d", static_cast<int>(logType));
        return;
    }

    if (fileNums_[logType].load() <= 0) {
        return;
    }

    std::vector<LogFile> fileList;
    GetOldLogFilesByLogType(it->second, fileList);
    time_t now = time(nullptr);
    if (now == static_cast<time_t>(-1)) {
        HIVIEW_LOGE("failed to get current time");
        return;
    }
    int deletedCount = 0;

    for (auto &file : fileList) {
        struct stat statBuf {};
        if (stat(file.path_.c_str(), &statBuf) != 0) {
            HIVIEW_LOGE("Failed to stat file: %{public}s", file.path_.c_str());
            continue;
        }
        time_t fileAge = now - statBuf.st_mtime;
        if (fileAge < rawheapAgingThresholdSec_) {
            continue;
        }

        if (remove(file.path_.c_str()) == 0) {
            HIVIEW_LOGI("time-aging %{public}s file: %{public}s", LOG_TYPE_NAME[logType], file.path_.c_str());
            SubFileLeakNums(logType);
            deletedCount++;
        } else {
            HIVIEW_LOGE("Failed to remove %{public}s file: %{public}s", LOG_TYPE_NAME[logType], file.path_.c_str());
        }
    }

    if (deletedCount > 0) {
        HIVIEW_LOGI("time-aging completed for %{public}s, removed %{public}d files",
                    LOG_TYPE_NAME[logType], deletedCount);
    }
}

void OldLogCleaner::CheckAndAgingRawheapFiles()
{
    for (auto logType : TimeAgingTypes_) {
        TimeAgingForRawheapType(logType);
    }
}

void OldLogCleaner::DeleteLogFile(const std::string &filePath)
{
    if (filePath.empty() || access(filePath.c_str(), F_OK) != 0) {
        return;
    }
    string dirPath = filePath.substr(0, filePath.find_last_of('/'));
    string fileName = filePath.substr(filePath.find_last_of('/') + 1);
    LogType logType = GetLogType(fileName, dirPath);
    if (remove(filePath.c_str()) == 0) {
        HIVIEW_LOGI("Send-then-delete: removed %{public}s file: %{public}s", LOG_TYPE_NAME[logType], filePath.c_str());
        SubFileLeakNums(logType);
    } else {
        HIVIEW_LOGE("Send-then-delete: failed to remove %{public}s file: %{public}s, err: %{public}s",
                    LOG_TYPE_NAME[logType], filePath.c_str(), strerror(errno));
    }
}
} // namespace HiviewDFX
} // namespace OHOS
