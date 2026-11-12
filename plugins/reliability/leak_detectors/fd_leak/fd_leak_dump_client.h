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
#ifndef DFR_FD_LEAK_DUMP_CLIENT_H
#define DFR_FD_LEAK_DUMP_CLIENT_H
#include <string>
#include <singleton.h>
#include <iservice_registry.h>
#include "idump_broker.h"
#include "fault_common.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {
class FdLeakDumpClient final : public DelayedRefSingleton<FdLeakDumpClient> {
    DECLARE_DELAYED_REF_SINGLETON(FdLeakDumpClient)
public:
    DISALLOW_COPY_AND_MOVE(FdLeakDumpClient);
    // Used for dump request
    ErrCode SendGetFdleakedListRequest(int32_t threshold, std::vector<int32_t> &leakedPids);
    ErrCode SendCountFdNumsRequest(int32_t pid, uint32_t &fdNums,
        std::string &detailFdInfo, std::vector<std::string> &topLeakedType);
    // Used for scan orphan vnode over limit
    ErrCode SendScanOrphanVnodeOverLimitRequest(int32_t fdLeakThreshold, int32_t orphanVnodeThreshold,
        std::vector<std::string> &topOrphanVnodeInfoList);

public:
    // Used for connect to hidump SA.
    ErrCode Connect();
    bool IsConnected();
    // Used for reset connect.
    void Reset();
private:
    class DumpManagerDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        DumpManagerDeathRecipient() = default;
        ~DumpManagerDeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote);
    private:
        DISALLOW_COPY_AND_MOVE(DumpManagerDeathRecipient);
    };
private:
    void ResetProxy(const wptr<IRemoteObject> &remote);
    ErrCode OnDemandStart(sptr<ISystemAbilityManager> sam, sptr<IRemoteObject> &remoteObject);
private:
    sptr<IDumpBroker> proxy_ {nullptr};
    sptr<IRemoteObject::DeathRecipient> deathRecipient_ {nullptr};
    ffrt::mutex mutex_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FD_LEAK_DUMP_CLIENT_H