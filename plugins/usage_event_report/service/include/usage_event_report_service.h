/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_USAGE_EVENT_REPORT_SERVICE_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_USAGE_EVENT_REPORT_SERVICE_H

#include <string>

#include "logger_event.h"
#include "usage_event_cacher.h"

namespace OHOS {
namespace HiviewDFX {
class UsageEventReportService {
public:
    UsageEventReportService();
    ~UsageEventReportService() {}
    bool ProcessArgsRequest(int argc, char* argv[]);

private:
    void InitWorkPath(const char* path);
    void ReportAppUsage();
    void ReportSysUsage();
    void SaveSysUsage();
    int UpdateCacheSysUsage(std::shared_ptr<LoggerEvent>& cacheUsage, const UsageEventCacher& cacher);
    int UpdateLastSysUsage(std::shared_ptr<LoggerEvent>& nowUsage, const UsageEventCacher& cacher);

private:
    std::string workPath_;
    uint64_t lastReportTime_;
    uint64_t lastSysReportTime_;
}; // UsageEventReportService
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_USAGE_EVENT_REPORT_SERVICE_H
