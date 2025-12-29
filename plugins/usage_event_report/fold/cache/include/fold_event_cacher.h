/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "fold_app_usage_db_helper.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
class FoldEventCacher {
public:
    FoldEventCacher(const std::string& workPath, void* handle = nullptr);
    ~FoldEventCacher() {}

    void ProcessEvent(std::shared_ptr<SysEvent> event);
private:
    void ProcessFocusWindowEvent(std::shared_ptr<SysEvent> event);
    void ProcessForegroundEvent(std::shared_ptr<SysEvent> event);
    void ProcessBackgroundEvent(std::shared_ptr<SysEvent> event);
    void ProcessSceenStatusChangedEvent(std::shared_ptr<SysEvent> event);
    void CountLifeCycleDuration(AppEventRecord& appEventRecord);
    void UpdateFoldStatus(int32_t status);
    void UpdateVhMode(int32_t mode);
    int GetStartIndex(const std::string& bundleName);
    void CalCulateDuration(uint64_t dayStartTime, std::vector<AppEventRecord>& events,
        std::map<int, uint64_t>& durations);
    bool CanCalcDuration(uint32_t preId, uint32_t id);
    void Accumulative(int foldStatus, uint64_t duration, std::map<int, uint64_t>& durations);
    void ProcessCountDurationEvent(AppEventRecord& appEventRecord, std::map<int, uint64_t>& durations);
    void UpdateMultiWindowInfos(uint8_t multiNum, const std::string& multiWindow);
    int32_t GetWindowModeOfFocusedApp();

private:
    std::unique_ptr<FoldAppUsageDbHelper> dbHelper_;
    std::pair<std::string, bool> focusedAppPair_;
    std::unordered_map<std::string, int32_t> multiWindowInfos_;
    int32_t foldStatus_ = 0;
    int32_t vhMode_ = 0;
    uint64_t timelyStart_ = 0;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_EVENT_CACHER_H
