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

#include "faultlogger_base.h"

#include <memory>

#include "ability_manager_client.h"
#include "ability_manager_proxy.h"
#include "common_utils.h"
#include "faultlog_bootscan.h"
#include "faultlog_bundle_util.h"
#include "faultlog_dump.h"
#include "faultlog_util.h"
#include "faultlog_event_factory.h"
#include "faultlog_manager.h"
#include "hiview_logger.h"
#include "if_system_ability_manager.h"
#include "parameters.h"
#include "sanitizer_telemetry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger_Base");
extern "C" {
FaultloggerInterface* NewFaultloggerInterface(void)
{
    static FaultloggerBase faultloggerBase;
    return &faultloggerBase;
}
} // extern "C"

namespace {
constexpr int32_t MAX_QUERY_NUM = 100;
constexpr int MIN_APP_UID = 10000;
constexpr int MAX_APPLICATION_ENABLE = 20;
constexpr uint64_t DEFAULT_DURATION = 7;
constexpr uint64_t DAYS_TO_MILLISEC = 24 * 60 * 60;
}

FaultloggerBase::FaultloggerBase()
{
    HIVIEW_LOGE("constructor FaultloggerBase");
}

FaultloggerBase::~FaultloggerBase()
{
    HIVIEW_LOGE("destructor FaultloggerBase");
}

void FaultloggerBase::FaultLogDumpByCommands(int fd, const std::vector<std::string>& cmds)
{
    auto manager = std::make_shared<FaultLogManager>();
    manager->Init();
    FaultLogDump faultLogDump(fd, manager);
    faultLogDump.DumpByCommands(cmds);
}

bool FaultloggerBase::ProcessFaultLogEvent(const std::string& eventName, std::shared_ptr<Event>& event)
{
    auto type = GetLogTypeByEventName(eventName);
    auto faultLogEvent = FaultLogEventFactory::CreateFaultLogEvent(type);
    if (faultLogEvent) {
        return faultLogEvent->AddFaultLog(event);
    }
    return false;
}

void FaultloggerBase::AddFaultLog(FaultLogType type, FaultLogInfo& info)
{
    auto faultLogEvent = FaultLogEventFactory::CreateFaultLogEvent(type);
    if (faultLogEvent) {
        faultLogEvent->AddFaultLog(info);
    }
}

std::list<FaultLogInfo> FaultloggerBase::QuerySelfFaultLog(int32_t uid, int32_t pid, int32_t faultType, int32_t maxNum)
{
    if (maxNum < 0 || maxNum > MAX_QUERY_NUM) {
        maxNum = MAX_QUERY_NUM;
    }

    std::string name;
    if (uid >= MIN_APP_UID) {
        name = GetApplicationNameById(uid);
    }

    if (name.empty()) {
        name = CommonUtils::GetProcNameByPid(pid);
    }
    return FaultLogDatabase::GetFaultInfoList(name, uid, faultType, maxNum);
}

void FaultloggerBase::StartFaultLogBootScan()
{
    FaultLogBootScan::StartBootScan();
}

void FaultloggerBase::SanitizerHandleUnorderedEvent(const Event& msg)
{
    SanitizerTelemetry sanitizerTelemetry;
    sanitizerTelemetry.OnUnorderedEvent(msg);
}

bool FaultloggerBase::EnableGwpAsanGrayscale(bool alwaysEnabled,
                                             double sampleRate,
                                             double maxSimutaneousAllocations,
                                             int32_t duration,
                                             int32_t uid)
{
    std::string bundleName = GetApplicationNameById(uid);
    if (bundleName.empty()) {
        HIVIEW_LOGE("Enable gwpAsanGrayscale failed, the bundleName is not exist");
        return false;
    }
    HIVIEW_LOGD("EnableGwpAsanGrayscale success, bundleName: %{public}s, alwaysEnabled: %{public}d "
        ", sampleRate: %{public}f, maxSimutaneousAllocations: %{public}f, duration: %{public}d",
        bundleName.c_str(), alwaysEnabled, sampleRate, maxSimutaneousAllocations, duration);
    std::string isEnable = system::GetParameter("gwp_asan.enable.app." + bundleName, "");
    std::string appNumStr = system::GetParameter("gwp_asan.app_num", "0");
    int appNum = 0;
    std::stringstream ss(appNumStr);
    if (!(ss >> appNum)) {
        HIVIEW_LOGE("Invalid appNum value: %{public}s", appNumStr.c_str());
        appNum = 0;
    }
    if (isEnable.empty() && appNum >= MAX_APPLICATION_ENABLE) {
        HIVIEW_LOGE("Enable gwpAsanGrayscale failed, the maximum quantity exceeds 20");
        return false;
    }
    if (isEnable.empty()) {
        system::SetParameter("gwp_asan.app_num", std::to_string(appNum + 1));
    }
    int sampleRateInt = static_cast<int>(std::ceil(sampleRate));
    int slotInt = static_cast<int>(std::ceil(maxSimutaneousAllocations));
    std::string sample = std::to_string(sampleRateInt) + ":" + std::to_string(slotInt);
    system::SetParameter("gwp_asan.enable.app." + bundleName, alwaysEnabled ? "true" : "false");
    system::SetParameter("gwp_asan.sample.app." + bundleName, sample);

    uint64_t beginTime = static_cast<uint64_t>(std::time(nullptr));
    system::SetParameter("gwp_asan.gray_begin.app." + bundleName, std::to_string(beginTime));
    system::SetParameter("gwp_asan.gray_days.app." + bundleName, std::to_string(duration));
    return true;
}

void FaultloggerBase::DisableGwpAsanGrayscale(int32_t uid)
{
    std::string bundleName = GetApplicationNameById(uid);
    if (bundleName.empty()) {
        HIVIEW_LOGE("Disable gwpAsanGrayscale failed, the bundleName is not exist");
        return;
    }
    system::SetParameter("gwp_asan.gray_begin.app." + bundleName, "");
    system::SetParameter("gwp_asan.gray_days.app." + bundleName, "");
    system::SetParameter("gwp_asan.enable.app." + bundleName, "");
    system::SetParameter("gwp_asan.sample.app." + bundleName, "");
}

uint32_t FaultloggerBase::GetGwpAsanGrayscaleState(int32_t uid)
{
    std::string bundleName = GetApplicationNameById(uid);
    if (bundleName.empty()) {
        HIVIEW_LOGE("Get gwpAsanGrayscale state failed, the bundleName is not exist");
        return 0;
    }
    std::string beginTimeStr = system::GetParameter("gwp_asan.gray_begin.app." + bundleName, "0");
    uint64_t beginTime = 0;
    std::stringstream beginSs(beginTimeStr);
    if (!(beginSs >> beginTime)) {
        HIVIEW_LOGE("Invalid beginTime value: %{public}s", beginTimeStr.c_str());
        return 0;
    }

    std::string durationStr = system::GetParameter("gwp_asan.gray_days.app." + bundleName, "7");
    uint64_t durationDays = DEFAULT_DURATION;
    std::stringstream durationSs(durationStr);
    if (!(durationSs >> durationDays)) {
        durationDays = DEFAULT_DURATION;
    }

    uint64_t now = static_cast<uint64_t>(std::time(nullptr));
    if (now < beginTime) {
        return 0;
    }

    uint64_t lastDays = (now - beginTime) / (DAYS_TO_MILLISEC);
    return static_cast<uint32_t>(durationDays > lastDays ? durationDays - lastDays : 0);
}

uint64_t FaultloggerBase::GetExtensionDelayTime()
{
    constexpr uint64_t defaultDelayTimeSeconds = 30 * 60; // 30min
    constexpr uint64_t maxDelayTimeSeconds = 3 * 60 * 60; // 3h
    return OHOS::system::GetUintParameter("faultloggerd.extension.delay",
        defaultDelayTimeSeconds, maxDelayTimeSeconds);
}
} // namespace HiviewDFX
} // namespace OHOS
