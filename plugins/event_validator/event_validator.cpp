/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "event_validator.h"

#include "daily_controller.h"
#include "event_json_parser.h"
#include "hiview_config_util.h"
#include "hiview_logger.h"
#include "plugin_factory.h"

namespace OHOS {
namespace HiviewDFX {
REGISTER(EventValidator);
DEFINE_LOG_TAG("EventValidator");
namespace {
constexpr char THRESHOLD_CONFIG_FILE_NAME[] = "event_threshold.json";
}

void EventValidator::OnLoad()
{
    HIVIEW_LOGI("start to load EventValidator");
    Init();
}

void EventValidator::Init()
{
    auto context = GetHiviewContext();
    if (context == nullptr) {
        HIVIEW_LOGW("context is null");
        return;
    }

    InitWorkLoop(context);
    InitEventVerifyUtil(context);
    InitController(context);
}

void EventValidator::InitWorkLoop(HiviewContext* context)
{
    workLoop_ = context->GetMainWorkLoop();
}

void EventValidator::InitEventVerifyUtil(HiviewContext* context)
{
    eventVerifyUtil_.Init(context);
}

void EventValidator::InitController(HiviewContext* context)
{
    std::string workPath = context->GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY);
    controller_ = std::make_unique<DailyController>(
        workPath, HiViewConfigUtil::GetConfigFilePath(THRESHOLD_CONFIG_FILE_NAME));
}

void EventValidator::OnUnload()
{
    HIVIEW_LOGI("start to unload EventValidator");
}

void EventValidator::OnConfigUpdate(const std::string& localCfgPath, const std::string& cloudCfgPath)
{
    isConfigUpdated_.store(true);
}

bool EventValidator::OnEvent(std::shared_ptr<Event> &event)
{
    if (event == nullptr) {
        HIVIEW_LOGI("event is null");
        return false;
    }
    if (event->messageType_ != Event::MessageType::SYS_EVENT) {
        HIVIEW_LOGI("event is not sys event");
        return false;
    }
    if (isConfigUpdated_) {
        UpdateConfig();
    }
    if (!CheckEvent(event)) {
        event->OnFinish();
        return false;
    }
    return true;
}

void EventValidator::UpdateConfig()
{
    // update controller
    if (controller_ != nullptr) {
        controller_->OnConfigUpdate(HiViewConfigUtil::GetConfigFilePath(THRESHOLD_CONFIG_FILE_NAME));
    }

    // update parser
    EventJsonParser::GetInstance()->OnConfigUpdate();

    // update flag
    isConfigUpdated_.store(false);
}

bool EventValidator::CheckEvent(std::shared_ptr<Event> event)
{
    std::shared_ptr<SysEvent> sysEvent = Event::DownCastTo<SysEvent>(event);
    if (sysEvent == nullptr) {
        HIVIEW_LOGE("sysevent is null");
        return false;
    }
    eventDelayedUtil_.CheckIfEventDelayed(sysEvent);

    if (controller_ != nullptr && !controller_->CheckThreshold(sysEvent)) {
        HIVIEW_LOGD("event[%{public}s|%{public}s}] exceeds the threshold",
            sysEvent->domain_.c_str(), sysEvent->eventName_.c_str());
        return false;
    }

    return eventVerifyUtil_.IsValidEvent(sysEvent);
}
} // namespace HiviewDFX
} // namespace OHOS
