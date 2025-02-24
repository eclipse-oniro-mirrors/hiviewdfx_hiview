/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "app_caller_parcelable.h"
#include "hiview_err_code.h"
#include "hiview_file_info.h"
#include "hiview_service.h"
#include "hiview_service_ability_stub.h"
#include "hiview_logger.h"
#include "memory_caller_parcelable.h"
#include "singleton.h"
#include "system_ability.h"

namespace OHOS {
namespace HiviewDFX {
class HiviewServiceAbility : public SystemAbility, public HiviewServiceAbilityStub {
    DECLARE_SYSTEM_ABILITY(HiviewServiceAbility);

public:
    HiviewServiceAbility();
    ~HiviewServiceAbility();
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
    static void StartService(HiviewService *service);
    static void StartServiceAbility(int sleepS);
    static HiviewService* GetOrSetHiviewService(HiviewService *service = nullptr);

    ErrCode ListFiles(const std::string& logType, std::vector<HiviewFileInfo>& fileInfos) override;
    ErrCode Copy(const std::string& logType, const std::string& logName, const std::string& dest) override;
    ErrCode Move(const std::string& logType, const std::string& logName, const std::string& dest) override;
    ErrCode Remove(const std::string& logType, const std::string& logName) override;

    ErrCode OpenSnapshotTrace(const std::vector<std::string>& tagGroups, int32_t& errNo, int32_t& ret) override;
    ErrCode DumpSnapshotTrace(int32_t client, int32_t& errNo, std::vector<std::string>& files) override;
    ErrCode OpenRecordingTrace(const std::string& tags, int32_t& errNo, int32_t& ret) override;
    ErrCode RecordingTraceOn(int32_t& errNo, int32_t& ret) override;
    ErrCode RecordingTraceOff(int32_t& errNo, std::vector<std::string>& files) override;
    ErrCode CloseTrace(int32_t& errNo, int32_t& ret) override;
    ErrCode CaptureDurationTrace(const AppCallerParcelable& appCaller, int32_t& errNo, int32_t& ret) override;
    ErrCode GetSysCpuUsage(int32_t& errNo, double& ret) override;
    ErrCode SetAppResourceLimit(
        const MemoryCallerParcelable& memoryCallerParcelable, int32_t& errNo, int32_t& ret) override;
    ErrCode SetSplitMemoryValue(
        const std::vector<MemoryCallerParcelable>& memCallerParcelableList, int32_t& errNo, int32_t& ret) override;
    ErrCode GetGraphicUsage(int32_t& errNo, int32_t& ret) override;

protected:
    void OnDump() override;
    void OnStart() override;
    void OnStop() override;

private:
    template<typename T>
    void TraceCalling(std::function<CollectResult<T>(HiviewService*)> traceRetHandler, int32_t& errNo, T& ret)
    {
        if (auto service = GetOrSetHiviewService(); service == nullptr) {
            errNo = UCollect::UcError::UNSUPPORT;
        } else {
            CollectResult<T> collectRet = traceRetHandler(service);
            errNo = collectRet.retCode;
            ret = collectRet.data;
        }
    }

private:
    void GetFileInfoUnderDir(const std::string& dirPath, std::vector<HiviewFileInfo>& fileInfos);
    ErrCode CopyOrMoveFile(
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