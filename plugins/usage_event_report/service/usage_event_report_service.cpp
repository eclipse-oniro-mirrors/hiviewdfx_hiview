/*
 * Copyright (C) 2022-2025 Huawei Device Co., Ltd.
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
#include "usage_event_report_service.h"

#include <algorithm>
#include <getopt.h>
#include <iostream>

#include "app_usage_event_factory.h"
#include "hiview_logger.h"
#include "sys_usage_event_factory.h"
#include "time_util.h"
#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-UsageEventReportService");
using namespace SysUsageEventSpace;
using namespace SysUsageDbSpace;
namespace {
constexpr char ARG_SELECTION[] = "p:t:T:sSAh";
constexpr char DEFAULT_WORK_PATH[] = "/data/log/hiview";
const std::string SYS_USAGE_KEYS[] = { KEY_OF_POWER, KEY_OF_RUNNING };

void PrintHelpInfo()
{
    const std::string helpInfo =
        "usage:\n"
        "  -h             | help text for the tool\n"
        "  -p [path]      | specify the storage path for data, which must be in the /data/log/hiview directory\n"
        "  -s             | storage system usage duration data\n"
        "  -A             | report APP_USAGE event\n"
        "  -S             | report SYS_USAGE event\n"
        "  -t [timestamp] | specify the last report time for APP_USAGE, in milliseconds\n"
        "  -T [timestamp] | specify the last report time for SYS_USAGE, in milliseconds\n";
    std::cout << helpInfo;
}
}

UsageEventReportService::UsageEventReportService() : workPath_(DEFAULT_WORK_PATH), lastReportTime_(0),
    lastSysReportTime_(0)
{}

void UsageEventReportService::InitWorkPath(const char* path)
{
    if (path == nullptr) {
        HIVIEW_LOGE("invalid path, path is nullptr");
        return;
    }
    char realPath[PATH_MAX] = { 0x00 };
    if (strlen(path) >= PATH_MAX || realpath(path, realPath) == nullptr) {
        HIVIEW_LOGE("invalid path, path does not exist");
        return;
    }
    if (std::string realPathStr = realPath; realPathStr.rfind(DEFAULT_WORK_PATH, 0) != 0) {
        HIVIEW_LOGE("invalid path, path should be in the hiview dir");
        return;
    }
    workPath_ = realPath;
}

void UsageEventReportService::ReportAppUsage()
{
    HIVIEW_LOGD("start to report app usage event");
    auto factory = std::make_unique<AppUsageEventFactory>(lastReportTime_);
    std::vector<std::unique_ptr<LoggerEvent>> events;
    factory->Create(events);
    HIVIEW_LOGI("report app usage event num: %{public}zu", events.size());
    for (size_t i = 0; i < events.size(); ++i) {
        events[i]->Report();
    }
}

void UsageEventReportService::ReportSysUsage()
{
    HIVIEW_LOGI("start to report sys usage event");
    UsageEventCacher cacher(workPath_);
    auto cacheUsage = cacher.GetSysUsageEvent();
    if (cacheUsage == nullptr) {
        HIVIEW_LOGE("failed to report sys usage event, cache is null");
        return;
    }
    cacheUsage->Report();

    // after reporting, update cache_usage event to the db
    cacheUsage->Update(KEY_OF_START, cacheUsage->GetValue(KEY_OF_END).GetUint64());
    for (auto key : SYS_USAGE_KEYS) {
        cacheUsage->Update(key, DEFAULT_UINT64);
    }
    (void)cacher.SaveSysUsageEventToDb(cacheUsage);
}

int UsageEventReportService::UpdateCacheSysUsage(std::shared_ptr<LoggerEvent>& cacheUsage,
    const UsageEventCacher& cacher)
{
    std::shared_ptr<LoggerEvent> nowUsage = std::make_unique<SysUsageEventFactory>()->Create();
    auto lastUsage = cacher.GetSysUsageEvent(LAST_SYS_USAGE_TABLE);
    for (auto key : SYS_USAGE_KEYS) {
        uint64_t nowUsageTime = nowUsage->GetValue(key).GetUint64();
        uint64_t lastUsageTime = lastUsage == nullptr ?  0 : lastUsage->GetValue(key).GetUint64();
        uint64_t curUsageTime = nowUsageTime > lastUsageTime ? (nowUsageTime - lastUsageTime) : nowUsageTime;
        cacheUsage->Update(key, curUsageTime + cacheUsage->GetValue(key).GetUint64());
    }
    cacheUsage->Update(KEY_OF_END, nowUsage->GetValue(KEY_OF_END).GetUint64());
    return UpdateLastSysUsage(nowUsage, cacher);
}

int UsageEventReportService::UpdateLastSysUsage(std::shared_ptr<LoggerEvent>& nowUsage,
    const UsageEventCacher& cacher)
{
    nowUsage->Update(KEY_OF_START, lastReportTime_); // save the last report time for app_usage
    return cacher.SaveSysUsageEventToDb(nowUsage, LAST_SYS_USAGE_TABLE);
}

void UsageEventReportService::SaveSysUsage()
{
    HIVIEW_LOGI("start to save sys usage event to db");
    UsageEventCacher cacher(workPath_);
    int ret = 0;
    auto cacheUsage = cacher.GetSysUsageEvent();
    if (cacheUsage == nullptr) {
        // if it is the first save, set the current usage to the cache_usage
        cacheUsage = std::make_unique<SysUsageEventFactory>()->Create();
        ret = UpdateLastSysUsage(cacheUsage, cacher);
        cacheUsage->Update(KEY_OF_START, lastSysReportTime_);
    } else {
        // add the current usage to the cache_usage since the last save
        ret = UpdateCacheSysUsage(cacheUsage, cacher);
    }

    if (ret == 0) { // not 0 means db is abnormal, do not need to save
        cacher.SaveSysUsageEventToDb(cacheUsage);
    }
}

bool UsageEventReportService::ProcessArgsRequest(int argc, char* argv[])
{
    if (argv == nullptr) {
        return false;
    }
    int opt = 0;
    while ((opt = getopt(argc, argv, ARG_SELECTION)) != -1) {
        switch (opt) {
            case 'p':
                InitWorkPath(optarg);
                break;
            case 's':
                SaveSysUsage();
                break;
            case 'A':
                ReportAppUsage();
                break;
            case 'S':
                ReportSysUsage();
                break;
            case 't':
                lastReportTime_ = strtoull(optarg, nullptr, 0);
                break;
            case 'T':
                lastSysReportTime_ = strtoull(optarg, nullptr, 0);
                break;
            case 'h':
                PrintHelpInfo();
                break;
            default:
                break;
        }
    }
    return true;
}
}  // namespace HiviewDFX
}  // namespace OHOS
