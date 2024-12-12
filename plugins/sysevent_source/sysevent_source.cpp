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

#include "sysevent_source.h"

#include <functional>
#include <memory>

#include "daily_controller.h"
#include "decoded/decoded_event.h"
#include "defines.h"
#include "raw_data_base_def.h"
#include "file_util.h"
#include "focused_event_util.h"
#include "hiview_config_util.h"
#include "hiview_logger.h"
#include "plugin_factory.h"
#include "time_util.h"
#include "sys_event.h"
#include "hiview_platform.h"
#include "param_const_common.h"
#include "parameter.h"
#include "sys_event_dao.h"
#include "sys_event_service_adapter.h"

namespace OHOS {
namespace HiviewDFX {
REGISTER(SysEventSource);
namespace {
DEFINE_LOG_TAG("HiView-SysEventSource");
constexpr char DEF_FILE_NAME[] = "hisysevent.def";
constexpr char DEF_ZIP_NAME[] = "hisysevent.zip";
constexpr char DEF_CFG_DIR[] = "sys_event_def";
constexpr char TEST_TYPE_PARAM_KEY[] = "hiviewdfx.hiview.testtype";
constexpr char TEST_TYPE_KEY[] = "test_type_";

uint64_t GenerateHash(std::shared_ptr<SysEvent> event)
{
    if (event == nullptr) {
        return 0;
    }
    constexpr size_t infoLenLimit = 256;
    size_t infoLen = event->rawData_->GetDataLength();
    size_t hashLen = (infoLen < infoLenLimit) ? infoLen : infoLenLimit;
    const uint8_t* p = event->rawData_->GetData();
    uint64_t ret { 0xCBF29CE484222325ULL }; // basis value
    size_t i = 0;
    while (i < hashLen) {
        ret ^= *(p + i);
        ret *= 0x100000001B3ULL; // prime value
        i++;
    }
    return ret;
}

void ParameterWatchCallback(const char* key, const char* value, void* context)
{
    if (context == nullptr) {
        HIVIEW_LOGE("context is null");
        return;
    }
    auto eventSourcePlugin = reinterpret_cast<SysEventSource*>(context);
    if (eventSourcePlugin == nullptr) {
        HIVIEW_LOGE("eventsource plugin is null");
        return;
    }
    size_t testTypeStrMaxLen = 256;
    std::string testTypeStr(value);
    if (testTypeStr.size() > testTypeStrMaxLen) {
        HIVIEW_LOGE("length of the test type string set exceeds the limit");
        return;
    }
    HIVIEW_LOGI("test_type is set to be \"%{public}s\"", testTypeStr.c_str());
    eventSourcePlugin->UpdateTestType(testTypeStr);
}
}

void SysEventReceiver::HandlerEvent(std::shared_ptr<EventRaw::RawData> rawData)
{
    if (rawData == nullptr || rawData->GetData() == nullptr) {
        HIVIEW_LOGW("raw data of sys event is null");
        return;
    }
    std::shared_ptr<PipelineEvent> event = std::make_shared<SysEvent>("SysEventSource",
        static_cast<PipelineEventProducer*>(&eventSource), rawData);
    if (eventSource.CheckEvent(event)) {
        eventSource.PublishPipelineEvent(event);
    }
}

void SysEventSource::OnLoad()
{
    HIVIEW_LOGI("SysEventSource load ");
    std::shared_ptr<EventLoop> looper = GetHiviewContext()->GetSharedWorkLoop();
    platformMonitor_.StartMonitor(looper);

    sysEventStat_ = std::make_unique<SysEventStat>();
    InitController();

    // start sys event service
    auto notifyFunc = [&] (std::shared_ptr<Event> event) -> void {
        this->GetHiviewContext()->PostUnorderedEvent(shared_from_this(), event);
    };
    SysEventServiceAdapter::StartService(this, notifyFunc);
    SysEventServiceAdapter::SetWorkLoop(looper);

    auto defFilePath = HiViewConfigUtil::GetConfigFilePath(DEF_ZIP_NAME, DEF_CFG_DIR, DEF_FILE_NAME);
    HIVIEW_LOGI("init json parser with %{public}s", defFilePath.c_str());
    sysEventParser_ = std::make_shared<EventJsonParser>(defFilePath);

    SysEventServiceAdapter::BindGetTagFunc(
        [this] (const std::string& domain, const std::string& name) {
            return this->sysEventParser_->GetTagByDomainAndName(domain, name);
        });
    SysEventServiceAdapter::BindGetTypeFunc(
        [this] (const std::string& domain, const std::string& name) {
            return this->sysEventParser_->GetTypeByDomainAndName(domain, name);
        });

    // watch parameter
    if (WatchParameter(TEST_TYPE_PARAM_KEY, ParameterWatchCallback, this) != 0) {
        HIVIEW_LOGW("failed to watch the change of parameter %{public}s", TEST_TYPE_PARAM_KEY);
    }
}

void SysEventSource::InitController()
{
    auto context = GetHiviewContext();
    if (context == nullptr) {
        HIVIEW_LOGW("context is null");
        return;
    }

    std::string workPath = context->GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY);
    std::string configPath = context->GetHiViewDirectory(HiviewContext::DirectoryType::CONFIG_DIRECTORY);
    const std::string configFileName = "event_threshold.json";
    controller_ = std::make_unique<DailyController>(workPath, configPath.append(configFileName));
}

void SysEventSource::OnUnload()
{
    eventServer_.Stop();
    HIVIEW_LOGI("SysEventSource unload");
}

void SysEventSource::StartEventSource()
{
    HIVIEW_LOGI("SysEventSource start");
    std::shared_ptr<EventReceiver> sysEventReceiver = std::make_shared<SysEventReceiver>(*this);
    eventServer_.AddReceiver(sysEventReceiver);
    eventServer_.Start();
}

void SysEventSource::Recycle(PipelineEvent *event)
{
    platformMonitor_.CollectCostTime(event);
}

void SysEventSource::PauseDispatch(std::weak_ptr<Plugin> plugin)
{
    auto requester = plugin.lock();
    if (requester != nullptr) {
        HIVIEW_LOGI("process pause dispatch event from plugin:%s.\n", requester->GetName().c_str());
    }
}

bool SysEventSource::PublishPipelineEvent(std::shared_ptr<PipelineEvent> event)
{
    platformMonitor_.CollectEvent(event);
    platformMonitor_.Breaking();
    auto context = GetHiviewContext();
    HiviewPlatform* hiviewPlatform = static_cast<HiviewPlatform*>(context);
    if (hiviewPlatform == nullptr) {
        HIVIEW_LOGW("hiviewPlatform is null");
        return false;
    }
    auto const &pipelineRules = hiviewPlatform->GetPipelineConfigMap();
    auto const &pipelineMap = hiviewPlatform->GetPipelineMap();
    for (auto it = pipelineRules.begin(); it != pipelineRules.end(); it++) {
        std::string pipelineName = it->first;
        auto dispathRule = it->second;
        if (dispathRule->FindEvent(event->domain_, event->eventName_)) {
            pipelineMap.at(pipelineName)->ProcessEvent(event);
            return true;
        }
    }
    pipelineMap.at("SysEventPipeline")->ProcessEvent(event);
    return true;
}

bool SysEventSource::CheckEvent(std::shared_ptr<Event> event)
{
    if (isConfigUpdated_) {
        auto defFilePath = HiViewConfigUtil::GetConfigFilePath(DEF_ZIP_NAME, DEF_CFG_DIR, DEF_FILE_NAME);
        HIVIEW_LOGI("update json parser with %{public}s", defFilePath.c_str());
        sysEventParser_->ReadDefFile(defFilePath);
        isConfigUpdated_.store(false);
    }
    std::shared_ptr<SysEvent> sysEvent = Convert2SysEvent(event);
    if (sysEvent == nullptr) {
        HIVIEW_LOGE("event or event parser is null.");
        sysEventStat_->AccumulateEvent(false);
        return false;
    }
    if (controller_ != nullptr && !controller_->CheckThreshold(sysEvent)) {
        sysEventStat_->AccumulateEvent(false);
        return false;
    }
    if (!IsValidSysEvent(sysEvent)) {
        sysEventStat_->AccumulateEvent(sysEvent->domain_, sysEvent->eventName_, false);
        return false;
    }
    if (FocusedEventUtil::IsFocusedEvent(sysEvent->domain_, sysEvent->eventName_)) {
        HIVIEW_LOGI("event[%{public}s|%{public}s|%{public}" PRIu64 "] is valid.",
            sysEvent->domain_.c_str(), sysEvent->eventName_.c_str(), event->happenTime_);
    } else {
        HIVIEW_LOGD("event[%{public}s|%{public}s|%{public}" PRIu64 "] is valid.",
            sysEvent->domain_.c_str(), sysEvent->eventName_.c_str(), event->happenTime_);
    }
    sysEventStat_->AccumulateEvent();
    return true;
}

std::shared_ptr<SysEvent> SysEventSource::Convert2SysEvent(std::shared_ptr<Event>& event)
{
    if (event == nullptr) {
        HIVIEW_LOGE("event is null");
        return nullptr;
    }
    if (event->messageType_ != Event::MessageType::SYS_EVENT) {
        HIVIEW_LOGE("receive out of sys event type");
        return nullptr;
    }
    return Event::DownCastTo<SysEvent>(event);
}

static void ShowUsage(int fd, const std::vector<std::string>& cmds)
{
    dprintf(fd, "invalid cmd:");
    for (auto it = cmds.begin(); it != cmds.end(); it++) {
        dprintf(fd, "%s ", it->c_str());
    }
    dprintf(fd, "\n");
    dprintf(fd, "usage: SysEventService [sum|detail|invalid|clear]\n");
}

void SysEventSource::Dump(int fd, const std::vector<std::string>& cmds)
{
    if (cmds.size() >= 2) { // 2: args from the second item
        std::string arg1 = cmds[1];
        if (arg1 == "sum") {
            sysEventStat_->StatSummary(fd);
        } else if (arg1 == "detail") {
            sysEventStat_->StatDetail(fd);
        } else if (arg1 == "invalid") {
            sysEventStat_->StatInvalidDetail(fd);
        } else if (arg1 == "clear") {
            sysEventStat_->Clear(fd);
        } else {
            ShowUsage(fd, cmds);
        }
    } else {
        sysEventStat_->StatSummary(fd);
    }
}

void SysEventSource::OnConfigUpdate(const std::string& localCfgPath, const std::string& cloudCfgPath)
{
    this->isConfigUpdated_.store(true);
}

bool SysEventSource::IsValidSysEvent(const std::shared_ptr<SysEvent> event)
{
    if (event->domain_.empty() || event->eventName_.empty()) {
        HIVIEW_LOGW("domain=%{public}s or name=%{public}s is empty.",
            event->domain_.c_str(), event->eventName_.c_str());
        return false;
    }
    auto baseInfo = sysEventParser_->GetDefinedBaseInfoByDomainName(event->domain_, event->eventName_);
    if (baseInfo.type == INVALID_EVENT_TYPE) {
        HIVIEW_LOGW("type defined for event[%{public}s|%{public}s|%{public}" PRIu64 "] is invalid.",
            event->domain_.c_str(), event->eventName_.c_str(), event->happenTime_);
        return false;
    }
    if (event->GetEventType() != baseInfo.type) {
        HIVIEW_LOGW("type=%{public}d of event[%{public}s|%{public}s|%{public}" PRIu64 "] is invalid.",
            event->GetEventType(), event->domain_.c_str(), event->eventName_.c_str(), event->happenTime_);
        return false;
    }
    // append id
    auto eventId = GenerateHash(event);
    if (IsDuplicateEvent(eventId)) {
        HIVIEW_LOGW("ignore duplicate event[%{public}s|%{public}s|%{public}" PRIu64 "].",
            event->domain_.c_str(), event->eventName_.c_str(), eventId);
        return false;
    }
    DecorateSysEvent(event, baseInfo, eventId);
    return true;
}

void SysEventSource::UpdateTestType(const std::string& testType)
{
    testType_ = testType;
}

void SysEventSource::DecorateSysEvent(const std::shared_ptr<SysEvent> event, const BaseInfo& baseInfo, uint64_t id)
{
    if (!baseInfo.level.empty()) {
        event->SetLevel(baseInfo.level);
    }
    if (!baseInfo.tag.empty()) {
        event->SetTag(baseInfo.tag);
    }
    event->SetPrivacy(baseInfo.privacy);
    if (!testType_.empty()) {
        event->SetEventValue(TEST_TYPE_KEY, testType_);
    }
    event->preserve_ = baseInfo.preserve;
    // add hashcode id
    event->SetId(id);
}

bool SysEventSource::IsDuplicateEvent(const uint64_t eventId)
{
    for (auto iter = eventIdList_.begin(); iter != eventIdList_.end(); iter++) {
        if (*iter == eventId) {
            return true;
        }
    }
    std::list<std::string>::size_type maxSize { 5 }; // size of queue limit to 5
    if (eventIdList_.size() >= maxSize) {
        eventIdList_.pop_front();
    }
    eventIdList_.emplace_back(eventId);
    return false;
}
} // namespace HiviewDFX
} // namespace OHOS
