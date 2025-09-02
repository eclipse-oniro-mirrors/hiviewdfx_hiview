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
constexpr char UC_HITRACE_API_STAT_TITLE[] = "Hitrace API detail statistics:";
constexpr char UC_HITRACE_API_STAT_ITEM[] =
    "Caller FailCall OverCall TotalCall AvgLatency(us) MaxLatency(us) TotalTimeSpent(us)";
constexpr char UC_HITRACE_TRAFFIC_TITLE[] = "Hitrace Traffic statistics:";
constexpr char UC_HITRACE_TRAFFIC_ITEM[] =
    "Caller TraceFile RawSize(b) ZipSize(b) HandleTime(us)";
constexpr char TRACE_TRAFFIC_LOG_PATH[] = "/data/log/hiview/unified_collection/ucollection_trace_traffic.log";

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

bool TraceDecorator::RecoverTmpTrace()
{
    return traceCollector_->RecoverTmpTrace();
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
}

void TraceDecorator::ResetStatInfo()
{
    traceStatWrapper_.ResetStatInfo();
}

void TraceDecorator::WriteTrafficAfterHandle(const TraceTrafficInfo& trace_traffic)
{
    if (!Parameter::IsBetaVersion() && !Parameter::IsUCollectionSwitchOn()) {
        return;
    }
    static ffrt::mutex mtx;
    std::lock_guard<ffrt::mutex> lock(mtx);
    traceStatWrapper_.WriteTrafficToLogFile(trace_traffic.ToString());
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
}

void TraceStatWrapper::UpdateAPIStatInfo(const TraceStatItem& item)
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
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

std::map<std::string, TraceStatInfo> TraceStatWrapper::GetTraceStatInfo()
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return traceStatInfos_;
}

void TraceStatWrapper::WriteTrafficToLogFile(const std::string& trafficInfo)
{
    std::list<std::string> trafficFormattedInfo;
    if (date_ != GetCurrentDate() || !FileUtil::FileExists(TRACE_TRAFFIC_LOG_PATH)) {
        WriteLinesToFile({""}, false, TRACE_TRAFFIC_LOG_PATH);
        date_ = GetCurrentDate();
        WriteLinesToFile({UC_STAT_DATE, date_}, true, TRACE_TRAFFIC_LOG_PATH);
        trafficFormattedInfo = {UC_HITRACE_TRAFFIC_TITLE, UC_HITRACE_TRAFFIC_ITEM};
    }
    trafficFormattedInfo.push_back(trafficInfo);
    WriteLinesToFile(trafficFormattedInfo, false, TRACE_TRAFFIC_LOG_PATH);
}

void TraceStatWrapper::ResetStatInfo()
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    traceStatInfos_.clear();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
