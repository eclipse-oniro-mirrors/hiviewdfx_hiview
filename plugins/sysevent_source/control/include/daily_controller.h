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
#ifndef HIVIEW_PLUGINS_SYS_EVENT_SOURCE_CONTROL_INCLUDE_DAILY_CONTROLLER_H
#define HIVIEW_PLUGINS_SYS_EVENT_SOURCE_CONTROL_INCLUDE_DAILY_CONTROLLER_H

#include "i_controller.h"
#include "daily_config.h"
#include "daily_db_helper.h"

namespace OHOS {
namespace HiviewDFX {
class DailyController : public IController {
public:
    DailyController(const std::string& workPath, const std::string& configPath);
    ~DailyController() = default;
    bool CheckThreshold(std::shared_ptr<SysEvent> event) override;
    void OnConfigUpdate(const std::string& configPath) override;

private:
    typedef std::pair<std::string, std::string> CacheKey;
    struct ControlInfo {
        int32_t threshold = 0;
        int32_t count = 0;
        int64_t exceedTime = 0;
    };

    void TryToUpdateCacheToDb(int64_t nowTime);
    void TryToReportDb(int64_t nowTime);
    bool CheckTimeOfCache(int64_t nowTime);
    bool CheckSizeOfCache();
    void UpdateCacheToDb();
    int32_t GetThreshold(const CacheKey& cachekey, int32_t type);
    int32_t GetCount(const CacheKey& cachekey);
    void UpdateCacheAndDb(const CacheKey& cachekey, int32_t threshold, int32_t count);
    void UpdateCache(const CacheKey& cachekey, int32_t threshold, int32_t count, int64_t exceedTime);
    void UpdateDb(const CacheKey& cachekey, int32_t count, int64_t exceedTime);

private:
    std::unique_ptr<DailyConfig> config_;
    std::unique_ptr<DailyDbHelper> dbHelper_;
    /* <<domain, name>, ControlInfo> */
    std::unordered_map<std::pair<std::string, std::string>, ControlInfo, EventPairHash> cache_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_SYS_EVENT_SOURCE_CONTROL_INCLUDE_DAILY_CONTROLLER_H
