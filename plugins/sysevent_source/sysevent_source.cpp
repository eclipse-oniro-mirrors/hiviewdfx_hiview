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

#include "sysevent_source.h"

#include "event_json_parser.h"
#include "hiview_logger.h"
#include "hiview_platform.h"
#include "plugin_factory.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
REGISTER(SysEventSource);
DEFINE_LOG_TAG("SysEventSource");

void SysEventReceiver::HandlerEvent(std::shared_ptr<EventRaw::RawData> rawData)
{
    if (rawData == nullptr || rawData->GetData() == nullptr) {
        HIVIEW_LOGW("raw data of sys event is null");
        return;
    }
    std::shared_ptr<PipelineEvent> event = std::make_shared<SysEvent>("SysEventSource",
        static_cast<PipelineEventProducer*>(&eventSource), rawData);
    eventSource.PublishPipelineEvent(event);
}

void SysEventSource::OnLoad()
{
    HIVIEW_LOGI("SysEventSource load");
    std::shared_ptr<EventLoop> looper = GetHiviewContext()->GetSharedWorkLoop();
    platformMonitor_.StartMonitor(looper);

    // start sys event service
    SysEventServiceAdapter::StartService(this);
    SysEventServiceAdapter::SetWorkLoop(looper);
}

void SysEventSource::OnUnload()
{
    HIVIEW_LOGI("SysEventSource unload");
    eventServer_.Stop();
}

void SysEventSource::StartEventSource()
{
    HIVIEW_LOGI("SysEventSource start");
    EventJsonParser::GetInstance()->ReadDefFile();
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
        auto dispatchRule = it->second;
        if (dispatchRule->FindEvent(event->domain_, event->eventName_)) {
            pipelineMap.at(pipelineName)->ProcessEvent(event);
            return true;
        }
    }

    constexpr char defaultPipeline[] = "SysEventPipeline";
    if (pipelineMap.find(defaultPipeline) == pipelineMap.end()) {
        HIVIEW_LOGW("default %{public}s does not exist", defaultPipeline);
        return false;
    }
    pipelineMap.at(defaultPipeline)->ProcessEvent(event);
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
