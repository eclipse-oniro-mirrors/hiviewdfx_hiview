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

#ifndef HIVIEW_SERVICE_ABILITY_H
#define HIVIEW_SERVICE_ABILITY_H

#include <string>
#include <memory>

#include "hiview_err_code.h"
#include "hiview_file_info.h"
#include "hiview_service.h"
#include "hiview_service_ability_stub.h"
#include "ihiview_service_ability.h"
#include "hiview_logger.h"
#include "singleton.h"
#include "system_ability.h"
#include "client/trace_collector.h"
#include "client/memory_collector.h"

namespace OHOS {
namespace HiviewDFX {
class HiviewServiceAbility : public SystemAbility,
    public HiviewServiceAbilityStub {
    DECLARE_SYSTEM_ABILITY(HiviewServiceAbility);

public:
    HiviewServiceAbility();
    ~HiviewServiceAbility();
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
    static void StartService(HiviewService *service);
    static void StartServiceAbility(int sleepS);
    static HiviewService* GetOrSetHiviewService(HiviewService *service = nullptr);

    int32_t List(const std::string& logType, std::vector<HiviewFileInfo>& fileInfos) override;
    int32_t Copy(const std::string& logType, const std::string& logName, const std::string& dest) override;
    int32_t Move(const std::string& logType, const std::string& logName, const std::string& dest) override;
    int32_t Remove(const std::string& logType, const std::string& logName) override;

    CollectResultParcelable<int32_t> OpenSnapshotTrace(const std::vector<std::string>& tagGroups) override;
    CollectResultParcelable<std::vector<std::string>> DumpSnapshotTrace(int32_t caller) override;
    CollectResultParcelable<int32_t> OpenRecordingTrace(const std::string& tags) override;
    CollectResultParcelable<int32_t> RecordingTraceOn() override;
    CollectResultParcelable<std::vector<std::string>> RecordingTraceOff() override;
    CollectResultParcelable<int32_t> CloseTrace() override;
    CollectResultParcelable<int32_t> RecoverTrace() override;
    CollectResultParcelable<int32_t> CaptureDurationTrace(UCollectClient::AppCaller &appCaller) override;
    CollectResultParcelable<double> GetSysCpuUsage() override;
    CollectResultParcelable<int32_t> SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller) override;

protected:
    void OnDump() override;
    void OnStart() override;
    void OnStop() override;
    CollectResultParcelable<int32_t> GetGraphicUsage(int32_t pid) override;

private:
    template<typename T>
    CollectResultParcelable<T> TraceCalling(std::function<CollectResult<T>(HiviewService*)> traceRetHandler)
    {
        auto traceRet = CollectResultParcelable<T>::Init();
        if (traceRetHandler == nullptr) {
            return traceRet;
        }
        auto service = GetOrSetHiviewService();
        if (service == nullptr) {
            return traceRet;
        }
        auto collectRet = traceRetHandler(service);
        return CollectResultParcelable<T>(collectRet);
    }

private:
    void GetFileInfoUnderDir(const std::string& dirPath, std::vector<HiviewFileInfo>& fileInfos);
    int32_t CopyOrMoveFile(
        const std::string& logType, const std::string& logName, const std::string& dest, bool isMove);
};

class HiviewServiceAbilityDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    HiviewServiceAbilityDeathRecipient();
    ~HiviewServiceAbilityDeathRecipient();
    void OnRemoteDied(const wptr<IRemoteObject> &object) override;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // IHIVIEW_SERVICE_H