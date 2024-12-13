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
#include <charconv>
#include <chrono>
#include <cinttypes>
#include <ctime>
#include <sys/stat.h>
#include <map>
#include <vector>

#include "app_caller_event.h"
#include "app_event_task_storage.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "string_util.h"
#include "time_util.h"
#include "trace_flow_controller.h"
#include "trace_utils.h"

using namespace OHOS::HiviewDFX;

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
const std::string UNIFIED_SHARE_PATH = "/data/log/hiview/unified_collection/trace/share/";
const std::string UNIFIED_SPECIAL_PATH = "/data/log/hiview/unified_collection/trace/special/";
const std::string DB_PATH = "/data/log/hiview/unified_collection/trace/";
const int64_t XPERF_SIZE = 1750 * 1024 * 1024;
const int64_t XPOWER_SIZE = 700 * 1024 * 1024;
const int64_t RELIABILITY_SIZE = 750 * 1024 * 1024;
const int64_t HIVIEW_SIZE = 350 * 1024 * 1024;
const int64_t FOUNDATION_SIZE = 150 * 1024 * 1024;
const float TEN_PERCENT_LIMIT = 0.1;

int64_t GetActualReliabilitySize()
{
    return  Parameter::IsLaboratoryMode() ? RELIABILITY_SIZE * 5 : RELIABILITY_SIZE; // 5 : laboratory largen 5 times
}

const std::map<UCollect::TraceCaller, std::pair<std::string, int64_t>> TRACE_QUOTA = {
    {UCollect::TraceCaller::XPERF, {"xperf", XPERF_SIZE}},
    {UCollect::TraceCaller::XPOWER, {"xpower", XPOWER_SIZE}},
    {UCollect::TraceCaller::RELIABILITY, {"reliability", GetActualReliabilitySize()}},
    {UCollect::TraceCaller::HIVIEW, {"hiview", HIVIEW_SIZE}},
    {UCollect::TraceCaller::FOUNDATION, {"foundation", FOUNDATION_SIZE}},
};
}

void CreateTracePath(const std::string &filePath)
{
    if (FileUtil::FileExists(filePath)) {
        return;
    }
    if (!CreateMultiDirectory(filePath)) {
        HIVIEW_LOGE("failed to create multidirectory %{public}s.", filePath.c_str());
        return;
    }
}

void TraceFlowController::InitTraceDb()
{
    traceFlowRecord_ = QueryDb();
    HIVIEW_LOGI("systemTime:%{public}s, callerName:%{public}s, usedSize:%{public}" PRId64,
        traceFlowRecord_.systemTime.c_str(), traceFlowRecord_.callerName.c_str(),
        traceFlowRecord_.usedSize);
}

void TraceFlowController::InitTraceStorage()
{
    CreateTracePath(UNIFIED_SHARE_PATH);
    CreateTracePath(UNIFIED_SPECIAL_PATH);

    traceStorage_ = std::make_shared<TraceStorage>(DB_PATH);
}

TraceFlowController::TraceFlowController(UCollect::TraceCaller caller) : caller_(caller)
{
    InitTraceStorage();
    InitTraceDb();
}

bool TraceFlowController::NeedDump()
{
    std::string nowDays = GetDate();
    HIVIEW_LOGI("start to dump, nowDays = %{public}s, systemTime = %{public}s.",
        nowDays.c_str(), traceFlowRecord_.systemTime.c_str());
    if (nowDays != traceFlowRecord_.systemTime) {
        // date changes
        traceFlowRecord_.systemTime = nowDays;
        traceFlowRecord_.usedSize = 0;
        return true;
    }
    return TRACE_QUOTA.find(caller_) != TRACE_QUOTA.end() ?
        traceFlowRecord_.usedSize < TRACE_QUOTA.at(caller_).second : true;
}

bool TraceFlowController::NeedUpload(TraceRetInfo ret)
{
    int64_t traceSize = GetTraceSize(ret);
    HIVIEW_LOGI("start to upload , traceSize = %{public}" PRId64 ".", traceSize);
    if (TRACE_QUOTA.find(caller_) == TRACE_QUOTA.end()) {
        return true;
    }
    if (IsLowerLimit(traceFlowRecord_.usedSize, traceSize, TRACE_QUOTA.at(caller_).second)) {
        traceFlowRecord_.usedSize += traceSize;
        return true;
    }
    return false;
}

bool TraceFlowController::IsLowerLimit(int64_t nowSize, int64_t traceSize, int64_t limitSize)
{
    if (limitSize == 0) {
        HIVIEW_LOGE("error, limit size is zero.");
        return false;
    }

    int64_t totalSize = nowSize + traceSize;
    if (totalSize < limitSize) {
        return true;
    }

    float limit = static_cast<float>(totalSize - limitSize) / limitSize;
    if (limit > TEN_PERCENT_LIMIT) {
        return false;
    }
    return true;
}

void TraceFlowController::StoreDb()
{
    if (TRACE_QUOTA.find(caller_) == TRACE_QUOTA.end()) {
        HIVIEW_LOGI("caller %{public}d not need store", caller_);
        return;
    }
    HIVIEW_LOGI("systemTime:%{public}s, callerName:%{public}s, usedSize:%{public}" PRId64,
        traceFlowRecord_.systemTime.c_str(), traceFlowRecord_.callerName.c_str(),
        traceFlowRecord_.usedSize);
    traceStorage_->Store(traceFlowRecord_);
}

int64_t TraceFlowController::GetTraceSize(TraceRetInfo ret)
{
    struct stat fileInfo;
    int64_t traceSize = 0;
    for (const auto &tracePath : ret.outputFiles) {
        int ret = stat(tracePath.c_str(), &fileInfo);
        if (ret != 0) {
            HIVIEW_LOGE("%{public}s is not exists, ret = %{public}d.", tracePath.c_str(), ret);
            continue;
        }
        traceSize += fileInfo.st_size;
    }
    return traceSize;
}

std::string TraceFlowController::GetDate()
{
    std::string dateStr = TimeUtil::TimestampFormatToDate(std::time(nullptr), "%Y-%m-%d");
    return dateStr;
}

TraceFlowRecord TraceFlowController::QueryDb()
{
    struct TraceFlowRecord tmpTraceFlowRecord;
    if (TRACE_QUOTA.find(caller_) == TRACE_QUOTA.end()) {
        return tmpTraceFlowRecord;
    }
    tmpTraceFlowRecord.callerName = TRACE_QUOTA.at(caller_).first;
    traceStorage_->Query(tmpTraceFlowRecord);
    return tmpTraceFlowRecord;
}

bool TraceFlowController::HasCallOnceToday(int32_t uid, uint64_t happenTime)
{
    uint64_t happenTimeInSecond = happenTime / TimeUtil::SEC_TO_MILLISEC;
    std::string date = TimeUtil::TimestampFormatToDate(happenTimeInSecond, "%Y%m%d");

    AppEventTask appEventTask;
    appEventTask.id_ = 0;
    int32_t dateNum = 0;
    auto result = std::from_chars(date.c_str(), date.c_str() + date.size(), dateNum);
    if (result.ec != std::errc()) {
        HIVIEW_LOGW("convert error, dateStr: %{public}s", date.c_str());
        return false;
    }
    traceStorage_->QueryAppEventTask(uid, dateNum, appEventTask);
    return appEventTask.id_ > 0;
}

bool TraceFlowController::RecordCaller(std::shared_ptr<AppCallerEvent> appEvent)
{
    uint64_t happenTimeInSecond = appEvent->happenTime_ / TimeUtil::SEC_TO_MILLISEC;
    std::string date = TimeUtil::TimestampFormatToDate(happenTimeInSecond, "%Y%m%d");
    int64_t dateNum = 0;
    auto result = std::from_chars(date.c_str(), date.c_str() + date.size(), dateNum);
    if (result.ec != std::errc()) {
        HIVIEW_LOGW("convert error, dateStr: %{public}s", date.c_str());
        return false;
    }
    AppEventTask appEventTask;
    appEventTask.taskDate_ = dateNum;
    appEventTask.taskType_ = APP_EVENT_TASK_TYPE_JANK_EVENT;
    appEventTask.uid_ = appEvent->uid_;
    appEventTask.pid_ = appEvent->pid_;
    appEventTask.bundleName_ = appEvent->bundleName_;
    appEventTask.bundleVersion_ = appEvent->bundleVersion_;
    appEventTask.startTime_ = appEvent->taskBeginTime_;
    appEventTask.finishTime_ = appEvent->taskEndTime_;
    appEventTask.resourePath_ = appEvent->externalLog_;
    appEventTask.resourceSize_ = static_cast<int32_t>(FileUtil::GetFileSize(appEventTask.resourePath_));
    appEventTask.state_ = APP_EVENT_TASK_STATE_FINISH;
    return traceStorage_->StoreAppEventTask(appEventTask);
}

void TraceFlowController::CleanOldAppTrace()
{
    UCollect::TraceCaller caller = UCollect::TraceCaller::APP;
    FileRemove(caller);

    uint64_t timeNow = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    uint32_t secondsOfThreeDays = 3 * TimeUtil::SECONDS_PER_DAY; // 3 : clean data three days ago
    if (timeNow < secondsOfThreeDays) {
        HIVIEW_LOGW("time is invalid");
        return;
    }
    uint64_t timeThreeDaysAgo = timeNow - secondsOfThreeDays;
    std::string dateThreeDaysAgo = TimeUtil::TimestampFormatToDate(timeThreeDaysAgo, "%Y%m%d");
    int32_t dateNum = 0;
    auto result = std::from_chars(dateThreeDaysAgo.c_str(),
        dateThreeDaysAgo.c_str() + dateThreeDaysAgo.size(), dateNum);
    if (result.ec != std::errc()) {
        HIVIEW_LOGW("convert error, dateStr: %{public}s", dateThreeDaysAgo.c_str());
        return;
    }
    traceStorage_->RemoveOldAppEventTask(dateNum);
}
} // HiViewDFX
} // OHOS
