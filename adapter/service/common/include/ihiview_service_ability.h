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

#ifndef IHIVIEW_SERVICE_ABILITY_H
#define IHIVIEW_SERVICE_ABILITY_H

#include <string>

#include "hiview_file_info.h"
#include "iremote_broker.h"
#include "hiview_service_ipc_interface_code.h"
#include "collect_result_pracelable.h"
#include "client/trace_collector.h"
#include "client/memory_collector.h"

namespace OHOS {
namespace HiviewDFX {
class IHiviewServiceAbility : public IRemoteBroker {
public:
    IHiviewServiceAbility() = default;
    ~IHiviewServiceAbility() = default;

    virtual int32_t List(const std::string& logType, std::vector<HiviewFileInfo>& fileInfos) = 0;
    virtual int32_t Copy(const std::string& logType, const std::string& logName, const std::string& dest) = 0;
    virtual int32_t Move(const std::string& logType, const std::string& logName, const std::string& dest) = 0;
    virtual int32_t Remove(const std::string& logType, const std::string& logName) = 0;

    virtual CollectResultParcelable<int32_t> OpenSnapshotTrace(const std::vector<std::string>& tagGroups) = 0;
    virtual CollectResultParcelable<std::vector<std::string>> DumpSnapshotTrace(int32_t caller) = 0;
    virtual CollectResultParcelable<int32_t> OpenRecordingTrace(const std::string& tags) = 0;
    virtual CollectResultParcelable<int32_t> RecordingTraceOn() = 0;
    virtual CollectResultParcelable<std::vector<std::string>> RecordingTraceOff() = 0;
    virtual CollectResultParcelable<int32_t> CloseTrace() = 0;
    virtual CollectResultParcelable<int32_t> RecoverTrace() = 0;
    virtual CollectResultParcelable<int32_t> CaptureDurationTrace(UCollectClient::AppCaller &appCaller) = 0;
    virtual CollectResultParcelable<double> GetSysCpuUsage() = 0;
    virtual CollectResultParcelable<int32_t> SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller) = 0;
    virtual CollectResultParcelable<int32_t> GetGraphicUsage() = 0;

public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.HiviewDFX.IHiviewServiceAbility");
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // IHIVIEW_SERVICE_H