/*
 * Copyright (C) 2023-2024  Huawei Device Co., Ltd.
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
#ifdef HAS_HIPERF
#include <charconv>

#include "cpu_perf_dump.h"
#include "file_util.h"
#include "time_util.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-CpuPerfDump");
namespace {
const double CPU_MONITOR_THRESHOLD_CPULOAD = 0.6;
const double CPU_MONITOR_THRESHOLD_CPUUSAGE = 0.8;
const double CPU_MONITOR_THRESHOLD_PRECISION = 0.00001;
const int TOP_N_PROCESS = 3;
const int PERF_COLLECT_TIME = 5;
constexpr char HIPERF_LOG_PATH[] = "/data/log/hiperf";
constexpr uint32_t MAX_NUM_OF_PERF_FILES = 7; // save files for one week
constexpr int64_t DUMP_HIPERF_INTERVAL = 15 * 60 * 1000; //ms
constexpr int64_t DUMP_HIPERF_DELAY_TIME = 2 * 60 * 1000; //ms
}

CpuPerfDump::CpuPerfDump()
{
    perfCollector_ = UCollectUtil::PerfCollector::Create();
    if (!FileUtil::FileExists(HIPERF_LOG_PATH)) {
        FileUtil::ForceCreateDirectory(HIPERF_LOG_PATH, FileUtil::FILE_PERM_770);
    }
    systemUpTime_ = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    lastRecordTime_ = systemUpTime_;
    isBetaVersion_ = Parameter::IsBetaVersion();
}

void CpuPerfDump::DumpTopNCpuProcessPerfData()
{
    if (!topNProcs_.empty()) {
        std::string filename = "hiperf-top3-";
        filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
        "%Y%m%d%H%M%S");
        filename += ".data";
        perfCollector_->SetOutputFilename(filename);
        perfCollector_->SetSelectPids(topNProcs_);
        perfCollector_->SetTimeStopSec(PERF_COLLECT_TIME);
        perfCollector_->StartPerf(HIPERF_LOG_PATH);
    }

    lastRecordTime_ = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    TryToAgePerfFiles();
}

bool CpuPerfDump::CompareCpuLoad(const ProcessCpuStatInfo &info1, const ProcessCpuStatInfo &info2)
{
    return info1.cpuLoad > info2.cpuLoad;
}

bool CpuPerfDump::CompareCpuUsage(const ProcessCpuStatInfo &info1, const ProcessCpuStatInfo &info2)
{
    return info1.cpuUsage > info2.cpuUsage;
}

bool CpuPerfDump::CheckRecordInterval()
{
    if (!isBetaVersion_) {
        return false;
    }
    int64_t nowTime = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    if (abs(nowTime - systemUpTime_) < DUMP_HIPERF_DELAY_TIME) {
        return false;
    }
    if (systemUpTime_ != lastRecordTime_ && abs(nowTime - lastRecordTime_) < DUMP_HIPERF_INTERVAL) {
        return false;
    }
    return true;
}

void CpuPerfDump::CheckAndDumpPerfData(std::vector<ProcessCpuStatInfo> &cpuCollectionInfos)
{
    if (!CheckRecordInterval()) {
        return;
    }
    if (cpuCollectionInfos.empty()) {
        return;
    }
    topNProcs_.clear();
    double sumCpuLoad = 0.0;
    double sumCpuUsage = 0.0;
    for (const auto &info : cpuCollectionInfos) {
        sumCpuLoad += info.cpuLoad;
        sumCpuUsage += info.cpuUsage;
    }

    size_t middlePos = (cpuCollectionInfos.size() >= TOP_N_PROCESS) ? TOP_N_PROCESS : cpuCollectionInfos.size();
    if (sumCpuLoad >= CPU_MONITOR_THRESHOLD_CPULOAD + CPU_MONITOR_THRESHOLD_PRECISION) {
        std::partial_sort(cpuCollectionInfos.begin(), cpuCollectionInfos.begin() + middlePos,
                          cpuCollectionInfos.end(), CompareCpuLoad);
    } else if (sumCpuLoad < CPU_MONITOR_THRESHOLD_PRECISION &&
               sumCpuUsage >= CPU_MONITOR_THRESHOLD_CPUUSAGE + CPU_MONITOR_THRESHOLD_PRECISION) {
        std::partial_sort(cpuCollectionInfos.begin(), cpuCollectionInfos.begin() + middlePos,
                          cpuCollectionInfos.end(), CompareCpuUsage);
    } else {
        return;
    }

    for (const auto &info : cpuCollectionInfos) {
        topNProcs_.emplace_back(info.pid);
        if (topNProcs_.size() >= TOP_N_PROCESS) {
            break;
        }
    }

    DumpTopNCpuProcessPerfData();
}

bool CpuPerfDump::NeedCleanPerfFiles(size_t size)
{
    return size > MAX_NUM_OF_PERF_FILES;
}

std::string CpuPerfDump::GetTimestamp(const std::string& fileName)
{
    auto startPos = fileName.find_last_of('-');
    if (startPos == std::string::npos) {
        return "";
    }
    auto endPos = fileName.find_last_of('.');
    if (endPos == std::string::npos) {
        return "";
    }
    if (endPos <= startPos + 1) {
        return "";
    }
    return fileName.substr(startPos + 1, endPos - startPos - 1);
}

bool CpuPerfDump::CompareFilenames(const std::string &name1, const std::string &name2)
{
    std::string timestamp1 = GetTimestamp(name1);
    std::string timestamp2 = GetTimestamp(name2);
    uint64_t time1 = 0;
    uint64_t time2 = 0;
    auto result1 = std::from_chars(timestamp1.c_str(), timestamp1.c_str() + timestamp1.size(), time1);
    if (result1.ec != std::errc()) {
        HIVIEW_LOGW("convert error, timestamp1: %{public}s", timestamp1.c_str());
    }
    auto result2 = std::from_chars(timestamp2.c_str(), timestamp2.c_str() + timestamp2.size(), time2);
    if (result2.ec != std::errc()) {
        HIVIEW_LOGW("convert error, timestamp2: %{public}s", timestamp2.c_str());
    }
    return time1 < time2;
}

void CpuPerfDump::TryToAgePerfFiles()
{
    std::vector<std::string> perfFiles;
    FileUtil::GetDirFiles(HIPERF_LOG_PATH, perfFiles);
    if (!NeedCleanPerfFiles(perfFiles.size())) {
        return;
    }
    std::sort(perfFiles.begin(), perfFiles.end(), CompareFilenames);
    uint32_t numOfCleanFiles = perfFiles.size() - MAX_NUM_OF_PERF_FILES;
    for (size_t i = 0; i < numOfCleanFiles; i++) {
        if (!FileUtil::RemoveFile(perfFiles[i])) {
            HIVIEW_LOGW("failed to delete perf file: %{public}s", perfFiles[i].c_str());
        }
    }
}
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // HAS_HIPERF
