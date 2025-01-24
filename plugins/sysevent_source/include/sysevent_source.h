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

#ifndef SYS_EVENT_SOURCE_H
#define SYS_EVENT_SOURCE_H

#include <atomic>
#include <fstream>
#include <list>
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
#include "period_file_operator.h"
#include "sys_event_service_adapter.h"
#include "sys_event_stat.h"

namespace OHOS {
namespace HiviewDFX {
class SysEventSource;
constexpr uint64_t DEFAULT_PERIOD_SEQ = 1; // period seq begins with 1
class SysEventReceiver : public EventReceiver {
public:
    explicit SysEventReceiver(SysEventSource& source): eventSource(source) {};
    ~SysEventReceiver() override {};
    void HandlerEvent(std::shared_ptr<EventRaw::RawData> rawData) override;
private:
    SysEventSource& eventSource;
};

struct SourcePeriodInfo {
    // format: YYYYMMDDHH
    std::string timeStamp;

    // count of event which will be preserve into db file in 1 hour
    uint64_t preserveCnt = 0;

    // count of event which will be exported in 1 hour
    uint64_t exportCnt = 0;

    SourcePeriodInfo(const std::string& timeStamp, uint64_t preserveCnt, uint64_t exportCnt)
        : timeStamp(timeStamp), preserveCnt(preserveCnt), exportCnt(exportCnt) {}
};

class SysEventSource : public EventSource, public SysEventServiceBase {
public:
    SysEventSource();
    ~SysEventSource() {}

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
    std::string GetEventExportConfigFilePath();
    void StatisticSourcePeriodInfo(const std::shared_ptr<SysEvent> event);
    void RecordSourcePeriodInfo();

private:
    EventServer eventServer_;
    PlatformMonitor platformMonitor_;
    std::unique_ptr<SysEventStat> sysEventStat_ = nullptr;
    std::shared_ptr<EventJsonParser> sysEventParser_ = nullptr;
    std::shared_ptr<IController> controller_;
    std::atomic<bool> isConfigUpdated_ { false };
    std::string testType_;
    std::list<uint64_t> eventIdList_;
    std::list<std::shared_ptr<SourcePeriodInfo>> periodInfoList_;
    uint64_t periodSeq_ = DEFAULT_PERIOD_SEQ;
    bool isLastEventDelayed_ = false;
    std::unique_ptr<PeriodInfoFileOperator> periodFileOpt_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // SYS_EVENT_SOURCE_H