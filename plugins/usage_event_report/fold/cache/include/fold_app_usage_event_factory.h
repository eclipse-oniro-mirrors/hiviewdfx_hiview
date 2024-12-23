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
 */

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_FOLD_APP_USAGE_EVENT_FACTORY_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_FOLD_APP_USAGE_EVENT_FACTORY_H

#include <vector>

#include "fold_app_usage_db_helper.h"
#include "logger_event_factory.h"

namespace OHOS {
namespace HiviewDFX {

class FoldAppUsageEventFactory : public LoggerEventFactory {
public:
    FoldAppUsageEventFactory(const std::string& workPath);
    std::unique_ptr<LoggerEvent> Create() override;
    void Create(std::vector<std::unique_ptr<LoggerEvent>> &events) override;

private:
    void GetForegroundAppNames(std::vector<std::string> &appNames);
    void GetForegroundAppsAtEndTime(std::vector<std::string> &appNames);
    void GetAppUsageInfo(std::vector<FoldAppUsageInfo> &infos);

private:
    uint64_t today0Time_ = 0;
    uint64_t startTime_ = 0;
    uint64_t endTime_ = 0;
    uint64_t clearDataTime_ = 0;
    int foldStatus_ = 0;
    std::unique_ptr<FoldAppUsageDbHelper> dbHelper_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_FOLD_APP_USAGE_EVENT_FACTORY_H