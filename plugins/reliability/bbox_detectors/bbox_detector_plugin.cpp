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

#include "common_defines.h"
#include "event.h"
#include "event_loop.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "panic_report_recovery.h"
#include "panic_error_info_handle.h"
#include "plugin_factory.h"
#include "hisysevent_util.h"
#include "smart_parser.h"
#include "string_util.h"
#include "tbox.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
using namespace std;
REGISTER(BBoxDetectorPlugin);
DEFINE_LOG_LABEL(0xD002D11, "BBoxDetectorPlugin");

namespace {
    const std::string HISIPATH = "/data/hisi_logs/";
    const std::string BBOXPATH = "/data/log/bbox/";
    const std::string LOGPARSECONFIG = "/system/etc/hiview";
    const std::vector<std::string> HISTORYLOGLIST = {
        "/data/hisi_logs/history.log",
        "/data/log/bbox/history.log"
    };
}

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
    HandleBBoxEvent(sysEvent);
    return true;
}

void BBoxDetectorPlugin::WaitForLogs(const std::string& logDir)
{
    std::string doneFile = logDir + "/DONE";
    if (!Tbox::WaitForDoneFile(doneFile, 60)) { // 60s
        HIVIEW_LOGE("can not find file: %{public}s", doneFile.c_str());
    }
}

void BBoxDetectorPlugin::HandleBBoxEvent(std::shared_ptr<SysEvent> &sysEvent)
{
    if (PanicReport::IsRecoveryPanicEvent(sysEvent)) {
        return;
    }
    string eventName = sysEvent->GetEventName();
    if (eventName == "CUSTOM") {
        std::string bboxTime = sysEvent->GetEventValue("BBOX_TIME");
        std::string bboxSysreset = sysEvent->GetEventValue("BBOX_SYSRESET");
        PanicErrorInfoHandle::RKTransData(bboxTime, bboxSysreset);
    }
    std::string event = sysEvent->GetEventValue("REASON");
    std::string module = sysEvent->GetEventValue("MODULE");
    std::string timeStr = sysEvent->GetEventValue("SUB_LOG_PATH");
    std::string logPath = sysEvent->GetEventValue("LOG_PATH");
    std::string name = sysEvent->GetEventValue("name_");

    std::string dynamicPaths = ((!logPath.empty() && logPath[logPath.size() - 1] == '/') ?
                                logPath : logPath + '/') + timeStr;
    if (HisysEventUtil::IsEventProcessed(name, "LOG_PATH", dynamicPaths)) {
        HIVIEW_LOGE("HandleBBoxEvent is processed event path is %{public}s", dynamicPaths.c_str());
        return;
    }
    if (name == "PANIC" && PanicReport::IsLastShortStartUp()) {
        PanicReport::CompressAndCopyLogFiles(dynamicPaths, timeStr);
    }
    if ((module == "AP") && (event == "BFM_S_NATIVE_DATA_FAIL")) {
        sysEvent->OnFinish();
    }
    auto happenTime = static_cast<uint64_t>(Tbox::GetHappenTime(StringUtil::GetRleftSubstr(timeStr, "-"),
        "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})"));
    sysEvent->SetEventValue("HAPPEN_TIME", happenTime);

    WaitForLogs(dynamicPaths);
    auto eventInfos = SmartParser::Analysis(dynamicPaths, LOGPARSECONFIG, name);
    Tbox::FilterTrace(eventInfos);

    sysEvent->SetEventValue("FIRST_FRAME", eventInfos["FIRST_FRAME"].empty() ? "/" :
                                StringUtil::EscapeJsonStringValue(eventInfos["FIRST_FRAME"]));
    sysEvent->SetEventValue("SECOND_FRAME", eventInfos["SECOND_FRAME"].empty() ? "/" :
                                StringUtil::EscapeJsonStringValue(eventInfos["SECOND_FRAME"]));
    sysEvent->SetEventValue("LAST_FRAME", eventInfos["LAST_FRAME"].empty() ? "/ " :
                                StringUtil::EscapeJsonStringValue(eventInfos["LAST_FRAME"]));
    sysEvent->SetEventValue("FINGERPRINT", Tbox::CalcFingerPrint(event + module + eventInfos["FIRST_FRAME"] +
        eventInfos["SECOND_FRAME"] + eventInfos["LAST_FRAME"], 0, FP_BUFFER));
    sysEvent->SetEventValue("LOG_PATH", dynamicPaths);
    if (eventRecorder_ != nullptr) {
        eventRecorder_->AddEventToMaps(name, happenTime, dynamicPaths);
    }
    HIVIEW_LOGI("HandleBBoxEvent event: %{public}s is success. happenTime %{public}" PRIu64, name.c_str(), happenTime);
}

void BBoxDetectorPlugin::StartBootScan()
{
    for (auto historyLog : HISTORYLOGLIST) {
        int num = READ_LINE_NUM;
        string line;

        if (FileUtil::FileExists(historyLog) && historyLog.find(HISIPATH) != std::string::npos) {
            hisiHistoryPath_ = true;
        }

        ifstream fin(historyLog, ios::ate);
        while (FileUtil::GetLastLine(fin, line) && num > 0) {
            num--;
            std::map<std::string, std::string> historyMap = GetValueFromHistory(line);
            string name = historyMap["category"];
            if (name.empty() || name == "NORMALBOOT") {
                continue;
            }
            if (name.find(":") != std::string::npos) {
                name = StringUtil::GetRleftSubstr(name, ":");
            }
            auto timeNow = TimeUtil::GetSeconds();
            auto happenTime = GetHappenTime(line);
            if (abs(timeNow - static_cast<int64_t>(happenTime)) > ONE_DAY  ||
                HisysEventUtil::IsEventProcessed(name, "LOG_PATH", historyMap["dynamicPaths"]) ||
                (eventRecorder_ != nullptr &&
                 eventRecorder_->IsExistEvent(name, happenTime, historyMap["dynamicPaths"]))) {
                HIVIEW_LOGI("Skip (%{public}s:%{public}" PRIu64 ")", name.c_str(), happenTime);
                continue;
            }
            int res = CheckAndHiSysEventWrite(name, historyMap, happenTime);
            HIVIEW_LOGI("BBox write history line is %{public}s write result =  %{public}d", line.c_str(), res);
        }
    }
    eventRecorder_.reset();
}

uint64_t BBoxDetectorPlugin::GetHappenTime(std::string& line)
{
    auto happenTime = hisiHistoryPath_ ?
        static_cast<uint64_t>(Tbox::GetHappenTime(StringUtil::GetMidSubstr(line, "time [", "-"),
            "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})")) :
        static_cast<uint64_t>(Tbox::GetHappenTime(StringUtil::GetMidSubstr(line, "time[", "-"),
            "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})"));

    return happenTime;
}

int BBoxDetectorPlugin::CheckAndHiSysEventWrite(std::string& name, std::map<std::string, std::string>& historyMap,
    uint64_t& happenTime)
{
    int res = HiSysEventWrite(HisysEventUtil::KERNEL_VENDOR, name, HiSysEvent::EventType::FAULT,
        "MODULE", historyMap["module"],
        "REASON", historyMap["reason"],
        "LOG_PATH", historyMap["logPath"],
        "SUB_LOG_PATH", historyMap["subLogPath"],
        "HAPPEN_TIME", happenTime,
        "SUMMARY", "bootup_keypoint:" + historyMap["bootup_keypoint"]);
    if (res == 0 || (res < 0 && name.find("UNKNOWNS") != std::string::npos)) {
        return res;
    } else if (res < 0) {
        name = "UNKNOWNS";
        CheckAndHiSysEventWrite(name, historyMap, happenTime);
    }

    return res;
}

std::map<std::string, std::string> BBoxDetectorPlugin::GetValueFromHistory(std::string& line)
{
    std::map<std::string, std::string> historyMap = {
        {"category", hisiHistoryPath_ ? StringUtil::GetMidSubstr(line, "category [", "]") :
            StringUtil::GetMidSubstr(line, "category[", "]")},
        {"module", hisiHistoryPath_ ? StringUtil::GetMidSubstr(line, "core [", "]") :
            StringUtil::GetMidSubstr(line, "module[", "]")},
        {"reason", hisiHistoryPath_ ? StringUtil::GetMidSubstr(line, "reason [", "]") :
            StringUtil::GetMidSubstr(line, "event[", "]")},
        {"bootup_keypoint", hisiHistoryPath_ ? StringUtil::GetMidSubstr(line, "bootup_keypoint [", "]") :
            StringUtil::GetMidSubstr(line, "errdesc[", "]")},
        {"dynamicPaths", hisiHistoryPath_ ? HISIPATH + StringUtil::GetMidSubstr(line, "time [", "]") :
            BBOXPATH + StringUtil::GetMidSubstr(line, "time[", "]")},
        {"logPath", hisiHistoryPath_ ? HISIPATH : BBOXPATH},
        {"subLogPath", hisiHistoryPath_ ? StringUtil::GetMidSubstr(line, "time [", "]") :
            StringUtil::GetMidSubstr(line, "time[", "]")}
    };

    return historyMap;
}

void BBoxDetectorPlugin::AddDetectBootCompletedTask()
{
    std::lock_guard<std::mutex> lock(lock_);
    if (workLoop_ && !timeEventAdded_) {
        timeEventId_ = workLoop_->AddTimerEvent(nullptr, nullptr, [this] {
            if (PanicReport::IsBootCompleted()) {
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
    if (PanicReport::TryToReportRecoveryPanicEvent()) {
        constexpr int timeout = 10; // 10s
        workLoop_->AddTimerEvent(nullptr, nullptr, [] {
            PanicReport::ConfirmReportResult();
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
    if (!PanicReport::InitPanicReport()) {
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
        StartBootScan();
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
