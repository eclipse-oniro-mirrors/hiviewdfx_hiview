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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_FOLD_EVENT_CACHER_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_FOLD_EVENT_CACHER_H

#include <memory>
#include <map>
#include <vector>

#include "app_mgr_interface.h"
#include "fold_app_usage_db_helper.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
class FoldEventCacher {
public:
    FoldEventCacher(const std::string& workPath);
    ~FoldEventCacher() {}

    void ProcessEvent(std::shared_ptr<SysEvent> event);
    void TimeOut();
private:
    void ProcessForegroundEvent(std::shared_ptr<SysEvent> event, AppEventRecord& appEventRecord);
    void ProcessBackgroundEvent(std::shared_ptr<SysEvent> event, AppEventRecord& appEventRecord);
    void ProcessSceenStatusChangedEvent(std::shared_ptr<SysEvent> event);
    void CountLifeCycleDuration(AppEventRecord& appEventRecord);
    void UpdateFoldStatus(int status);
    void UpdateVhMode(int mode);
    int GetStartIndex(const std::string& bundleName);
    void CalCulateDuration(uint64_t dayStartTime, std::vector<AppEventRecord>& events,
        std::map<int, uint64_t>& durations);
    bool CanCalcDuration(uint32_t preId, uint32_t id);
    void Accumulative(int foldStatus, uint64_t duration, std::map<int, uint64_t>& durations);
    int64_t GetFoldStatusDuration(const int foldStatus, std::map<int, uint64_t>& durations);
    void ProcessCountDurationEvent(AppEventRecord& appEventRecord, std::map<int, uint64_t>& durations);

private:
    std::unique_ptr<FoldAppUsageDbHelper> dbHelper_;
    std::map<std::string, std::string> foregroundApps_;
    int foldStatus_ = 0;
    int vhMode_ = 0;
    uint64_t timelyStart_ = 0;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_EVENT_CACHER_H
