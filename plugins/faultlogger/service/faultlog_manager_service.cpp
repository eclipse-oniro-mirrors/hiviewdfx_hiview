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

#include "faultlog_manager_service.h"

#include "common_utils.h"
#include "hiview_logger.h"
#include "faultlog_bundle_util.h"
#include "faultlog_dump.h"
#include "faultlog_processor_factory.h"
#include "parameters.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
namespace {
constexpr int32_t MAX_QUERY_NUM = 100;
constexpr int MIN_APP_UID = 10000;
constexpr int MAX_APPLICATION_ENABLE = 20;
constexpr uint64_t DEFAULT_DURATION = 7;
constexpr uint64_t DAYS_TO_MILLISEC = 24 * 60 * 60;
} // namespace

void FaultLogManagerService::Dump(int fd, const std::vector<std::string>& cmds)
{
    if (!faultLogManager_) {
        return;
    }
    FaultLogDump faultLogDump(fd, faultLogManager_);
    faultLogDump.DumpByCommands(cmds);
}

void FaultLogManagerService::AddFaultLog(FaultLogInfo& info)
{
    if (!faultLogManager_) {
        return;
    }
    FaultLogProcessorFactory factory;
    auto processor = factory.CreateFaultLogProcessor(static_cast<FaultLogType>(info.faultLogType));
    if (processor) {
        processor->AddFaultLog(info, workLoop_, faultLogManager_);
    } else {
        HIVIEW_LOGW("Failed to create the faultlog processor");
    }
}

std::unique_ptr<FaultLogQueryResultInner> FaultLogManagerService::QuerySelfFaultLog(int32_t id,
    int32_t pid, int32_t faultType, int32_t maxNum)
{
    if (!faultLogManager_) {
        return nullptr;
    }
    if ((faultType < FaultLogType::ALL) || (faultType > FaultLogType::APP_FREEZE)) {
        HIVIEW_LOGW("Unsupported fault type");
        return nullptr;
    }

    if (maxNum < 0 || maxNum > MAX_QUERY_NUM) {
        maxNum = MAX_QUERY_NUM;
    }

    std::string name;
    if (id >= MIN_APP_UID) {
        name = GetApplicationNameById(id);
    }

    if (name.empty()) {
        name = CommonUtils::GetProcNameByPid(pid);
    }
    return std::make_unique<FaultLogQueryResultInner>(faultLogManager_->GetFaultInfoList(name, id, faultType, maxNum));
}

bool FaultLogManagerService::EnableGwpAsanGrayscale(bool alwaysEnabled, double sampleRate,
    double maxSimutaneousAllocations, int32_t duration, int32_t uid)
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

void FaultLogManagerService::DisableGwpAsanGrayscale(int32_t uid)
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

uint32_t FaultLogManagerService::GetGwpAsanGrayscaleState(int32_t uid)
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
}  // namespace HiviewDFX
}  // namespace OHOS
