/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "bbox_detector_plugin.h"

#include <fstream>
#include <securec.h>

#include "event.h"
#include "event_loop.h"
#include "export_bbox_detectors_interface.h"
#include "hiview_logger.h"
#include "plugin_factory.h"

namespace OHOS {
namespace HiviewDFX {
using namespace std;
REGISTER(BBoxDetectorPlugin);
DEFINE_LOG_LABEL(0xD002D11, "BBoxDetectorPlugin");

class BBoxDetectorPlugin::BBoxListener : public EventListener {
public:
    explicit BBoxListener(BBoxDetectorPlugin& bBoxDetector);
    ~BBoxListener() {}
    void OnUnorderedEvent(const Event &msg) override;
    std::string GetListenerName() override;

private:
    BBoxDetectorPlugin& bBoxDetector_;
};

void BBoxDetectorPlugin::OnLoad()
{
    SetName("BBoxDetectorPlugin");
    SetVersion("BBoxDetector1.0");
    auto context = GetHiviewContext();
    if (context == nullptr) {
        HIVIEW_LOGE("GetHiviewContext failed.");
        return;
    }
    InitPanicReporter();

    eventListener_ = std::make_shared<BBoxListener>(*this);
    context->RegisterUnorderedEventListener(eventListener_);
    eventRecorder_ = std::make_shared<BboxEventRecorder>();
}

void BBoxDetectorPlugin::OnUnload()
{
    HIVIEW_LOGI("BBoxDetectorPlugin OnUnload");
    RemoveDetectBootCompletedTask();
}

bool BBoxDetectorPlugin::OnEvent(std::shared_ptr<Event> &event)
{
    if (event == nullptr || event->domain_ != "KERNEL_VENDOR") {
        return false;
    }
    auto sysEvent = Event::DownCastTo<SysEvent>(event);
    auto instance = GetBBoxDetectorsInterface(BBOX_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        return false;
    }
    instance->HandleBBoxEvent(sysEvent, eventRecorder_, isLastStartUpShort_);
    return true;
}

void BBoxDetectorPlugin::AddDetectBootCompletedTask()
{
    std::lock_guard<std::mutex> lock(lock_);
    if (workLoop_ && !timeEventAdded_) {
        timeEventId_ = workLoop_->AddTimerEvent(nullptr, nullptr, [this] {
            auto instance = GetBBoxDetectorsInterface();
            if (instance == nullptr) {
                return;
            }
            if (instance->IsBootCompleted()) {
                NotifyBootCompleted();
            }
        }, 1, true);
        timeEventAdded_ = true;
    }
}

void BBoxDetectorPlugin::RemoveDetectBootCompletedTask()
{
    std::lock_guard<std::mutex> lock(lock_);
    if (workLoop_ && timeEventAdded_) {
        workLoop_->RemoveEvent(timeEventId_);
        timeEventId_ = 0;
        timeEventAdded_ = false;
    }
}

void BBoxDetectorPlugin::NotifyBootStable()
{
    auto instance = GetBBoxDetectorsInterface(BBOX_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        return;
    }
    if (instance->TryToReportRecoveryPanicEvent()) {
        constexpr int timeout = 10; // 10s
        workLoop_->AddTimerEvent(nullptr, nullptr, [] {
            auto instance = GetBBoxDetectorsInterface();
            if (instance == nullptr) {
                return;
            }
            instance->ConfirmReportResult();
        }, timeout, false);
    }
}

void BBoxDetectorPlugin::NotifyBootCompleted()
{
    HIVIEW_LOGI("System boot completed, remove the task");
    RemoveDetectBootCompletedTask();
    constexpr int timeout = 60 * 10; // 10min
    workLoop_->AddTimerEvent(nullptr, nullptr, [this] {
        NotifyBootStable();
    }, timeout, false);
}

void BBoxDetectorPlugin::InitPanicReporter()
{
    auto instance = GetBBoxDetectorsInterface(BBOX_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        return;
    }
    if (!instance->InitPanicReport(isLastStartUpShort_)) {
        return;
    }
    AddDetectBootCompletedTask();
}

void BBoxDetectorPlugin::AddBootScanEvent()
{
    if (workLoop_ == nullptr) {
        HIVIEW_LOGE("workLoop_ is nullptr.");
        return;
    }

    auto task = [this]() {
        auto instance = GetBBoxDetectorsInterface(BBOX_LIB_DELAY_RELEASE_TIME);
        if (instance == nullptr) {
            return;
        }
        instance->StartBootScan(this->eventRecorder_);
    };
    workLoop_->AddTimerEvent(nullptr, nullptr, task, SECONDS, false); // delay 60s
}

BBoxDetectorPlugin::BBoxListener::BBoxListener(BBoxDetectorPlugin& bBoxDetector) : bBoxDetector_(bBoxDetector)
{
    AddListenerInfo(Event::MessageType::PLUGIN_MAINTENANCE);
}

void BBoxDetectorPlugin::BBoxListener::OnUnorderedEvent(const Event &msg)
{
    if (msg.messageType_ != Event::MessageType::PLUGIN_MAINTENANCE ||
        msg.eventId_ != Event::EventId::PLUGIN_LOADED) {
        HIVIEW_LOGE("messageType_(%{public}u), eventId_(%{public}u).", msg.messageType_, msg.eventId_);
        return;
    }
    bBoxDetector_.AddBootScanEvent();
}

std::string BBoxDetectorPlugin::BBoxListener::GetListenerName()
{
    return "BBoxListener";
}
}
}
