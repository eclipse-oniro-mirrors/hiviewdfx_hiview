/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef SYS_EVENT_SOURCE_H
#define SYS_EVENT_SOURCE_H

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "event_json_parser.h"
#include "event_server.h"
#include "event_source.h"
#include "i_controller.h"
#include "pipeline.h"
#include "platform_monitor.h"
#include "base/raw_data.h"
#include "sys_event_service_adapter.h"
#include "sys_event_stat.h"

namespace OHOS {
namespace HiviewDFX {
class SysEventSource;

class SysEventReceiver : public EventReceiver {
public:
    explicit SysEventReceiver(SysEventSource& source): eventSource(source) {};
    ~SysEventReceiver() override {};
    void HandlerEvent(std::shared_ptr<EventRaw::RawData> rawData) override;
private:
    SysEventSource& eventSource;
};

class SysEventSource : public EventSource, public SysEventServiceBase {
public:
    SysEventSource() {};
    ~SysEventSource() {};
    void OnLoad() override;
    void OnUnload() override;
    void StartEventSource() override;
    void Recycle(PipelineEvent *event) override;
    void PauseDispatch(std::weak_ptr<Plugin> plugin) override;
    bool CheckEvent(std::shared_ptr<Event> event);
    bool PublishPipelineEvent(std::shared_ptr<PipelineEvent> event);
    void Dump(int fd, const std::vector<std::string>& cmds) override;
    void OnConfigUpdate(const std::string& localCfgPath, const std::string& cloudCfgPath) override;
    void UpdateTestType(const std::string& testType);

private:
    void InitController();
    bool IsValidSysEvent(const std::shared_ptr<SysEvent> event);
    std::shared_ptr<SysEvent> Convert2SysEvent(std::shared_ptr<Event>& event);
    void DecorateSysEvent(const std::shared_ptr<SysEvent> event, const BaseInfo& info, uint64_t id);
    bool IsDuplicateEvent(const uint64_t eventId);
    void ParseEventDefineFile();

private:
    EventServer eventServer_;
    PlatformMonitor platformMonitor_;
    std::unique_ptr<SysEventStat> sysEventStat_ = nullptr;
    std::shared_ptr<EventJsonParser> sysEventParser_ = nullptr;
    std::shared_ptr<IController> controller_;
    std::atomic<bool> isConfigUpdated_ { false };
    std::string testType_;
    std::list<uint64_t> eventIdList_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // SYS_EVENT_SOURCE_H