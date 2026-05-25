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

#include <map>
#include <memory>

#include "ability_manager_client.h"
#include "ability_manager_proxy.h"
#include "common_utils.h"
#include "faultlog_bootscan.h"
#include "faultlog_bundle_util.h"
#include "faultlog_dump.h"
#include "faultlog_event_factory.h"
#include "faultlog_manager.h"
#include "faultlog_util.h"
#include "hiview_logger.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
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
constexpr const char* DHA_SERVICE_NAME = "dha_service";
constexpr int32_t DHA_SERVICE_UID = 7780;
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

bool FaultloggerBase::EnableGwpAsanGrayscale(GwpAsanParams gwpAsanParams,
                                             int32_t uid)
{
    std::string bundleName = GetApplicationNameById(uid);
    if (bundleName.empty()) {
        HIVIEW_LOGE("Enable gwpAsanGrayscale failed, the bundleName is not exist");
        return false;
    }
    HIVIEW_LOGD("EnableGwpAsanGrayscale success, bundleName: %{public}s, alwaysEnabled: %{public}d "
        ", sampleRate: %{public}f, maxSimutaneousAllocations: %{public}f "
        ", duration: %{public}d, isRecover: %{public}d",
        bundleName.c_str(), gwpAsanParams.alwaysEnabled, gwpAsanParams.sampleRate,
        gwpAsanParams.maxSimutaneousAllocations, gwpAsanParams.duration, gwpAsanParams.isRecover);
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
    int sampleRateInt = static_cast<int>(std::ceil(gwpAsanParams.sampleRate));
    int slotInt = static_cast<int>(std::ceil(gwpAsanParams.maxSimutaneousAllocations));
    std::string sample = std::to_string(sampleRateInt) + ":" + std::to_string(slotInt);
    system::SetParameter("gwp_asan.enable.app." + bundleName, gwpAsanParams.alwaysEnabled ? "true" : "false");
    system::SetParameter("gwp_asan.recoverable.app." + bundleName, gwpAsanParams.isRecover ? "true" : "false");
    system::SetParameter("gwp_asan.sample.app." + bundleName, sample);

    uint64_t beginTime = static_cast<uint64_t>(std::time(nullptr));
    system::SetParameter("gwp_asan.gray_begin.app." + bundleName, std::to_string(beginTime));
    system::SetParameter("gwp_asan.gray_days.app." + bundleName, std::to_string(gwpAsanParams.duration));
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
    system::SetParameter("gwp_asan.recoverable.app." + bundleName, "");
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

bool FaultloggerBase::EnableGwpAsanInner(GwpAsanParams gwpAsanParams, const std::string& processName)
{
    if (!CheckCallerIsAllowed()) {
        HIVIEW_LOGE("Enable EnableGwpAsanInner failed, the caller is not permitted");
        return false;
    }
    
    HIVIEW_LOGD("EnableGwpAsanInner success, process: %{public}s, alwaysEnabled: %{public}d "
        ", sampleRate: %{public}f, maxSimutaneousAllocations: %{public}f "
        ", duration: %{public}d, isRecover: %{public}d",
        processName.c_str(), gwpAsanParams.alwaysEnabled, gwpAsanParams.sampleRate,
        gwpAsanParams.maxSimutaneousAllocations, gwpAsanParams.duration, gwpAsanParams.isRecover);

    // force sample process in any version
    system::SetParameter("gwp_asan.sample.service.forcible", "true");

    int sampleRateInt = static_cast<int>(std::ceil(gwpAsanParams.sampleRate));
    int slotInt = static_cast<int>(std::ceil(gwpAsanParams.maxSimutaneousAllocations));
    int pid = CommonUtils::GetPidByName(processName);
    if (pid < 0) {
        HIVIEW_LOGE("EnableGwpAsanInner failed, get pid failed, process: %{public}s", processName.c_str());
        return false;
    }
    
    int32_t uid = CommonUtils::GetUidByPid(pid);
    if (uid < 0) {
        HIVIEW_LOGE("EnableGwpAsanInner failed, get uid failed, process: %{public}s", processName.c_str());
        return false;
    }
    std::string sample = std::to_string(sampleRateInt) + ":" + std::to_string(slotInt);
    system::SetParameter("gwp_asan.enable.app." + std::to_string(uid), gwpAsanParams.alwaysEnabled ? "true" : "false");
    system::SetParameter("gwp_asan.recoverable.app." + std::to_string(uid), gwpAsanParams.isRecover ? "true" : "false");
    system::SetParameter("gwp_asan.sample.app." + std::to_string(uid), sample);

    uint64_t beginTime = static_cast<uint64_t>(std::time(nullptr));
    system::SetParameter("gwp_asan.gray_begin.app." + std::to_string(uid), std::to_string(beginTime));
    system::SetParameter("gwp_asan.gray_days.app." + std::to_string(uid), std::to_string(gwpAsanParams.duration));
    return true;
}

bool FaultloggerBase::CheckCallerIsAllowed()
{
    int32_t uid = IPCSkeleton::GetCallingUid();
    int32_t pid = IPCSkeleton::GetCallingPid();
    std::string processName = CommonUtils::GetProcFullNameByPid(pid);
    static const std::map<int32_t, std::string> allowedCallerMap = {
        {DHA_SERVICE_UID, DHA_SERVICE_NAME},
    };
    auto it = allowedCallerMap.find(uid);
    if (it == allowedCallerMap.end() || it->second != processName) {
        HIVIEW_LOGE("the caller [uid=%{public}d, process=%{public}s] is not permitted", uid, processName.c_str());
        return false;
    }
    
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
