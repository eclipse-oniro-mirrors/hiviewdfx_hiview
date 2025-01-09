/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_INCLUDE_USAGE_EVENT_REPORT_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_INCLUDE_USAGE_EVENT_REPORT_H

#include <memory>
#include <string>

#include "event_loop.h"
#include "ffrt.h"
#include "fold_app_usage_event_factory.h"
#include "fold_event_cacher.h"
#include "plugin.h"
#include "shutdown/iasync_shutdown_callback.h"

namespace OHOS {
namespace HiviewDFX {
class UsageEventReport : public Plugin {
public:
    UsageEventReport();
    ~UsageEventReport() {}
    void OnLoad() override;
    void OnUnload() override;
    bool OnEvent(std::shared_ptr<Event>& event) override;
    bool IsRunning();
    void TimeOut();
    static void SaveEventToDb();

private:
    void Init();
    void InitCallback();
    void Start();
    void Stop();
    void RunTask();
    void ReportDailyEvent();
    void ReportFoldAppUsageEvent();
    void ReportTimeOutEvent();
    void ReportSysUsageEvent();
    void DeletePluginStatsEvents();
    static void StartServiceByOption(const std::string& opt);
    static void SavePluginStatsEvents();
    static void SaveSysUsageEvent();
    void InitFoldEventCacher(const std::string& workPath);

private:
    sptr<PowerMgr::IAsyncShutdownCallback> callback_;
    uint64_t timeOutCnt_;
    ffrt::mutex runningMutex_;
    bool isRunning_;
    static uint64_t lastSysReportTime_;
    static uint64_t lastReportTime_;
    static uint64_t nextReportTime_;
    static std::string workPath_;
    std::unique_ptr<FoldEventCacher> foldEventCacher_;
    std::unique_ptr<FoldAppUsageEventFactory> foldAppUsageFactory_;
}; // UsageEventReport
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_INCLUDE_USAGE_EVENT_REPORT_H
