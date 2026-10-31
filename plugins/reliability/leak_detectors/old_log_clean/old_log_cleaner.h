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

#ifndef DFR_OLD_LOG_CLEANER_H
#define DFR_OLD_LOG_CLEANER_H

#include <string>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <ctime>
#include <atomic>
#include <unistd.h>
#include "ffrt.h"
#include "singleton.h"
#include "log_file.h"
#include "old_log_param_config.h"
#include "dfr_util.h"
#include "fault_info_base.h"

namespace OHOS {
namespace HiviewDFX {

enum LogType {
    SMAPS = 0,
    SAMPLE,
    GWPASAN,
    MEDIA,
    AVC,
    NATIVE_PROFILER,
    RAWHEAP,
    LOWMEM,
    FD_LEAK_LOG,
    FD_PROFILER,
    THREAD_LEAK_LOG,
    THREAD_PROFILER,
    HIDUMPER_RAWHEAP,
    SYSLOAD_DETECTOR_SMAPS,
    SYSLOAD_DETECTOR_MEM,
    SYSLOAD_DETECTOR_CPU,
    SYSLOAD_DETECTOR_KMSG,
    SYSLOAD_DETECTOR_SUMMARY,
    SYSLOAD_DETECTOR_SYSLOG,
    KERNEL_LEAK_PROFILER,
    KERNEL_LOG,
    UI_TREE,
    RS_TREE,
    VNODE_LOG,
    UNKNOWN_LOG_TYPE,
    COUNT
};

inline constexpr const char* const LOG_TYPE_NAME[] = {
    "smapsLog",
    "sampleLog",
    "gwpAsanLog",
    "mediaLog",
    "avcLog",
    "nativeProfilerLog",
    "rawheapLog",
    "lowmemLog",
    "fdLog",
    "fdProfilerLog",
    "threadLog",
    "threadProfilerLog",
    "hidumperLog",
    "sysloadSmapsLog",
    "sysloadMemLog",
    "sysloadCpuLog",
    "sysloadKmsgLog",
    "sysloadSummaryLog",
    "sysloadSysLog",
    "vmaLeakProfilerLog",
    "kernelLog",
    "uiTreeLog",
    "rsTreeLog",
    "vnodeLog",
    "unknowLog"
};

struct LogTypeStru {
    std::string key;
    std::string path;
    std::string mark;
    std::string perfix;
};

class OldLogCleaner : public Singleton<OldLogCleaner> {
    DECLARE_SINGLETON(OldLogCleaner);
public:
    void ManageLogs(const std::string &procName, const std::string &fileName, const std::string &path);
    void AddToBlackList(const std::string &path);
    void AgingSysloadLog(const std::string &faultType);
    void RemoveFromBlackList(const std::string &path);
    void Start();
    bool CopyFile(const std::string &src, const std::string &dst, const std::string &procName,
        const std::string &fileName);
    void GetFileParams(LogType logtype, FaultInfoBase *faultInfo, FileParams &fileParams);
    void CheckAndAgingRawheapFiles();
    void DeleteLogFile(const std::string &filePath);

    template <typename OutputType>
    void GetLogFile(LogType logtype, FaultInfoBase *faultInfo, OutputType& file, bool isAppend = false)
    {
        FileParams fileParams;
        GetFileParams(logtype, faultInfo, fileParams);
        CreateFile(fileParams, file, isAppend);
    }

    template <typename OutputType>
    void CreateFile(FileParams &fileParams, OutputType &output, bool isAppend = false)
    {
        const std::string &realPath = DfrUtil::GetInstance().RealPath(fileParams.logPath);
        if (realPath.empty()) {
            return;
        }
        bool isFirstCreate = !DfrUtil::GetInstance().FileExist(realPath);

        if constexpr (std::is_same_v<OutputType, std::ofstream>) {
            output.open(realPath.c_str(), isAppend ? std::ios::out | std::ios::app : fileParams.foutMode);
        } else if constexpr (std::is_same_v<OutputType, int>) {
            output = open(realPath.c_str(), fileParams.openFlags, fileParams.openMode);
        } else {
            return;
        }

        if (isFirstCreate) {
            ManageLogs(fileParams.procName, fileParams.fileName, realPath);
        }
        fileParams.logPath = realPath;
    }

private:
    void GetOldLogFilesByLogType(const LogTypeStru &logTypeStru, std::vector<LogFile> &fileList);
    void DoDeleteOldLogFiles(std::vector<LogFile> &fileList, int minKeep, int exceededNum, const LogType &type);
    bool IsInBlackList(const std::string &path);
    LogType GetLogType(const std::string &fileName, const std::string &filePath);
    bool IsSysloadType(const LogType &type);
    void AddFileLeakNums(const LogType &type);
    void SubFileLeakNums(const LogType &type);
    void ResetFileLeakNums(const LogType &type, int fileCount);
    void MayRemoveOldLog(const LogType &type);
    void GetMatchingFileCount(const std::string &path, bool firstCharMustDigit);
    bool DirectCleanNow(const std::string &procName, const std::string &fileName, std::string &needCleanFileName);
    void RemoveAllLeakFiles();
    void InitAllFileNums(bool clearFiles);
    int ExtractPid(const std::string &fileName);
    void OldlogParamCheck(OldLogCategory &category);
    void InitOldLogCleanerParam();
    bool CopyFileFast(const std::string &src, const std::string &dst);
    void GetFileParamsForFdLeakLog(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForFdProfiler(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForSmaps(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForSample(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForNativeProfiler(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForThreadLeakLog(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForThreadProfiler(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForVamLeakProfiler(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForGwpAsan(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForUiTree(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForRsTree(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForMedia(FaultInfoBase *faultInfo, FileParams &fileParams);
    void GetFileParamsForAvc(FaultInfoBase *faultInfo, FileParams &fileParams);
    void PushProfiler2List(const LogTypeStru &logTypeStru, const std::string &fullName,
                           const std::string &childNode, std::vector<LogFile> &fileList);
    bool ShouldSkipFile(const std::string &childNode, const std::string &fullName, const LogTypeStru &logTypeStru);
    bool MayOverFolderSize(uint64_t &currentSize, uint64_t &currentSizeMB);
    void MayRemoveOldLogByFolderSize(const LogType &triggerType);
    bool IsOlderTriggerType(const LogType &type);
    void GetLogFileList(const std::vector<std::string> &filePathList, std::vector<LogFile> &fileList);
    void GetSortedFileListInResourceLeakDir(std::vector<LogFile> &fileList);
    bool RemoveFolderAgingFile(LogFile &logFile, LogType fileType, uint64_t &removedSize);
    uint64_t CalcFolderSizeExcludingRawheap();

private:

    ffrt::mutex blockListMutex_;
    std::vector<std::string> blockList_;

    ffrt::mutex procPidMapMutex_;
    std::unordered_map<std::string, int> procPidMap_;
    std::array<std::atomic<int>, COUNT> fileNums_;
    OldLogCategory oldLogCategory_;
    int rawheapAgingThresholdSec_ = INT_MAX;
    std::vector<LogType> TimeAgingTypes_;
    void TimeAgingForRawheapType(LogType logType);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_OLD_LOG_CLEANER_H
