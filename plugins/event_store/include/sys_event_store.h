/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_PLUGINS_EVENT_SERVICE_INCLUDE_SYS_EVENT_SERVICE_H
#define HIVIEW_PLUGINS_EVENT_SERVICE_INCLUDE_SYS_EVENT_SERVICE_H

#include <fstream>
#include <list>
#include <memory>

#include "event.h"
#include "period_file_operator.h"
#include "plugin.h"
#include "sys_event_db_mgr.h"

namespace OHOS {
namespace HiviewDFX {
struct StorePeriodInfo {
    // format: YYYYMMDDHH
    std::string timeStamp;

    // count of event which has been stored into db file in 1 hour
    size_t storedCnt = 0;

    StorePeriodInfo(const std::string& timeStamp, size_t storedCnt) : timeStamp(timeStamp), storedCnt(storedCnt) {}
};

class SysEventStore : public Plugin {
public:
    SysEventStore();
    ~SysEventStore() {}

    void OnLoad() override;
    void OnUnload() override;
    bool OnEvent(std::shared_ptr<Event>& event) override;

private:
    std::shared_ptr<SysEvent> Convert2SysEvent(std::shared_ptr<Event>& event);
    bool IsNeedBackup(const std::string& dateStr);
    void StatisticStorePeriodInfo(const std::shared_ptr<SysEvent> event);
    void RecordStorePeriodInfo();

private:
    std::unique_ptr<SysEventDbMgr> sysEventDbMgr_ = nullptr;
    std::atomic<bool> hasLoaded_ { false };
    std::string lastBackupTime_;
    std::list<std::shared_ptr<StorePeriodInfo>> periodInfoList_;
    std::unique_ptr<PeriodInfoFileOperator> periodFileOpt_;
    std::once_flag exportEngineStartFlag_;
}; // SysEventService
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_EVENT_SERVICE_INCLUDE_SYS_EVENT_SERVICE_H
