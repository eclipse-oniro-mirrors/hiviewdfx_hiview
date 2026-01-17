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

#include "bbox_detectors_base.h"

#include "hisysevent.h"
#include "hisysevent_util.h"
#include "hiview_logger.h"
#include "panic_error_info_handle.h"
#include "panic_report_recovery.h"
#include "smart_parser.h"
#include "string_util.h"
#include "tbox.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger_BBOX_Base");

extern "C" {
BBoxDetectorsInterface* NewBBoxDetectorsInterface(void)
{
    static BBoxDetectorsBase bboxDetectorsBase;
    return &bboxDetectorsBase;
}
} // extern "C"

namespace {
    constexpr const char* const HISIPATH = "/data/hisi_logs/";
    constexpr const char* const BBOXPATH = "/data/log/bbox/";
    constexpr const char* const LOGPARSECONFIG = "/system/etc/hiview";
    constexpr const char* const HISTORYLOGLIST[] = {
        "/data/hisi_logs/history.log",
        "/data/log/bbox/history.log"
    };
}

BBoxDetectorsBase::BBoxDetectorsBase()
{
    HIVIEW_LOGE("constructor BBoxDetectorsBase");
}

BBoxDetectorsBase::~BBoxDetectorsBase()
{
    HIVIEW_LOGE("destructor BBoxDetectorsBase");
}

void BBoxDetectorsBase::HandleBBoxEvent(std::shared_ptr<SysEvent> &sysEvent,
                                        std::shared_ptr<BboxEventRecorder> eventRecorder,
                                        bool isLastStartUpShort)
{
    if (PanicReport::IsRecoveryPanicEvent(sysEvent)) {
        return;
    }
    std::string eventName = sysEvent->GetEventName();
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
    auto happenTime = static_cast<uint64_t>(Tbox::GetHappenTime(StringUtil::GetRleftSubstr(timeStr, "-"),
        "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})"));
    if (HisysEventUtil::IsEventProcessed(name, "LOG_PATH", dynamicPaths) ||
        (eventRecorder != nullptr && eventRecorder->IsExistEvent(name, happenTime, dynamicPaths))) {
        HIVIEW_LOGE("HandleBBoxEvent is processed event path is %{public}s", dynamicPaths.c_str());
        return;
    }
    if (name == "PANIC" && PanicReport::IsLastShortStartUp(isLastStartUpShort)) {
        PanicReport::CompressAndCopyLogFiles(dynamicPaths, timeStr);
    }
    if ((module == "AP") && (event == "BFM_S_NATIVE_DATA_FAIL")) {
        sysEvent->OnFinish();
    }
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
    if (eventRecorder != nullptr) {
        eventRecorder->AddEventToMaps(name, happenTime, dynamicPaths);
    }
    HIVIEW_LOGI("HandleBBoxEvent event: %{public}s is success. happenTime %{public}" PRIu64, name.c_str(), happenTime);
}

void BBoxDetectorsBase::StartBootScan(std::shared_ptr<BboxEventRecorder> eventRecorder)
{
    constexpr int oneDaySecond = 60 * 60 * 24;
    constexpr int readLineNum = 5;
    bool hisiHistory = false;
    for (std::string historyLog : HISTORYLOGLIST) {
        int num = readLineNum;
        std::string line;
        if (FileUtil::FileExists(historyLog) && historyLog.find(HISIPATH) != std::string::npos) {
            hisiHistory = true;
        }

        std::ifstream fin(historyLog, std::ios::ate);
        while (FileUtil::GetLastLine(fin, line) && num > 0) {
            num--;
            std::map<std::string, std::string> historyMap = GetValueFromHistory(line, hisiHistory);
            std::string name = historyMap["category"];
            if (name.empty() || name == "NORMALBOOT") {
                continue;
            }
            if (name.find(":") != std::string::npos) {
                name = StringUtil::GetRleftSubstr(name, ":");
            }
            auto timeNow = TimeUtil::GetSeconds();
            auto happenTime = GetHappenTime(line, hisiHistory);
            if (abs(timeNow - static_cast<int64_t>(happenTime)) > oneDaySecond ||
                HisysEventUtil::IsEventProcessed(name, "LOG_PATH", historyMap["dynamicPaths"]) ||
                (eventRecorder != nullptr &&
                 eventRecorder->IsExistEvent(name, happenTime, historyMap["dynamicPaths"]))) {
                HIVIEW_LOGI("Skip (%{public}s:%{public}" PRIu64 ")", name.c_str(), happenTime);
                continue;
            }
            int res = CheckAndHiSysEventWrite(name, historyMap, happenTime);
            HIVIEW_LOGI("BBox write history line is %{public}s write result =  %{public}d", line.c_str(), res);
            if (eventRecorder != nullptr) {
                eventRecorder->AddEventToMaps(name, happenTime, historyMap["dynamicPaths"]);
            }
        }
    }
    eventRecorder.reset();
}

std::map<std::string, std::string> BBoxDetectorsBase::GetValueFromHistory(std::string& line, bool isHisiHistory)
{
    std::map<std::string, std::string> historyMap = {
        {"category", isHisiHistory ? StringUtil::GetMidSubstr(line, "category [", "]") :
            StringUtil::GetMidSubstr(line, "category[", "]")},
        {"module", isHisiHistory ? StringUtil::GetMidSubstr(line, "core [", "]") :
            StringUtil::GetMidSubstr(line, "module[", "]")},
        {"reason", isHisiHistory ? StringUtil::GetMidSubstr(line, "reason [", "]") :
            StringUtil::GetMidSubstr(line, "event[", "]")},
        {"bootup_keypoint", isHisiHistory ? StringUtil::GetMidSubstr(line, "bootup_keypoint [", "]") :
            StringUtil::GetMidSubstr(line, "errdesc[", "]")},
        {"dynamicPaths", isHisiHistory ? HISIPATH + StringUtil::GetMidSubstr(line, "time [", "]") :
            BBOXPATH + StringUtil::GetMidSubstr(line, "time[", "]")},
        {"logPath", isHisiHistory ? HISIPATH : BBOXPATH},
        {"subLogPath", isHisiHistory ? StringUtil::GetMidSubstr(line, "time [", "]") :
            StringUtil::GetMidSubstr(line, "time[", "]")}
    };

    return historyMap;
}

uint64_t BBoxDetectorsBase::GetHappenTime(std::string& line, bool isHisiHistory)
{
    auto happenTime = isHisiHistory ?
        static_cast<uint64_t>(Tbox::GetHappenTime(StringUtil::GetMidSubstr(line, "time [", "-"),
            "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})")) :
        static_cast<uint64_t>(Tbox::GetHappenTime(StringUtil::GetMidSubstr(line, "time[", "-"),
            "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})"));

    return happenTime;
}

int BBoxDetectorsBase::CheckAndHiSysEventWrite(std::string& name, std::map<std::string, std::string>& historyMap,
    uint64_t& happenTime)
{
    auto summary = "bootup_keypoint:" + historyMap["bootup_keypoint"];
    HiSysEventParam params[] = {
        {.name = "MODULE", .t = HISYSEVENT_STRING, .v = {.s = historyMap["module"].data()}, .arraySize = 0},
        {.name = "REASON", .t = HISYSEVENT_STRING, .v = {.s = historyMap["reason"].data()}, .arraySize = 0},
        {.name = "LOG_PATH", .t = HISYSEVENT_STRING, .v = {.s = historyMap["logPath"].data()}, .arraySize = 0},
        {.name = "SUB_LOG_PATH", .t = HISYSEVENT_STRING, .v = {.s = historyMap["subLogPath"].data()}, .arraySize = 0},
        {.name = "HAPPEN_TIME", .t = HISYSEVENT_INT64, .v = {.i64 = happenTime}, .arraySize = 0},
        {.name = "SUMMARY", .t = HISYSEVENT_STRING, .v = {.s = summary.data()}, .arraySize = 0},
    };
    int res = OH_HiSysEvent_Write(HisysEventUtil::KERNEL_VENDOR, name.c_str(), HISYSEVENT_FAULT,
        params, sizeof(params) / sizeof(HiSysEventParam));
    if (res == 0 || (res < 0 && name.find("UNKNOWNS") != std::string::npos)) {
        return res;
    } else if (res < 0) {
        name = "UNKNOWNS";
        CheckAndHiSysEventWrite(name, historyMap, happenTime);
    }

    return res;
}

void BBoxDetectorsBase::WaitForLogs(const std::string& logDir)
{
    std::string doneFile = logDir + "/DONE";
    if (!Tbox::WaitForDoneFile(doneFile, 60)) { // 60s
        HIVIEW_LOGE("can not find file: %{public}s", doneFile.c_str());
    }
}

bool BBoxDetectorsBase::InitPanicReport(bool& isLastStartUpShort)
{
    return PanicReport::InitPanicReport(isLastStartUpShort);
}

bool BBoxDetectorsBase::IsBootCompleted()
{
    return PanicReport::IsBootCompleted();
}

bool BBoxDetectorsBase::TryToReportRecoveryPanicEvent()
{
    return PanicReport::TryToReportRecoveryPanicEvent();
}

void BBoxDetectorsBase::ConfirmReportResult()
{
    PanicReport::ConfirmReportResult();
}
} // namespace HiviewDFX
} // namespace OHOS
