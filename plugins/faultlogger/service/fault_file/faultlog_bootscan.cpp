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
#include "faultlog_bootscan.h"

#include "constants.h"
#include "event_publish.h"
#include "faultlog_formatter.h"
#include "faultlog_util.h"
#include "faultlog_processor_factory.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "sanitizer_telemetry.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

namespace {
constexpr time_t FORTYEIGHT_HOURS = 48 * 60 * 60;
}
using namespace FaultLogger;

bool FaultLogBootScan::IsCrashType(const std::string& file)
{
    // if file type is not cppcrash, skip!
    if (file.find("cppcrash") == std::string::npos) {
        HIVIEW_LOGI("Skip this file(%{public}s) that the type is not cppcrash.", file.c_str());
        return false;
    }
    return true;
}

bool FaultLogBootScan::IsInValidTime(const std::string& file, const time_t& now)
{
    time_t lastAccessTime = GetFileLastAccessTimeStamp(file);
    if (now < lastAccessTime) {
        HIVIEW_LOGI("Skip this file(%{public}s) that current time may be incorrect.", file.c_str());
        return false;
    }
    if (now - lastAccessTime > FORTYEIGHT_HOURS) {
        HIVIEW_LOGI("Skip this file(%{public}s) that were created 48 hours ago.", file.c_str());
        return false;
    }
    return true;
}

bool FaultLogBootScan::IsCrashTempBigFile(const std::string& file)
{
    constexpr uint64_t tempMaxFileSize = 1024 * 1024 * 5;
    auto fileSize = FileUtil::GetFileSize(file);
    if (fileSize > tempMaxFileSize) {
        HIVIEW_LOGI("Skip this file(%{public}s) that file size(%{public}" PRIu64 ") exceeds limit.",
                    file.c_str(), fileSize);
        FileUtil::RemoveFile(file);
        return true;
    }
    return false;
}

bool FaultLogBootScan::IsEmptyStack(const std::string& file, const FaultLogInfo& info)
{
    if (info.summary.find("#00") == std::string::npos) {
        HIVIEW_LOGI("Skip this file(%{public}s) which stack is empty.", file.c_str());
        auto module = info.module;
        HiSysEventParam params[] = {
            {.name = "PID", .t = HISYSEVENT_INT32, .v = {.i32 = info.pid}, .arraySize = 0},
            {.name = "UID", .t = HISYSEVENT_INT32, .v = {.i32 = info.id}, .arraySize = 0},
            {.name = "PROCESS_NAME", .t = HISYSEVENT_STRING, .v = {.s = module.data()}, .arraySize = 0},
            {.name = "HAPPEN_TIME", .t = HISYSEVENT_INT64, .v = {.i64 = info.time}, .arraySize = 0},
        };
        OH_HiSysEvent_Write(HiSysEvent::Domain::RELIABILITY, "CPP_CRASH_NO_LOG", HISYSEVENT_FAULT,
            params, sizeof(params) / sizeof(HiSysEventParam));
        if (remove(file.c_str()) != 0) {
            HIVIEW_LOGE("Failed to remove file(%{public}s) which stack is empty", file.c_str());
        }
        return true;
    }
    return false;
}

bool FaultLogBootScan::IsReported(const FaultLogInfo& info)
{
    if (faultLogManager_->IsProcessedFault(info.pid, info.id, info.faultLogType)) {
        HIVIEW_LOGI("Skip processed fault.(%{public}d:%{public}d) ", info.pid, info.id);
        return true;
    }
    return false;
}

void FaultLogBootScan::StartBootScan()
{
    std::vector<std::string> files;
    time_t now = time(nullptr);
    FileUtil::GetDirFiles(FAULTLOG_TEMP_FOLDER, files);
    for (const auto& file : files) {
        if (!IsCrashType(file) || !IsInValidTime(file, now) || IsCrashTempBigFile(file)) {
            continue;
        }

        auto info = ParseCppCrashFromFile(file);
        if (IsEmptyStack(file, info) || IsReported(info)) {
            continue;
        }
        FaultLogProcessorFactory factory;
        auto processor = factory.CreateFaultLogProcessor(static_cast<FaultLogType>(info.faultLogType));
        if (processor) {
            info.sectionMap["START_BOOT_SCAN"] = "true";
            processor->AddFaultLog(info, workLoop_, faultLogManager_);
        }
    }
}

void FaultLogBootScan::AddBootScanEvent()
{
    if (workLoop_ == nullptr) {
        HIVIEW_LOGE("workLoop_ is nullptr.");
        return;
    }
    // some crash happened before hiview start, ensure every crash event is added into eventdb
    auto task = [this] {
        StartBootScan();
    };
    workLoop_->AddTimerEvent(nullptr, nullptr, task, 60, false); // delay 60 seconds
}

FaultLogBootScan::FaultLogBootScan(std::shared_ptr<EventLoop> workLoop,
    std::shared_ptr<FaultLogManager> faultLogManager) : workLoop_(workLoop),  faultLogManager_(faultLogManager)
{
    AddListenerInfo(Event::MessageType::PLUGIN_MAINTENANCE);
    AddListenerInfo(Event::TELEMETRY_EVENT);
}

void FaultLogBootScan::OnUnorderedEvent(const Event& msg)
{
    if (msg.messageType_ == Event::TELEMETRY_EVENT) {
        SanitizerTelemetry sanitizerTelemetry;
        sanitizerTelemetry.OnUnorderedEvent(msg);
    }
#ifndef UNITTEST
    if (msg.messageType_ != Event::MessageType::PLUGIN_MAINTENANCE ||
        msg.eventId_ != Event::EventId::PLUGIN_LOADED) {
        HIVIEW_LOGE("messageType_(%{public}u), eventId_(%{public}u).", msg.messageType_, msg.eventId_);
        return;
    }
    AddBootScanEvent();
#endif
}

std::string FaultLogBootScan::GetListenerName()
{
    return "Faultlogger";
}
} // namespace HiviewDFX
} // namespace OHOS
