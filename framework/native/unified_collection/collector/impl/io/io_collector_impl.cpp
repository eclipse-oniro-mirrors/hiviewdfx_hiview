/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "io_collector_impl.h"

#include <regex>

#include <fcntl.h>
#include <securec.h>
#include <string_ex.h>
#include <unistd.h>

#include "common_util.h"
#include "common_utils.h"
#include "file_util.h"
#include "io_calculator.h"
#include "io_decorator.h"
#include "hiview_logger.h"
#include "process_status.h"
#include "string_util.h"
#include "time_util.h"

using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("UCollectUtil-IoCollector");
constexpr int DISK_STATS_SIZE = 12;
constexpr int DISK_STATS_PERIOD = 2;
constexpr int PROC_IO_STATS_PERIOD = 2;
constexpr int EMMC_INFO_SIZE_RATIO = 2 * 1024 * 1024;
constexpr int MAX_FILE_NUM = 10;
const std::string MMC = "mmc";
const std::string EXPORT_FILE_SUFFIX = ".txt";
const std::string EXPORT_FILE_REGEX = "[0-9]{14}(.*)";
const std::string UNDERLINE = "_";
const std::string RAW_DISK_STATS_FILE_PREFIX = "proc_diskstats_";
const std::string DISK_STATS_FILE_PREFIX = "proc_diskstats_statistics_";
const std::string EMMC_INFO_FILE_PREFIX = "emmc_info_";
const std::string PROC_IO_STATS_FILE_PREFIX = "proc_io_stats_";
const std::string SYS_IO_STATS_FILE_PREFIX = "sys_io_stats_";
const std::string PROC_DISKSTATS = "/proc/diskstats";
const std::string COLLECTION_IO_PATH = "/data/log/hiview/unified_collection/io/";
}

std::shared_ptr<IoCollector> IoCollector::Create()
{
    static std::shared_ptr<IoCollector> instance_ = std::make_shared<IoDecorator>(std::make_shared<IoCollectorImpl>());
    return instance_;
}

IoCollectorImpl::IoCollectorImpl()
{
    HIVIEW_LOGI("init collect data.");
    InitDiskData();
    InitProcIoData();
}

void IoCollectorImpl::InitDiskData()
{
    std::unique_lock<std::mutex> lockDisk(collectDiskMutex_);
    preCollectDiskTime_ = TimeUtil::GetMilliseconds();
    CalculateDiskStats(0, true);
}

void IoCollectorImpl::InitProcIoData()
{
    std::unique_lock<std::mutex> lockProcIo(collectProcIoMutex_);
    currCollectProcIoTime_ = TimeUtil::GetMilliseconds();
    preCollectProcIoTime_ = currCollectProcIoTime_;
    CalculateAllProcIoStats(0, true);
}

CollectResult<ProcessIo> IoCollectorImpl::CollectProcessIo(int32_t pid)
{
    CollectResult<ProcessIo> result;
    std::string filename = PROC + std::to_string(pid) + IO;
    std::string content;
    FileUtil::LoadStringFromFile(filename, content);
    std::vector<std::string> vec;
    OHOS::SplitStr(content, "\n", vec);
    ProcessIo& processIO = result.data;
    processIO.pid = pid;
    processIO.name = CommonUtils::GetProcNameByPid(pid);
    std::string type;
    int32_t value = 0;
    for (const std::string &str : vec) {
        if (CommonUtil::ParseTypeAndValue(str, type, value)) {
            if (type == "rchar") {
                processIO.rchar = value;
            } else if (type == "wchar") {
                processIO.wchar = value;
            } else if (type == "syscr") {
                processIO.syscr = value;
            } else if (type == "syscw") {
                processIO.syscw = value;
            } else if (type == "read_bytes") {
                processIO.readBytes = value;
            } else if (type == "cancelled_write_bytes") {
                processIO.cancelledWriteBytes = value;
            } else if (type == "write_bytes") {
                processIO.writeBytes = value;
            }
        }
    }
    result.retCode = UcError::SUCCESS;
    return result;
}

static void GetDirRegexFiles(const std::string& path, const std::string& pattern,
    std::vector<std::string>& files)
{
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("failed to open dir=%{public}s", path.c_str());
        return;
    }
    std::regex reg = std::regex(pattern);
    while (true) {
        struct dirent* ptr = readdir(dir);
        if (ptr == nullptr) {
            break;
        }
        if (ptr->d_type == DT_REG) {
            if (regex_match(ptr->d_name, reg)) {
                files.push_back(FileUtil::IncludeTrailingPathDelimiter(path) + std::string(ptr->d_name));
            }
        }
    }
    closedir(dir);
    std::sort(files.begin(), files.end());
}

static int GetFileNameNum(const std::string& fileName)
{
    int ret = 0;
    auto startPos = fileName.find(UNDERLINE);
    if (startPos == std::string::npos) {
        return ret;
    }
    auto endPos = fileName.find(EXPORT_FILE_SUFFIX);
    if (endPos == std::string::npos) {
        return ret;
    }
    if (endPos <= startPos + 1) {
        return ret;
    }
    return StringUtil::StrToInt(fileName.substr(startPos + 1, endPos - startPos - 1));
}

std::string IoCollectorImpl::CreateExportFileName(const std::string& filePrefix)
{
    std::unique_lock<std::mutex> lock(exportFileMutex_);
    if (!FileUtil::IsDirectory(COLLECTION_IO_PATH) && !FileUtil::ForceCreateDirectory(COLLECTION_IO_PATH)) {
        HIVIEW_LOGE("failed to create dir=%{public}s", COLLECTION_IO_PATH.c_str());
        return "";
    }

    std::vector<std::string> files;
    GetDirRegexFiles(COLLECTION_IO_PATH, filePrefix + EXPORT_FILE_REGEX, files);
    if (files.size() >= MAX_FILE_NUM) {
        for (size_t index = 0; index <= files.size() - MAX_FILE_NUM; ++index) {
            HIVIEW_LOGI("remove file=%{public}s", files[index].c_str());
            (void)FileUtil::RemoveFile(files[index]);
        }
    }

    uint64_t fileTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    std::string timeFormat = TimeUtil::TimestampFormatToDate(fileTime, "%Y%m%d%H%M%S");
    std::string fileName;
    fileName.append(COLLECTION_IO_PATH).append(filePrefix).append(timeFormat);
    if (!files.empty()) {
        auto startPos = files.back().find(timeFormat);
        if (startPos != std::string::npos) {
            int fileNameNum = GetFileNameNum(files.back().substr(startPos)); // yyyymmddHHMMSS_1.txt
            fileName.append(UNDERLINE).append(std::to_string(++fileNameNum));
        }
    }
    fileName.append(EXPORT_FILE_SUFFIX);
    (void)FileUtil::CreateFile(fileName);
    HIVIEW_LOGI("create file=%{public}s", fileName.c_str());
    return fileName;
}

CollectResult<std::string> IoCollectorImpl::CollectRawDiskStats()
{
    CollectResult<std::string> result;
    result.retCode = UcError::UNSUPPORT;

    std::string fileName = CreateExportFileName(RAW_DISK_STATS_FILE_PREFIX);
    if (fileName.empty()) {
        return result;
    }
    int ret = FileUtil::CopyFile(PROC_DISKSTATS, fileName);
    if (ret != 0) {
        HIVIEW_LOGE("copy /proc/diskstats to file=%{public}s failed.", fileName.c_str());
        return result;
    }

    result.data = fileName;
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::vector<DiskStats>> IoCollectorImpl::CollectDiskStats(DiskStatsFilter filter, bool isUpdate)
{
    CollectResult<std::vector<DiskStats>> result;
    GetDiskStats(filter, isUpdate, result.data);
    HIVIEW_LOGI("collect disk stats size=%{public}zu, isUpdate=%{public}d", result.data.size(), isUpdate);
    result.retCode = UcError::SUCCESS;
    return result;
}

void IoCollectorImpl::GetDiskStats(DiskStatsFilter filter, bool isUpdate, std::vector<DiskStats>& diskStats)
{
    std::unique_lock<std::mutex> lock(collectDiskMutex_);
    uint64_t currCollectDiskTime = TimeUtil::GetMilliseconds();
    uint64_t period = (currCollectDiskTime > preCollectDiskTime_) ?
        ((currCollectDiskTime - preCollectDiskTime_) / TimeUtil::SEC_TO_MILLISEC) : 0;
    if (period > DISK_STATS_PERIOD) {
        if (isUpdate) {
            preCollectDiskTime_ = currCollectDiskTime;
        }
        CalculateDiskStats(period, isUpdate);
    }

    for (auto it = diskStatsMap_.begin(); it != diskStatsMap_.end();) {
        if (it->second.collectTime == preCollectDiskTime_) {
            if (!it->second.stats.deviceName.empty() && !filter(it->second.stats)) {
                diskStats.push_back(it->second.stats);
            }
            ++it;
        } else {
            it = diskStatsMap_.erase(it);
        }
    }
    return;
}

void IoCollectorImpl::CalculateDiskStats(uint64_t period, bool isUpdate)
{
    std::string content;
    if (!FileUtil::LoadStringFromFile(PROC_DISKSTATS, content) || content.empty()) {
        HIVIEW_LOGE("load file=%{public}s failed.", PROC_DISKSTATS.c_str());
        return;
    }
    std::vector<std::string> contents;
    OHOS::SplitStr(content, "\n", contents);
    for (const std::string& line : contents) {
        std::vector<std::string> items;
        StringUtil::SplitStr(line, " ", items);
        if (items.size() < DISK_STATS_SIZE) {
            HIVIEW_LOGE("items num=%{public}zu.", items.size());
            continue;
        }
        std::string deviceName = items[2]; // 2 : index of device name
        if (deviceName.empty()) {
            HIVIEW_LOGE("device name empty.");
            continue;
        }
        DiskData currData;
        currData.operRead = StringUtil::StringToUl(items[4]);    // 4 : index of reads merged
        currData.sectorRead = StringUtil::StringToUl(items[5]);  // 5 : index of sectors read
        currData.readTime = StringUtil::StringToUl(items[6]);    // 6 : index of time spent reading (ms)
        currData.operWrite = StringUtil::StringToUl(items[8]);   // 8 : index of writes merged
        currData.sectorWrite = StringUtil::StringToUl(items[9]); // 9 : index of sectors written
        currData.writeTime = StringUtil::StringToUl(items[10]);  // 10 : index of time spent reading (ms)
        currData.ioWait = StringUtil::StringToUl(items[11]);     // 11 : index of I/Os currently in progress

        CalculateDeviceDiskStats(currData, deviceName, period);
        if (isUpdate) {
            diskStatsMap_[deviceName].collectTime = preCollectDiskTime_;
            diskStatsMap_[deviceName].preData = currData;
        }
    }
}

void IoCollectorImpl::CalculateDeviceDiskStats(const DiskData& currData, const std::string& deviceName, uint64_t period)
{
    if (diskStatsMap_.find(deviceName) == diskStatsMap_.end()) {
        return;
    }

    DiskStatsDevice& device = diskStatsMap_[deviceName];
    DiskData& preData = device.preData;
    DiskStats& stats = device.stats;
    stats.deviceName = deviceName;
    if (period != 0) {
        stats.sectorReadRate = IoCalculator::PercentValue(preData.sectorRead, currData.sectorRead, period);
        stats.sectorWriteRate = IoCalculator::PercentValue(preData.sectorWrite, currData.sectorWrite, period);
        stats.operReadRate = IoCalculator::PercentValue(preData.operRead, currData.operRead, period);
        stats.operWriteRate = IoCalculator::PercentValue(preData.operWrite, currData.operWrite, period);
        stats.readTimeRate = IoCalculator::PercentValue(preData.readTime, currData.readTime, period);
        stats.writeTimeRate = IoCalculator::PercentValue(preData.writeTime, currData.writeTime, period);
        stats.ioWait = currData.ioWait;
    }
}

CollectResult<std::string> IoCollectorImpl::ExportDiskStats(DiskStatsFilter filter)
{
    CollectResult<std::string> result;
    result.retCode = UcError::UNSUPPORT;

    std::vector<DiskStats> diskStats;
    GetDiskStats(filter, false, diskStats);
    std::sort(diskStats.begin(), diskStats.end(), [](const DiskStats &leftStats, const DiskStats &rightStats) {
        return leftStats.deviceName < rightStats.deviceName;
    });

    std::string fileName = CreateExportFileName(DISK_STATS_FILE_PREFIX);
    if (fileName.empty()) {
        return result;
    }
    int fd = open(fileName.c_str(), O_WRONLY);
    if (fd < 0) {
        HIVIEW_LOGE("create fileName=%{public}s failed.", fileName.c_str());
        return result;
    }
    dprintf(fd, "%-13s\t%20s\t%20s\t%20s\t%20s\t%12s\t%12s\t%12s\n", "device", "sectorReadRate/s", "sectorWriteRate/s",
        "operReadRate/s", "operWriteRate/s", "readTime", "writeTime", "ioWait");
    for (auto &stats : diskStats) {
        dprintf(fd, "%-13s\t%12.2f\t%12.2f\t%12.2f\t%12.2f\t%12.4f\t%12.4f\t%12" PRIu64 "\n",
            stats.deviceName.c_str(), stats.sectorReadRate, stats.sectorWriteRate, stats.operReadRate,
            stats.operWriteRate, stats.readTimeRate, stats.writeTimeRate, stats.ioWait);
    }
    close(fd);

    result.retCode = UcError::SUCCESS;
    result.data = fileName;
    return result;
}

void IoCollectorImpl::ReadEMMCInfo(const std::string& path, std::vector<EMMCInfo>& mmcInfos)
{
    EMMCInfo mmcInfo;
    mmcInfo.type = FileUtil::GetFirstLine(path + "/type");
    if (mmcInfo.type.empty()) {
        HIVIEW_LOGE("load file=%{public}s/type failed.", path.c_str());
        return;
    }
    mmcInfo.csd = FileUtil::GetFirstLine(path + "/csd");
    mmcInfo.name = FileUtil::GetFirstLine(path + "/name");
    if (mmcInfo.name.empty()) {
        HIVIEW_LOGE("load file=%{public}s/name failed.", path.c_str());
        return;
    }
    mmcInfo.size = IoCalculator::GetEMMCSize(path);
    if (mmcInfo.size == -1) {
        return;
    }
    mmcInfo.manfid = IoCalculator::GetEMMCManfid(path);
    if (mmcInfo.manfid.empty()) {
        return;
    }
    mmcInfos.emplace_back(mmcInfo);
}

std::string IoCollectorImpl::GetEMMCPath(const std::string& path)
{
    std::string mmcPath;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("open dir=%{public}s failed.", path.c_str());
        return mmcPath;
    }
    struct dirent *de = nullptr;
    while ((de = readdir(dir)) != nullptr) {
        if ((de->d_type == DT_LNK) || (de->d_type == DT_DIR)) {
            std::string fileName = std::string(de->d_name);
            if (fileName.length() <= MMC.length()) {
                continue;
            }
            if (fileName.substr(0, MMC.length()) != MMC) {
                continue;
            }
            if (fileName.find(":") == std::string::npos) {
                continue;
            }
            // mmc0:0001
            mmcPath = path + "/" + fileName + "/cid";
            if (FileUtil::FileExists(mmcPath)) {
                mmcPath = path + "/" + fileName;
            } else {
                mmcPath = "";
            }
            break;
        }
    }
    closedir(dir);
    return mmcPath;
}

void IoCollectorImpl::CalculateEMMCInfo(std::vector<EMMCInfo>& mmcInfos)
{
    const std::string procBootDevice = "/proc/bootdevice";
    ReadEMMCInfo(procBootDevice, mmcInfos);

    const std::string mmcHostPath = "/sys/class/mmc_host";
    DIR *dir = opendir(mmcHostPath.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("open dir=%{public}s failed.", mmcHostPath.c_str());
        return;
    }
    struct dirent *de = nullptr;
    while ((de = readdir(dir)) != nullptr) {
        if ((de->d_type == DT_LNK) || (de->d_type == DT_DIR)) {
            if ((strlen(de->d_name) <= MMC.length()) || (de->d_name[0] != 'm')) {
                continue;
            }
            // mmc0
            std::string mmcPath = mmcHostPath + "/" + std::string(de->d_name);
            mmcPath = GetEMMCPath(mmcPath);
            if (!mmcPath.empty()) {
                ReadEMMCInfo(mmcPath, mmcInfos);
            }
        }
    }
    closedir(dir);
}

CollectResult<std::vector<EMMCInfo>> IoCollectorImpl::CollectEMMCInfo()
{
    CollectResult<std::vector<EMMCInfo>> result;
    CalculateEMMCInfo(result.data);
    HIVIEW_LOGI("collect emmc info size=%{public}zu", result.data.size());
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::string> IoCollectorImpl::ExportEMMCInfo()
{
    CollectResult<std::string> result;
    result.retCode = UcError::UNSUPPORT;

    std::vector<EMMCInfo> mmcInfos;
    CalculateEMMCInfo(mmcInfos);
    std::sort(mmcInfos.begin(), mmcInfos.end(), [](const EMMCInfo &leftInfo, const EMMCInfo &rightInfo) {
        return leftInfo.name < rightInfo.name;
    });

    std::string fileName = CreateExportFileName(EMMC_INFO_FILE_PREFIX);
    if (fileName.empty()) {
        return result;
    }
    int fd = open(fileName.c_str(), O_WRONLY);
    if (fd < 0) {
        HIVIEW_LOGE("open file=%{public}s failed.", fileName.c_str());
        return result;
    }
    dprintf(fd, "%-15s\t%15s\t%15s\t%15s\t%15s\n", "name", "manfid", "csd", "type", "capacity(GB)");
    for (auto &mmcInfo : mmcInfos) {
        dprintf(fd, "%-15s\t%-12s\t%-35s\t%-12s\t%12.2f\n", mmcInfo.name.c_str(), mmcInfo.manfid.c_str(),
            mmcInfo.csd.c_str(), mmcInfo.type.c_str(), static_cast<double>(mmcInfo.size) / EMMC_INFO_SIZE_RATIO);
    }
    close(fd);

    result.retCode = UcError::SUCCESS;
    result.data = fileName;
    return result;
}

void IoCollectorImpl::GetProcIoStats(std::vector<ProcessIoStats>& allProcIoStats, bool isUpdate)
{
    std::unique_lock<std::mutex> lock(collectProcIoMutex_);
    currCollectProcIoTime_ = TimeUtil::GetMilliseconds();
    uint64_t period = (currCollectProcIoTime_ > preCollectProcIoTime_) ?
        ((currCollectProcIoTime_ - preCollectProcIoTime_) / TimeUtil::SEC_TO_MILLISEC) : 0;
    if (period > PROC_IO_STATS_PERIOD) {
        CalculateAllProcIoStats(period, isUpdate);
        if (isUpdate) {
            preCollectProcIoTime_ = currCollectProcIoTime_;
        }
    }

    for (auto it = procIoStatsMap_.begin(); it != procIoStatsMap_.end();) {
        if (it->second.collectTime == preCollectProcIoTime_) {
            if (it->second.stats.pid != 0 && !ProcIoStatsFilter(it->second.stats)) {
                allProcIoStats.push_back(it->second.stats);
            }
            ++it;
        } else {
            it = procIoStatsMap_.erase(it);
        }
    }
}

void IoCollectorImpl::CalculateAllProcIoStats(uint64_t period, bool isUpdate)
{
    DIR *dir = opendir(PROC);
    if (dir == nullptr) {
        HIVIEW_LOGE("open dir=%{public}s failed.", PROC);
        return;
    }

    struct dirent *de = nullptr;
    while ((de = readdir(dir)) != nullptr) {
        if (de->d_type != DT_DIR) {
            continue;
        }
        int32_t pid = StringUtil::StrToInt(std::string(de->d_name));
        if (pid <= 0) {
            continue;
        }
        auto collectProcIoResult = CollectProcessIo(pid);
        if (collectProcIoResult.retCode == UcError::SUCCESS) {
            CalculateProcIoStats(collectProcIoResult.data, pid, period);
            if (isUpdate) {
                procIoStatsMap_[pid].collectTime = currCollectProcIoTime_;
                procIoStatsMap_[pid].preData = collectProcIoResult.data;
            }
        }
    }
    closedir(dir);
}

bool IoCollectorImpl::ProcIoStatsFilter(const ProcessIoStats& stats)
{
    return (stats.rcharRate == 0 && stats.wcharRate == 0 && stats.syscrRate == 0 && stats.syscwRate == 0 &&
        stats.readBytesRate == 0 && stats.writeBytesRate == 0);
}

int32_t IoCollectorImpl::GetProcStateInCollectionPeriod(int32_t pid)
{
    ProcessState procState = ProcessStatus::GetInstance().GetProcessState(pid);
    if (procState == FOREGROUND) {
        return static_cast<int32_t>(FOREGROUND);
    }
    uint64_t procForegroundTime = ProcessStatus::GetInstance().GetProcessLastForegroundTime(pid);
    if (procForegroundTime >= preCollectProcIoTime_) {
        return static_cast<int32_t>(FOREGROUND);
    }
    return static_cast<int32_t>(procState);
}

void IoCollectorImpl::CalculateProcIoStats(const ProcessIo& currData, int32_t pid, uint64_t period)
{
    if (procIoStatsMap_.find(pid) == procIoStatsMap_.end()) {
        return;
    }

    ProcessIoStatsInfo& statsInfo = procIoStatsMap_[pid];
    ProcessIo& preData = statsInfo.preData;
    ProcessIoStats& stats = statsInfo.stats;
    stats.pid = pid;
    stats.name = ProcessStatus::GetInstance().GetProcessName(pid);
    stats.ground = GetProcStateInCollectionPeriod(pid);
    if (period != 0) {
        stats.rcharRate = IoCalculator::PercentValue(preData.rchar, currData.rchar, period);
        stats.wcharRate = IoCalculator::PercentValue(preData.wchar, currData.wchar, period);
        stats.syscrRate = IoCalculator::PercentValue(preData.syscr, currData.syscr, period);
        stats.syscwRate = IoCalculator::PercentValue(preData.syscw, currData.syscw, period);
        stats.readBytesRate = IoCalculator::PercentValue(preData.readBytes, currData.readBytes, period);
        stats.writeBytesRate = IoCalculator::PercentValue(preData.writeBytes, currData.writeBytes, period);
    }
}

CollectResult<std::vector<ProcessIoStats>> IoCollectorImpl::CollectAllProcIoStats(bool isUpdate)
{
    CollectResult<std::vector<ProcessIoStats>> result;
    GetProcIoStats(result.data, isUpdate);
    HIVIEW_LOGI("collect process io stats size=%{public}zu, isUpdate=%{public}d", result.data.size(), isUpdate);
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::string> IoCollectorImpl::ExportAllProcIoStats()
{
    CollectResult<std::string> result;
    result.retCode = UcError::UNSUPPORT;

    std::vector<ProcessIoStats> allProcIoStats;
    GetProcIoStats(allProcIoStats, false);
    std::sort(allProcIoStats.begin(), allProcIoStats.end(),
        [](const ProcessIoStats &leftStats, const ProcessIoStats &rightStats) {
            return leftStats.name < rightStats.name;
        });

    std::string fileName = CreateExportFileName(PROC_IO_STATS_FILE_PREFIX);
    if (fileName.empty()) {
        return result;
    }
    int fd = open(fileName.c_str(), O_WRONLY);
    if (fd < 0) {
        HIVIEW_LOGE("open file=%{public}s failed.", fileName.c_str());
        return result;
    }
    dprintf(fd, "%-13s\t%12s\t%12s\t%12s\t%12s\t%12s\t%12s\t%20s\t%20s\n", "pid", "pname", "fg/bg",
        "rchar/s", "wchar/s", "syscr/s", "syscw/s", "readBytes/s", "writeBytes/s");
    for (auto &procIoStats : allProcIoStats) {
        dprintf(fd, "%-12d\t%12s\t%12d\t%12.2f\t%12.2f\t%12.2f\t%12.2f\t%12.2f\t%12.2f\n",
            procIoStats.pid, procIoStats.name.c_str(), procIoStats.ground, procIoStats.rcharRate, procIoStats.wcharRate,
            procIoStats.syscrRate, procIoStats.syscwRate, procIoStats.readBytesRate, procIoStats.writeBytesRate);
    }
    close(fd);

    result.retCode = UcError::SUCCESS;
    result.data = fileName;
    return result;
}

CollectResult<SysIoStats> IoCollectorImpl::CollectSysIoStats()
{
    CollectResult<SysIoStats> result;
    std::vector<ProcessIoStats> allProcIoStats;
    GetProcIoStats(allProcIoStats, false);

    auto &sysIoStats = result.data;
    for (auto &procIoStats : allProcIoStats) {
        sysIoStats.rcharRate += procIoStats.rcharRate;
        sysIoStats.wcharRate += procIoStats.wcharRate;
        sysIoStats.syscrRate += procIoStats.syscrRate;
        sysIoStats.syscwRate += procIoStats.syscwRate;
        sysIoStats.readBytesRate += procIoStats.readBytesRate;
        sysIoStats.writeBytesRate += procIoStats.writeBytesRate;
    }
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::string> IoCollectorImpl::ExportSysIoStats()
{
    CollectResult<std::string> result;
    result.retCode = UcError::UNSUPPORT;
    auto collectSysIoStatsResult = CollectSysIoStats();
    if (collectSysIoStatsResult.retCode != UcError::SUCCESS) {
        return result;
    }

    std::string fileName = CreateExportFileName(SYS_IO_STATS_FILE_PREFIX);
    if (fileName.empty()) {
        return result;
    }
    int fd = open(fileName.c_str(), O_WRONLY);
    if (fd < 0) {
        HIVIEW_LOGE("open file=%{public}s failed.", fileName.c_str());
        return result;
    }
    dprintf(fd, "%-12s\t%12s\t%12s\t%12s\t%20s\t%20s\n",
        "rchar/s", "wchar/s", "syscr/s", "syscw/s", "readBytes/s", "writeBytes/s");
    auto &sysIoStats = collectSysIoStatsResult.data;
    dprintf(fd, "%-12.2f\t%12.2f\t%12.2f\t%12.2f\t%12.2f\t%12.2f\n", sysIoStats.rcharRate, sysIoStats.wcharRate,
        sysIoStats.syscrRate, sysIoStats.syscwRate, sysIoStats.readBytesRate, sysIoStats.writeBytesRate);
    close(fd);

    result.retCode = UcError::SUCCESS;
    result.data = fileName;
    return result;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
