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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_APP_USAGE_EVENT_FACTORY_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_APP_USAGE_EVENT_FACTORY_H

#include <vector>

#include "logger_event_factory.h"

namespace OHOS {
namespace HiviewDFX {
struct AppUsageInfo {
    AppUsageInfo(const std::string& package, const std::string& version, uint64_t usage,
        const std::string& date, uint32_t startNum)
        : package_(package), version_(version), usage_(usage), date_(date), startNum_(startNum)
    {}
    AppUsageInfo() {}

    std::string package_;
    std::string version_;
    uint64_t usage_ = 0;
    std::string date_;
    uint32_t startNum_ = 0;
};

class AppUsageEventFactory : public LoggerEventFactory {
public:
    AppUsageEventFactory(uint64_t lastReportTime) : lastReportTime_(lastReportTime) {}
    std::unique_ptr<LoggerEvent> Create() override;
    void Create(std::vector<std::unique_ptr<LoggerEvent>>& events) override;

private:
    void GetAllCreatedOsAccountIds(std::vector<int32_t>& ids);
    void GetAppUsageInfosByUserId(std::vector<AppUsageInfo>& infos, int32_t userId);
    std::string GetAppVersion(const std::string& bundleName);

private:
    uint64_t lastReportTime_ = 0;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_APP_USAGE_EVENT_FACTORY_H
