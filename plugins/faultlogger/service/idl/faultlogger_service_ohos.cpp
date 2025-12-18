/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "faultlogger_service_ohos.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "export_faultlogger_interface.h"
#include "faultlog_info_inner.h"
#include "faultlog_info_ohos.h"
#include "faultlog_query_result_inner.h"
#include "faultlog_query_result_ohos.h"
#include "faultlogger.h"
#include "hiview_logger.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

DEFINE_LOG_LABEL(0xD002D11, "FaultloggerServiceOhos");
namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int32_t UID_SHELL = 2000;
constexpr int32_t UID_ROOT = 0;
constexpr int32_t UID_HIDUMPER = 1212;
constexpr int32_t UID_HIVIEW = 1201;
constexpr int32_t UID_FAULTLOGGERD = 1202;
}
void FaultloggerServiceOhos::ClearQueryStub(int32_t uid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    queries_.erase(uid);
}

int32_t FaultloggerServiceOhos::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    int32_t uid = IPCSkeleton::GetCallingUid();
    if ((uid != UID_SHELL) && (uid != UID_ROOT) && (uid != UID_HIDUMPER)) {
        dprintf(fd, "No permission for uid:%d.\n", uid);
        return -1;
    }

    std::vector<std::string> cmdList;
    for (auto arg : args) {
        for (auto c : arg) {
            if (!isalnum(c) && c != '-' && c != ' ' && c != '_' && c != '.') {
                dprintf(fd, "string arg contain invalid char:%c.\n", c);
                return -1;
            }
        }
    }
    std::transform(args.begin(), args.end(), std::back_inserter(cmdList), [](const std::u16string &arg) {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        return converter.to_bytes(arg);
    });

    auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        dprintf(fd, "Service is not ready.\n");
        return -1;
    }
    instance->FaultLogDumpByCommands(fd, cmdList);
    return 0;
}

void FaultloggerServiceOhos::StartService()
{
    sptr<ISystemAbilityManager> serviceManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (serviceManager == nullptr) {
        HIVIEW_LOGE("Failed to find samgr, exit.");
        return;
    }

    static sptr<FaultloggerServiceOhos> instance = new FaultloggerServiceOhos();
    serviceManager->AddSystemAbility(DFX_FAULT_LOGGER_ABILITY_ID, instance);
    HIVIEW_LOGI("FaultLogger Service started.");
}

void FaultloggerServiceOhos::AddFaultLog(const FaultLogInfoOhos& info)
{
    int32_t uid = IPCSkeleton::GetCallingUid();
    HIVIEW_LOGD("info.uid:%{public}d uid:%{public}d info.pid:%{public}d", info.uid, uid, info.pid);
    if ((uid != static_cast<int32_t>(getuid())) && uid != info.uid && uid != UID_FAULTLOGGERD) {
        HIVIEW_LOGW("Fail to add fault log, mismatch uid:%{public}d(%{public}d)", uid, info.uid);
        return;
    }

    auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        return;
    }
    FaultLogInfo outInfo;
    outInfo.time = info.time;
    outInfo.id = info.uid;
    outInfo.pid = info.pid;
    auto fdDeleter = [] (int32_t *ptr) {
        if (*ptr > 0) {
            close(*ptr);
        }
        delete ptr;
    };
    outInfo.pipeFd.reset(new int32_t(info.pipeFd), fdDeleter);
    outInfo.faultLogType = info.faultLogType;
    outInfo.fd = (info.fd > 0) ? dup(info.fd) : -1;
    outInfo.logFileCutoffSizeBytes = info.logFileCutoffSizeBytes;
    outInfo.module = info.module;
    outInfo.reason = info.reason;
    outInfo.summary = info.summary;
    if (uid == UID_HIVIEW) {
        outInfo.logPath = info.logPath;
    }
    outInfo.registers = info.registers;
    outInfo.sectionMap = info.sectionMaps;
    instance->AddFaultLog(static_cast<FaultLogType>(outInfo.faultLogType), outInfo);
}

sptr<IRemoteObject> FaultloggerServiceOhos::QuerySelfFaultLog(int32_t faultType, int32_t maxNum)
{
    int32_t uid = IPCSkeleton::GetCallingUid();
    int32_t pid = IPCSkeleton::GetCallingPid();
    std::lock_guard<std::mutex> lock(mutex_);
    if ((queries_.find(uid) != queries_.end()) &&
        (queries_[uid]->pid == pid)) {
        HIVIEW_LOGW("Ongoing query is still alive for uid:%d", uid);
        return nullptr;
    }

    if ((faultType < FaultLogType::ALL) || (faultType > FaultLogType::APP_FREEZE)) {
        HIVIEW_LOGW("Unsupported fault type");
        return nullptr;
    }

    auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        return nullptr;
    }

    auto queryResult =
        std::make_unique<FaultLogQueryResultInner>(instance->QuerySelfFaultLog(uid, pid, faultType, maxNum));
    if (queryResult == nullptr) {
        HIVIEW_LOGW("Fail to query fault log for uid:%d", uid);
        return nullptr;
    }

    sptr<FaultLogQueryResultOhos> resultRef =
        new FaultLogQueryResultOhos(std::move(queryResult));
    auto queryStub = std::make_unique<FaultLogQuery>();
    queryStub->pid = pid;
    queryStub->ptr = resultRef;
    queries_[uid] = std::move(queryStub);
    return resultRef->AsObject();
}

bool FaultloggerServiceOhos::EnableGwpAsanGrayscale(bool alwaysEnabled, double sampleRate,
    double maxSimutaneousAllocations, int32_t duration)
{
    if (sampleRate <= 0 || maxSimutaneousAllocations <= 0 || duration <= 0) {
        HIVIEW_LOGE("failed to enable gwp asan grayscale, sampleRate: %{public}f"
            ", maxSimutaneousAllocations: %{public}f,  duration: %{public}d.",
            sampleRate, maxSimutaneousAllocations, duration);
        return false;
    }
    auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        return false;
    }
    int32_t uid = IPCSkeleton::GetCallingUid();
    bool result = instance->EnableGwpAsanGrayscale(alwaysEnabled, sampleRate,
        maxSimutaneousAllocations, duration, uid);
    return result;
}

void FaultloggerServiceOhos::DisableGwpAsanGrayscale()
{
    auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        return;
    }

    int32_t uid = IPCSkeleton::GetCallingUid();
    instance->DisableGwpAsanGrayscale(uid);
}

uint32_t FaultloggerServiceOhos::GetGwpAsanGrayscaleState()
{
    auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        return 0;
    }

    int32_t uid = IPCSkeleton::GetCallingUid();
    return instance->GetGwpAsanGrayscaleState(uid);
}

void FaultloggerServiceOhos::Destroy()
{
    int32_t uid = IPCSkeleton::GetCallingUid();
    HIVIEW_LOGI("Destroy Query from uid:%d", uid);
    ClearQueryStub(uid);
}
}  // namespace HiviewDFX
}  // namespace OHOS
