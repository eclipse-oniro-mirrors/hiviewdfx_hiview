/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "trace_decorator.h"

#include "file_util.h"
#include "string_util.h"
#include "trace_utils.h"
#include "decorator_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
const std::string TRACE_COLLECTOR_NAME = "TraceCollector";
const std::string UC_HITRACE_API_STAT_TITLE = "Hitrace API detail statistics:";
const std::string UC_HITRACE_API_STAT_ITEM =
    "Caller FailCall OverCall TotalCall AvgLatency(us) MaxLatency(us) TotalTimeSpent(us)";
const std::string UC_HITRACE_COMPRESS_RATIO = "Hitrace Traffic Compress Ratio:";
const std::string UC_HITRACE_TRAFFIC_STAT_TITLE = "Hitrace Traffic statistics:";
const std::string UC_HITRACE_TRAFFIC_STAT_ITEM =
    "Caller TraceFile RawSize(b) TimeSpent(us) TimeStamp(us)";
const std::string UC_HITRACE_TRAFFIC_ZIP_TITLE = "Hitrace Zip Traffic statistics:";
const std::string UC_HITRACE_TRAFFIC_ZIP_ITEM = "Caller TraceFile ZipSize(b)";
const std::string TRACE_TRAFFIC_LOG_PATH = "/data/log/hiview/unified_collection/ucollection_trace_traffic.log";

TraceStatWrapper TraceDecorator::traceStatWrapper_;

uint64_t GetRawTraceSize(const std::string &file)
{
    std::string originTracePath;
    if (StringUtil::EndWith(file, ".zip")) {
        std::string fileNameWithoutVersion = StringUtil::GetRleftSubstr(FileUtil::ExtractFileName(file), "@");
        originTracePath = "/data/log/hitrace/" + fileNameWithoutVersion + ".sys";
    } else {
        std::string fileNameWithoutPrefix = StringUtil::GetRightSubstr(FileUtil::ExtractFileName(file), "_");
        originTracePath = "/data/log/hitrace/" + fileNameWithoutPrefix;
    }
    std::string realPath;
    if (!FileUtil::PathToRealPath(originTracePath, realPath)) {
        return 0;
    }
    return FileUtil::GetFileSize(realPath);
}

CollectResult<std::vector<std::string>> TraceDecorator::DumpTrace(UCollect::TraceCaller caller)
{
    auto task = [this, &caller] { return traceCollector_->DumpTrace(caller); };
    return Invoke(task, caller);
}

CollectResult<std::vector<std::string>> TraceDecorator::DumpTraceWithDuration(UCollect::TraceCaller caller,
    uint32_t timeLimit, uint64_t happenTime)
{
    auto task = [this, &caller, &timeLimit, &happenTime] {
        return traceCollector_->DumpTraceWithDuration(caller, timeLimit, happenTime);
    };
    return Invoke(task, caller);
}

CollectResult<std::vector<std::string>> TraceDecorator::DumpTraceWithFilter(UCollect::TeleModule module,
    uint32_t timeLimit, uint64_t happenTime)
{
    return traceCollector_->DumpTraceWithFilter(module, timeLimit, happenTime);
}

CollectResult<int32_t> TraceDecorator::FilterTraceOn(UCollect::TeleModule module, uint64_t postTime)
{
    return traceCollector_->FilterTraceOn(module, postTime);
}

CollectResult<int32_t> TraceDecorator::FilterTraceOff(UCollect::TeleModule module)
{
    return traceCollector_->FilterTraceOff(module);
}

void TraceDecorator::SaveStatSpecialInfo()
{
    WriteLinesToFile({""}, false, UC_STAT_LOG_PATH); // a blank line after common stat info
    std::map<std::string, TraceStatInfo> traceStatInfo = traceStatWrapper_.GetTraceStatInfo();
    std::list<std::string> traceFormattedStatInfo = {UC_HITRACE_API_STAT_TITLE, UC_HITRACE_API_STAT_ITEM};
    for (const auto& record : traceStatInfo) {
        traceFormattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(traceFormattedStatInfo, true, UC_STAT_LOG_PATH);
    std::map<std::string, std::vector<TraceTrafficInfo>> traceTrafficInfo = traceStatWrapper_.GetTrafficStatInfo();
    std::list<std::string> trafficFormattedInfo = {UC_HITRACE_TRAFFIC_STAT_TITLE, UC_HITRACE_TRAFFIC_STAT_ITEM};
    for (const auto& record : traceTrafficInfo) {
        std::transform(record.second.begin(), record.second.end(), std::back_inserter(trafficFormattedInfo),
            [] (const TraceTrafficInfo& info) { return info.ToString(); });
    }
    WriteLinesToFile(trafficFormattedInfo, false, UC_STAT_LOG_PATH);
}

void TraceDecorator::ResetStatInfo()
{
    traceStatWrapper_.ResetStatInfo();
}

void TraceDecorator::WriteTrafficAfterZip(const std::string& caller, const std::string& traceZipFile)
{
    std::string str;
    str.append(caller).append(" ")
    .append(traceZipFile).append(" ")
    .append(std::to_string(FileUtil::GetFileSize(traceZipFile)));
    traceStatWrapper_.WriteZipTrafficToLogFile(str);
}

void TraceStatWrapper::UpdateTraceStatInfo(uint64_t startTime, uint64_t endTime, UCollect::TraceCaller& caller,
    const CollectResult<std::vector<std::string>>& result)
{
    bool isCallSucc = (result.retCode == UCollect::UcError::SUCCESS);
    bool isOverCall = (result.retCode == UCollect::UcError::TRACE_OVER_FLOW);
    uint64_t latency = (endTime - startTime > 0) ? (endTime - startTime) : 0;
    std::string callerStr = EnumToString(caller);
    TraceStatItem item = {.caller = callerStr, .isCallSucc = isCallSucc,
        .isOverCall = isOverCall, .latency = latency};
    UpdateAPIStatInfo(item);
    UpdateTrafficInfo(callerStr, latency, result);
}

void TraceStatWrapper::UpdateAPIStatInfo(const TraceStatItem& item)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    if (traceStatInfos_.find(item.caller) == traceStatInfos_.end()) {
        TraceStatInfo statInfo = {
            .caller = item.caller,
            .failCall = (item.isCallSucc || item.isOverCall) ? 0 : 1,
            .overCall = item.isOverCall ? 1 : 0,
            .totalCall = 1,
            .avgLatency = item.latency,
            .maxLatency = item.latency,
            .totalTimeSpend = item.latency,
        };
        traceStatInfos_.insert(std::make_pair(item.caller, statInfo));
        return;
    }
    
    TraceStatInfo& statInfo = traceStatInfos_[item.caller];
    statInfo.totalCall += 1;
    statInfo.failCall += ((item.isCallSucc || item.isOverCall) ? 0 : 1);
    statInfo.overCall += (item.isOverCall ? 1 : 0);
    statInfo.totalTimeSpend += item.latency;
    if (statInfo.maxLatency < item.latency) {
        statInfo.maxLatency = item.latency;
    }
    uint32_t succCall = statInfo.totalCall;
    if (succCall > 0) {
        statInfo.avgLatency = statInfo.totalTimeSpend / succCall;
    }
}

void TraceStatWrapper::UpdateTrafficInfo(const std::string& caller, uint64_t latency,
    const CollectResult<std::vector<std::string>>& result)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    std::vector<std::string> traceFiles = result.data;
    if (latency <= 0 || traceFiles.empty()) { // dumptrace fail, no need to update traffic
        return;
    }
    
    uint64_t timeStamp = TimeUtil::GenerateTimestamp();
    uint64_t avgLatency = latency / traceFiles.size();
    for (const auto& file : traceFiles) {
        uint64_t fileSize = GetRawTraceSize(file);
        if (fileSize == 0) {
            continue;
        }
        TraceTrafficInfo statInfo;
        statInfo.caller = caller;
        statInfo.traceFile = file;
        statInfo.timeSpent = avgLatency;
        statInfo.rawSize = fileSize;
        statInfo.timeStamp = timeStamp;

        if (trafficStatInfos_.find(caller) == trafficStatInfos_.end()) {
            trafficStatInfos_[caller] = {statInfo};
        } else {
            std::vector<TraceTrafficInfo>& tmp = trafficStatInfos_[caller];
            tmp.push_back(statInfo);
        }
    }
}

std::map<std::string, TraceStatInfo> TraceStatWrapper::GetTraceStatInfo()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return traceStatInfos_;
}

std::map<std::string, std::vector<TraceTrafficInfo>> TraceStatWrapper::GetTrafficStatInfo()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return trafficStatInfos_;
}

void TraceStatWrapper::WriteZipTrafficToLogFile(const std::string& trafficInfo)
{
    std::list<std::string> trafficFormattedInfo;
    if (date_ != GetCurrentDate() || !FileUtil::FileExists(TRACE_TRAFFIC_LOG_PATH)) {
        WriteLinesToFile({""}, false, TRACE_TRAFFIC_LOG_PATH);
        date_ = GetCurrentDate();
        WriteLinesToFile({UC_STAT_DATE, date_}, true, TRACE_TRAFFIC_LOG_PATH);
        trafficFormattedInfo = {UC_HITRACE_TRAFFIC_ZIP_TITLE, UC_HITRACE_TRAFFIC_ZIP_ITEM};
    }
    trafficFormattedInfo.push_back(trafficInfo);
    WriteLinesToFile(trafficFormattedInfo, false, TRACE_TRAFFIC_LOG_PATH);
}

void TraceStatWrapper::ResetStatInfo()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    traceStatInfos_.clear();
    trafficStatInfos_.clear();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
