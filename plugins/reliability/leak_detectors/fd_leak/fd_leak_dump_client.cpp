/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * Description: this file used to IPC with hidumper
 * Author: huangyu
 * Create: 2023-11-16
 */
#include "fd_leak_dump_client.h"

#include <cinttypes>
#include <iservice_registry.h>
#include <string_ex.h>
#include <unistd.h>
#include "hiview_logger.h"
#include "dump_broker_proxy.h"
#include "fault_common.h"
#include "fd_leak_dump_on_demand_load.h"

using namespace std;

namespace {
enum DumperAbilityId {
    DFX_SYS_HIDUMPER_ABILITY_ID = 1212,
};
constexpr int32_t SLEEP_DUR = 5 * 1000 * 1000;
constexpr int32_t SLEEP_UNIT = 100 * 1000;
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakDumpClient");

FdLeakDumpClient::FdLeakDumpClient()
{
}

FdLeakDumpClient::~FdLeakDumpClient()
{
    mutex_.lock();
    if (proxy_ != nullptr) {
        auto remoteObject = proxy_->AsObject();
        if (remoteObject != nullptr) {
            remoteObject->RemoveDeathRecipient(deathRecipient_);
        }
    }
    mutex_.unlock();
}

ErrCode FdLeakDumpClient::SendGetFdleakedListRequest(int32_t threshold, vector<int32_t> &leakedPids)
{
    mutex_.lock();
    if (Connect() != SUCCESSED) {
        mutex_.unlock();
        HIVIEW_LOGE("failed to connect hidumper service");
        return FAILURE;
    }
    int32_t ret = proxy_->ScanPidOverLimit("fd", threshold, leakedPids);
    if (ret < 0) {
        HIVIEW_LOGW("failed to ScanPidOverLimit, ret is %{public}d.", ret);
    }
    mutex_.unlock();
    return static_cast<ErrCode>(ret);
}

ErrCode FdLeakDumpClient::SendCountFdNumsRequest(int32_t pid,
    uint32_t &fdNums, string &detailFdInfo, vector<string> &topLeakedTypeList)
{
    mutex_.lock();
    int32_t ret = SUCCESSED;
    ret = Connect();
    if (ret != SUCCESSED) {
        mutex_.unlock();
        HIVIEW_LOGE("failed to connect hidumper service, ret is %{public}d", ret);
        return FAILURE;
    }
    ret = proxy_->CountFdNums(pid, fdNums, detailFdInfo, topLeakedTypeList);
    if (ret < 0) {
        HIVIEW_LOGW("failed to CountFdNums, ret is %{public}d", ret);
    }
    mutex_.unlock();
    return static_cast<ErrCode>(ret);
}

ErrCode FdLeakDumpClient::SendScanOrphanVnodeOverLimitRequest(int32_t fdLeakThreshold,
    int32_t orphanVnodeThreshold, vector<string> &topOrphanVnodeInfoList)
{
    mutex_.lock();
    int32_t ret = SUCCESSED;
    ret = Connect();
    if (ret != SUCCESSED) {
        mutex_.unlock();
        HIVIEW_LOGE("failed to connect hidumper service, ret is %{public}d", ret);
        return FAILURE;
    }
    ret = proxy_->ScanOrphanVnodeOverLimit(fdLeakThreshold, orphanVnodeThreshold, topOrphanVnodeInfoList);
    if (ret < 0) {
        HIVIEW_LOGW("failed to ScanOrphanVnodeOverLimit, ret is %{public}d", ret);
    }
    mutex_.unlock();
    return static_cast<ErrCode>(ret);
}

ErrCode FdLeakDumpClient::Connect()
{
    if (proxy_ != nullptr) {
        return SUCCESSED;
    }
    sptr<ISystemAbilityManager> sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sam == nullptr) {
        return ERROR_GET_SYSTEM_ABILITY_MANAGER;
    }
    sptr<IRemoteObject> remoteObject = sam->CheckSystemAbility(DFX_SYS_HIDUMPER_ABILITY_ID);
    if (remoteObject == nullptr) {
        ErrCode retStart = OnDemandStart(sam, remoteObject);
        if (remoteObject == nullptr || retStart != SUCCESSED) {
            return ERROR_GET_DUMPER_SERVICE;
        }
    }
    deathRecipient_ = sptr<IRemoteObject::DeathRecipient>(new DumpManagerDeathRecipient());
    if (deathRecipient_ == nullptr) {
        return ERR_NO_MEMORY;
    }
    if ((remoteObject->IsProxyObject()) && (!remoteObject->AddDeathRecipient(deathRecipient_))) {
        return ERROR_ADD_DEATH_RECIPIENT;
    }
    proxy_ = iface_cast<IDumpBroker>(remoteObject);
    if (proxy_ == nullptr) {
        HIVIEW_LOGE("Connect to hidumper failed, proxy_ is nullptr");
        return ERROR_GET_DUMPER_SERVICE;
    }
    HIVIEW_LOGI("sucess connect to remote service");
    return SUCCESSED;
}

bool FdLeakDumpClient::IsConnected()
{
    mutex_.lock();
    bool ret = proxy_ == nullptr;
    mutex_.unlock();
    return !ret;
}

void FdLeakDumpClient::Reset()
{
    mutex_.lock();
    if (proxy_ == nullptr) {
        mutex_.unlock();
        return;
    }
    auto serviceRemote = proxy_->AsObject();
    if (serviceRemote != nullptr) {
        serviceRemote->RemoveDeathRecipient(deathRecipient_);
        proxy_ = nullptr;
        HIVIEW_LOGI("Reset: sucess disconnected remote service");
    }
    mutex_.unlock();
}

void FdLeakDumpClient::ResetProxy(const wptr<IRemoteObject> &remote)
{
    mutex_.lock();
    if (proxy_ == nullptr) {
        mutex_.unlock();
        return;
    }
    auto serviceRemote = proxy_->AsObject();
    if ((serviceRemote != nullptr) && (serviceRemote == remote.promote())) {
        serviceRemote->RemoveDeathRecipient(deathRecipient_);
        proxy_ = nullptr;
        HIVIEW_LOGI("ResetProxy: sucess disconnected remote service");
    }
    mutex_.unlock();
}

void FdLeakDumpClient::DumpManagerDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    if (remote == nullptr) {
        return;
    }
    FdLeakDumpClient::GetInstance().ResetProxy(remote);
}

ErrCode FdLeakDumpClient::OnDemandStart(sptr<ISystemAbilityManager> sam, sptr<IRemoteObject> &remoteObject)
{
    // 如果 remoteObject 已经存在，直接返回成功
    if (remoteObject != nullptr) {
        return SUCCESSED;
    }
    if (sam == nullptr) {
        HIVIEW_LOGE("OnDemandStart sam is null.");
        return ERROR_GET_SYSTEM_ABILITY_MANAGER;
    }
    sptr<FdLeakOnDemandLoadCallback> loadCallback = new FdLeakOnDemandLoadCallback();
    int32_t result = sam->LoadSystemAbility(DFX_SYS_HIDUMPER_ABILITY_ID, loadCallback);
    if (result != SUCCESSED) {
        HIVIEW_LOGE("systemAbilityId:%{public}d load failed, result code:%{public}d",
            DFX_SYS_HIDUMPER_ABILITY_ID, result);
        return ERROR_GET_DUMPER_SERVICE;
    }

    int32_t loop = SLEEP_DUR / SLEEP_UNIT;
    while (loop-- > 0) {
        if (!loadCallback->CheckLoadSystemAbilityStatus()) {
            usleep(SLEEP_UNIT);
            continue;
        }
        remoteObject = sam->CheckSystemAbility(DFX_SYS_HIDUMPER_ABILITY_ID);
        if (remoteObject != nullptr) {
            return SUCCESSED;
        }
        usleep(SLEEP_UNIT);
    }
    HIVIEW_LOGE("on demand start fail");
    return ERROR_GET_DUMPER_SERVICE;
}

} // namespace HiviewDFX
} // namespace OHOS
