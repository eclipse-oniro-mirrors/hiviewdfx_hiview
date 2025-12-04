/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef FREEZE_DETECTOR_PLUGIN_H
#define FREEZE_DETECTOR_PLUGIN_H

#include <memory>

#include "event.h"
#include "event_loop.h"
#include "freeze_common.h"
#include "plugin.h"
#include "watch_point.h"
#include "resolver.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {
struct WatchPointParams {
    long seq;
    long pid;
    long tid;
    long uid;
    std::string appRunningUniqueId;
    std::string packageName;
    std::string processName;
    std::string hitraceTime;
    std::string sysrqTime;
    std::string terminalThreadStack;
    std::string telemetryId;
    std::string info;
    std::string hitraceIdInfo;
    std::string procStatm;
    std::string hostResourceWarning;
    std::string freezeExtFile;
    std::string applicationInfo;
    std::string taskName;
    std::string timeoutEventId;
    std::string lastDispatchEventId;
    std::string lastProcessEventId;
    std::string lastMarkedEventId;
    std::string thermalLevel;
    std::string logFile;
    std::string foreGround;
    std::string clusterRaw;
    std::string msg;
};
class FreezeDetectorPlugin : public Plugin {
public:
    FreezeDetectorPlugin();
    ~FreezeDetectorPlugin();
    bool ReadyToLoad() override;
    bool OnEvent(std::shared_ptr<Event> &event) override;
    void OnLoad() override;
    void OnUnload() override;
    bool CanProcessEvent(std::shared_ptr<Event> event) override;
    void OnEventListeningCallback(const Event& msg) override;
    void ScheduleEventProcessing(const WatchPoint &watchpoint, const std::vector<FreezeResult> &freezeResultList);

private:

    std::string RemoveRedundantNewline(const std::string& content) const;
    WatchPoint MakeWatchPoint(const Event& event);
    void ProcessEvent(WatchPoint watchPoint);
    void ExtractWatchPointParams(SysEvent& sysEvent, const Event& event, WatchPointParams& params);

    std::shared_ptr<FreezeCommon> freezeCommon_ = nullptr;
    std::unique_ptr<FreezeResolver> freezeResolver_ = nullptr;
    std::unique_ptr<ffrt::queue> warnQueue_ = nullptr;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGIN_FREEZE_DETECTOR_H
