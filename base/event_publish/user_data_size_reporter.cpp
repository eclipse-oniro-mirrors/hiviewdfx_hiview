/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "user_data_size_reporter.h"

#include "file_util.h"
#include "hisysevent.h"
#include "hisysevent_c.h"
#include "hiview_logger.h"
#include "time_util.h"
#include "event_publish.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-UserDataSizeReporter");
namespace {
constexpr uint64_t MS_PER_HOUR = 3600 * 1000;
constexpr uint64_t REPORT_LIMIT_H = 24; // 24h
constexpr uint64_t REPORT_LIMIT_MS = REPORT_LIMIT_H * MS_PER_HOUR;
constexpr size_t RECORD_MAX_CNT = 128;

void MayPushBackPath(std::vector<std::string>& paths, const std::string& path)
{
    if (FileUtil::FileExists(path)) {
        paths.push_back(path);
    }
}

std::vector<std::string> GetReportPath(int32_t uid, const std::string& pathHolder, const std::string& eventName)
{
    std::vector<std::string> rtn;
    MayPushBackPath(rtn, FileUtil::GetSandBoxLogPath(uid, pathHolder, "hiappevent"));
    if (eventName == HiAppEvent::EVENT_MAIN_THREAD_JANK) {
        MayPushBackPath(rtn, FileUtil::GetSandBoxLogPath(uid, pathHolder, "watchdog"));
    }
    if (eventName == HiAppEvent::EVENT_RESOURCE_OVERLIMIT) {
        MayPushBackPath(rtn, FileUtil::GetSandBoxLogPath(uid, pathHolder, "resourcelimit"));
    }

    return rtn;
}

std::vector<uint64_t> GetFoldersSize(const std::vector<std::string>& dirs)
{
    std::vector<uint64_t> rtn;
    for (const auto& dir : dirs) {
        rtn.push_back(FileUtil::GetFolderSize(dir));
    }

    return rtn;
}

void DoReport(int32_t uid, const std::string& pathHolder, const std::string& eventName)
{
    std::string componentName = "hiappevent";
    std::string partitionName = "/data";
    uint64_t partitionSize = static_cast<uint64_t>(FileUtil::GetDeviceValidSize(partitionName));
    std::vector<std::string> dirs = GetReportPath(uid, pathHolder, eventName);
    std::vector<uint64_t> dirSizes = GetFoldersSize(dirs);
    uint32_t count = static_cast<uint32_t>(dirs.size());
    char* fileArr[count];
    uint64_t fileSizeArr[count];
    for (uint32_t i = 0; i < count; i++) {
        fileArr[i] = const_cast<char*>(dirs[i].c_str());
        fileSizeArr[i] = dirSizes[i];
    }

    HiSysEventParam componentNamePara = { .name = { "COMPONENT_NAME" }, .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char*>(componentName.c_str()) }, .arraySize = 0};
    HiSysEventParam partitionNamePara = { .name = { "PARTITION_NAME" }, .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char*>(partitionName.c_str()) }, .arraySize = 0};
    HiSysEventParam partitionSizePara = { .name = { "REMAIN_PARTITION_SIZE" }, .t = HISYSEVENT_UINT64,
        .v = { .ui64 = partitionSize }, .arraySize = 0};
    HiSysEventParam filesPathPara = { .name = { "FILE_OR_FOLDER_PATH" }, .t = HISYSEVENT_STRING_ARRAY,
        .v = { .array = fileArr }, .arraySize = count};
    HiSysEventParam filesSizePara = { .name = { "FILE_OR_FOLDER_SIZE" }, .t = HISYSEVENT_UINT64_ARRAY,
        .v = { .array = fileSizeArr }, .arraySize = count};
    HiSysEventParam params[] = {componentNamePara, partitionNamePara, partitionSizePara, filesPathPara, filesSizePara};
    int ret = OH_HiSysEvent_Write(HiviewDFX::HiSysEvent::Domain::FILEMANAGEMENT, "USER_DATA_SIZE",
        HiSysEventEventType::HISYSEVENT_STATISTIC, params, sizeof(params) / sizeof(params[0]));
    if (ret != 0) {
        HIVIEW_LOGE("failed to report USER_DATA_SIZE event, ret: %{public}d", ret);
    }
}
}

void UserDataSizeReporter::ClearOverTimeRecord()
{
    if (reportLimitRecords_.size() <= 0) {
        return;
    }
    uint64_t earliest = reportLimitRecords_[0].second;
    uint64_t now = TimeUtil::GetMilliseconds();
    if (earliest > now) {
        HIVIEW_LOGE("reportlimit lose efficacy by time jump, earliest: %{public}" PRIu64 ", now: %{public}" PRIu64,
            earliest, now);
        reportLimitRecords_.clear();
        return;
    }

    size_t index = 0;
    for (index = 0; index < reportLimitRecords_.size(); index++) {
        if (reportLimitRecords_[index].second + REPORT_LIMIT_MS > now) {
            break;
        }
    }
    HIVIEW_LOGI("remove reportLimitRecords_ cnt: %{public}zu", index);
    reportLimitRecords_.erase(reportLimitRecords_.begin(), reportLimitRecords_.begin() + index);
}

bool UserDataSizeReporter::ShouldReport(const std::string& pathLimmitKey)
{
    auto it = std::find_if(reportLimitRecords_.begin(), reportLimitRecords_.end(),
                           [&](const auto& p) { return p.first == pathLimmitKey; });
    return it == reportLimitRecords_.end();
}

void UserDataSizeReporter::ReportUserDataSize(int32_t uid, const std::string& pathHolder, const std::string& eventName)
{
    std::lock_guard<std::mutex> lock(recordMutex_);
    std::string pathLimmitKey = pathHolder + "_" + eventName;
    ClearOverTimeRecord();
    if (!ShouldReport(pathLimmitKey)) {
        HIVIEW_LOGI("should not report, pathLimmitKey: %{public}s", pathLimmitKey.c_str());
        return;
    }
    HIVIEW_LOGI("should report, pathLimmitKey: %{public}s", pathLimmitKey.c_str());
    DoReport(uid, pathHolder, eventName);
    if (reportLimitRecords_.size() < RECORD_MAX_CNT) {
        reportLimitRecords_.push_back({pathLimmitKey, TimeUtil::GetMilliseconds()});
    }
}
} // namespace HiviewDFX
} // namespace OHOS
