/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "hiview_service_ability_stub.h"

#include <unordered_map>
#include <vector>

#include "accesstoken_kit.h"
#include "ash_memory_utils.h"
#include "client/trace_collector.h"
#include "client/memory_collector.h"
#include "errors.h"
#include "hiview_err_code.h"
#include "ipc_skeleton.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiViewSA-HiViewServiceAbilityStub");
const std::string ASH_MEM_NAME = "HiviewLogLibrary SharedMemory";
constexpr uint32_t ASH_MEM_SIZE = 107 * 5000; // 535k

const std::unordered_map<uint32_t, std::string> ALL_PERMISSION_MAP = {
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_LIST),
        "ohos.permission.READ_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_COPY),
        "ohos.permission.READ_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_MOVE),
        "ohos.permission.WRITE_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_REMOVE),
        "ohos.permission.WRITE_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_OPEN_SNAPSHOT_TRACE),
        "ohos.permission.WRITE_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_DUMP_SNAPSHOT_TRACE),
        "ohos.permission.READ_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_OPEN_RECORDING_TRACE),
        "ohos.permission.WRITE_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECORDING_TRACE_ON),
        "ohos.permission.READ_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECORDING_TRACE_OFF),
        "ohos.permission.READ_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_CLOSE_TRACE),
        "ohos.permission.WRITE_HIVIEW_SYSTEM"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECOVER_TRACE),
        "ohos.permission.WRITE_HIVIEW_SYSTEM"}
};

const std::unordered_map<uint32_t, std::string> TRACE_PERMISSION_MAP = {
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_OPEN_SNAPSHOT_TRACE),
        "ohos.permission.DUMP"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_DUMP_SNAPSHOT_TRACE),
        "ohos.permission.DUMP"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_OPEN_RECORDING_TRACE),
        "ohos.permission.DUMP"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECORDING_TRACE_ON),
        "ohos.permission.DUMP"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECORDING_TRACE_OFF),
        "ohos.permission.DUMP"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_CLOSE_TRACE),
        "ohos.permission.DUMP"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECOVER_TRACE),
        "ohos.permission.DUMP"},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_GET_APP_TRACE), ""},
};

const std::unordered_map<uint32_t, std::string> CPU_PERMISSION_MAP = {
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_GET_SYSTEM_CPU_USAGE), ""}
};

const std::unordered_map<uint32_t, std::string> MEMORY_PERMISSION_MAP = {
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_SET_APPRESOURCE_LIMIT), ""},
    {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_GET_GRAPHIC_USAGE), ""}
};

bool HasAccessPermission(uint32_t code, const std::unordered_map<uint32_t, std::string>& permissions)
{
    using namespace Security::AccessToken;
    auto iter = permissions.find(code);
    if (iter == permissions.end()) {
        return false;
    }
    if (iter->second.empty()) {
        return true;
    }
    AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    int verifyResult = AccessTokenKit::VerifyAccessToken(callerToken, iter->second);
    if (verifyResult == PERMISSION_GRANTED) {
        return true;
    }
    HIVIEW_LOGW("%{public}s not granted, code: %{public}u", iter->second.c_str(), code);
    return false;
}

int32_t WritePracelableToMessage(MessageParcel& dest, Parcelable& data)
{
    if (!dest.WriteParcelable(&data)) {
        HIVIEW_LOGW("failed to write TraceErrorCodeWrapper to parcel");
        return TraceErrCode::ERR_WRITE_MSG_PARCEL;
    }
    return TraceErrCode::ERR_OK;
}
}

int32_t HiviewServiceAbilityStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    HIVIEW_LOGI("cmd = %{public}d, flags= %{public}d", code, option.GetFlags());
    std::u16string descripter = HiviewServiceAbilityStub::GetDescriptor();
    std::u16string remoteDescripter = data.ReadInterfaceToken();
    if (descripter != remoteDescripter) {
        return -ERR_INVALID_VALUE;
    }
    if (!IsPermissionGranted(code)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    auto requestHandler = GetRequestHandler(code);
    if (requestHandler == nullptr) {
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return requestHandler(data, reply, option);
}

bool HiviewServiceAbilityStub::IsPermissionGranted(uint32_t code)
{
    return HasAccessPermission(code, ALL_PERMISSION_MAP) || HasAccessPermission(code, TRACE_PERMISSION_MAP) ||
        HasAccessPermission(code, CPU_PERMISSION_MAP) || HasAccessPermission(code, MEMORY_PERMISSION_MAP);
}

std::unordered_map<uint32_t, RequestHandler> HiviewServiceAbilityStub::GetRequestHandlers()
{
    static std::unordered_map<uint32_t, RequestHandler> requestHandlers = {
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_LIST),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleListRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_COPY),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleCopyRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_MOVE),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleMoveRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_REMOVE),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleRemoveRequest(data, reply, option);
            }
        }
    };
    return requestHandlers;
}

std::unordered_map<uint32_t, RequestHandler> HiviewServiceAbilityStub::GetTraceRequestHandlers()
{
    static std::unordered_map<uint32_t, RequestHandler> requestHandlers = {
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_OPEN_SNAPSHOT_TRACE),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleOpenSnapshotTraceRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_DUMP_SNAPSHOT_TRACE),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleDumpSnapshotTraceRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_OPEN_RECORDING_TRACE),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleOpenRecordingTraceRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECORDING_TRACE_ON),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleRecordingTraceOnRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECORDING_TRACE_OFF),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleRecordingTraceOffRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_CLOSE_TRACE),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleCloseTraceRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECOVER_TRACE),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleRecoverTraceRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_GET_APP_TRACE),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return this->HandleCaptureDurationTraceRequest(data, reply, option);
            }
        }
    };
    return requestHandlers;
}

std::unordered_map<uint32_t, RequestHandler> HiviewServiceAbilityStub::GetCpuRequestHandlers()
{
    static std::unordered_map<uint32_t, RequestHandler> cpuRequestHandlers = {
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_GET_SYSTEM_CPU_USAGE),
         [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return HandleGetSysCpuUsageRequest(data, reply, option);
            }
        }
    };
    return cpuRequestHandlers;
}

std::unordered_map<uint32_t, RequestHandler> HiviewServiceAbilityStub::GetMemoryRequestHandlers()
{
    static std::unordered_map<uint32_t, RequestHandler> memoryRequestHandlers = {
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_SET_APPRESOURCE_LIMIT),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return HandleSetAppResourceLimitRequest(data, reply, option);
            }
        },
        {static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_GET_GRAPHIC_USAGE),
            [this] (MessageParcel& data, MessageParcel& reply, MessageOption& option) {
                return HandleGetGraphicUsageRequest(data, reply, option);
            }
        }
    };
    return memoryRequestHandlers;
}

RequestHandler HiviewServiceAbilityStub::GetRequestHandler(uint32_t code)
{
    std::vector<std::unordered_map<uint32_t, RequestHandler>> allHandlerMaps = {
        GetRequestHandlers(),
        GetTraceRequestHandlers(),
        GetCpuRequestHandlers(),
        GetMemoryRequestHandlers()
    };
    for (const auto &handlerMap : allHandlerMaps) {
        auto iter = handlerMap.find(code);
        if (iter == handlerMap.end()) {
            continue;
        }
        return iter->second;
    }
    HIVIEW_LOGE("function for handling request isn't found");
    return nullptr;
}

int32_t HiviewServiceAbilityStub::HandleListRequest(MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    std::string logType;
    if (!data.ReadString(logType)) {
        HIVIEW_LOGE("cannot get log type");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    std::vector<HiviewFileInfo> fileInfos;
    int32_t ret = List(logType, fileInfos);
    if (ret != ERR_OK) {
        return ret;
    }
    HIVIEW_LOGW("file list num:%{public}zu", fileInfos.size());
    sptr<Ashmem> ashmem = AshMemoryUtils::GetAshmem(ASH_MEM_NAME, ASH_MEM_SIZE);
    if (ashmem == nullptr) {
        HIVIEW_LOGE("ge ashmem failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    std::vector<uint32_t> allSize;
    if (!AshMemoryUtils::WriteBulkData<HiviewFileInfo>(fileInfos, ashmem, ASH_MEM_SIZE, allSize)) {
        HIVIEW_LOGE("WriteBulkData failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    if (!reply.WriteUInt32Vector(allSize)) {
        HIVIEW_LOGE("write size failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    if (!reply.WriteAshmem(ashmem)) {
        HIVIEW_LOGE("write ashmem failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    return ERR_OK;
}

int32_t HiviewServiceAbilityStub::HandleCopyRequest(MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    return HandleCopyOrMoveRequest(data, reply, option, false);
}

int32_t HiviewServiceAbilityStub::HandleMoveRequest(MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    return HandleCopyOrMoveRequest(data, reply, option, true);
}

int32_t HiviewServiceAbilityStub::HandleCopyOrMoveRequest(
    MessageParcel& data, MessageParcel& reply, MessageOption& option, bool isMove)
{
    std::string logType;
    if (!data.ReadString(logType)) {
        HIVIEW_LOGW("cannot get logtype");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    std::string logName;
    if (!data.ReadString(logName)) {
        HIVIEW_LOGW("cannot get log type");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    std::string dest;
    if (!data.ReadString(dest)) {
        HIVIEW_LOGW("cannot get dest dir");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    if (dest.find("..") != std::string::npos) {
        HIVIEW_LOGW("invalid dest: %{public}s", dest.c_str());
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    int32_t ret = isMove ? Move(logType, logName, dest) : Copy(logType, logName, dest);
    if (!reply.WriteInt32(ret)) {
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    return ERR_OK;
}

int32_t HiviewServiceAbilityStub::HandleRemoveRequest(MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    std::string logType;
    if (!data.ReadString(logType)) {
        HIVIEW_LOGW("cannot get log type");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    std::string logName;
    if (!data.ReadString(logName)) {
        HIVIEW_LOGW("cannot get log name");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    int32_t ret = Remove(logType, logName);
    if (!reply.WriteInt32(ret)) {
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    return ERR_OK;
}

int32_t HiviewServiceAbilityStub::HandleOpenSnapshotTraceRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    std::vector<std::string> tagGroups;
    if (!data.ReadStringVector(&tagGroups)) {
        HIVIEW_LOGW("failed to read tag groups from parcel");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }
    auto ret = OpenSnapshotTrace(tagGroups);
    return WritePracelableToMessage(reply, ret);
}

int32_t HiviewServiceAbilityStub::HandleDumpSnapshotTraceRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    int32_t caller = UCollect::TraceCaller::OTHER;
    if (!data.ReadInt32(caller)) {
        HIVIEW_LOGW("failed to read trace caller from parcel");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }
    auto ret = DumpSnapshotTrace(caller);
    return WritePracelableToMessage(reply, ret);
}

int32_t HiviewServiceAbilityStub::HandleOpenRecordingTraceRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    std::string tags;
    if (!data.ReadString(tags)) {
        HIVIEW_LOGW("failed to read tags from parcel");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }
    auto ret = OpenRecordingTrace(tags);
    return WritePracelableToMessage(reply, ret);
}

int32_t HiviewServiceAbilityStub::HandleRecordingTraceOnRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    auto ret = RecordingTraceOn();
    return WritePracelableToMessage(reply, ret);
}

int32_t HiviewServiceAbilityStub::HandleRecordingTraceOffRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    auto ret = RecordingTraceOff();
    return WritePracelableToMessage(reply, ret);
}

int32_t HiviewServiceAbilityStub::HandleCloseTraceRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    auto ret = CloseTrace();
    return WritePracelableToMessage(reply, ret);
}

int32_t HiviewServiceAbilityStub::HandleRecoverTraceRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    auto ret = RecoverTrace();
    return WritePracelableToMessage(reply, ret);
}

static bool ReadAppCallerBase(MessageParcel& data, UCollectClient::AppCaller &appCaller, std::string &errField)
{
    if (!data.ReadInt32(appCaller.actionId)) {
        errField = "actionId";
        return false;
    }

    if (!data.ReadString(appCaller.bundleName)) {
        errField = "bundleName";
        return false;
    }

    if (!data.ReadString(appCaller.bundleVersion)) {
        errField = "bundleVersion";
        return false;
    }

    if (!data.ReadString(appCaller.threadName)) {
        errField = "threadName";
        return false;
    }

    if (!data.ReadInt32(appCaller.foreground)) {
        errField = "foreground";
        return false;
    }
    return true;
}

static bool ReadAppCallerExternal(MessageParcel& data, UCollectClient::AppCaller &appCaller, std::string &errField)
{
    if (!data.ReadInt32(appCaller.uid)) {
        errField = "uid";
        return false;
    }

    if (!data.ReadInt32(appCaller.pid)) {
        errField = "pid";
        return false;
    }

    if (!data.ReadInt64(appCaller.happenTime)) {
        errField = "happenTime";
        return false;
    }

    if (!data.ReadInt64(appCaller.beginTime)) {
        errField = "beginTime";
        return false;
    }

    if (!data.ReadInt64(appCaller.endTime)) {
        errField = "endTime";
        return false;
    }

    if (!data.ReadBool(appCaller.isBusinessJank)) {
        errField = "isBusinessJank";
        return false;
    }
    return true;
}

int32_t HiviewServiceAbilityStub::HandleCaptureDurationTraceRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    UCollectClient::AppCaller appCaller;

    std::string errField;
    do {
        if (!ReadAppCallerBase(data, appCaller, errField)) {
            break;
        }
        if (!ReadAppCallerExternal(data, appCaller, errField)) {
            break;
        }
    } while (0);

    if (!errField.empty()) {
        HIVIEW_LOGW("failed to read %{public}s from parcel", errField.c_str());
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }

    auto ret = CaptureDurationTrace(appCaller);
    return WritePracelableToMessage(reply, ret);
}

int32_t HiviewServiceAbilityStub::HandleGetSysCpuUsageRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    auto ret = GetSysCpuUsage();
    return WritePracelableToMessage(reply, ret);
}

int32_t HiviewServiceAbilityStub::HandleSetAppResourceLimitRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    if (!Parameter::IsBetaVersion() && !Parameter::IsLeakStateMode()) {
        HIVIEW_LOGE("Called SetAppResourceLimitRequest service failed.");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }
    UCollectClient::MemoryCaller memoryCaller;
    if (!data.ReadInt32(memoryCaller.pid)) {
        HIVIEW_LOGW("HandleSetAppResourceLimitRequest failed to read pid from parcel");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }

    if (!data.ReadString(memoryCaller.resourceType)) {
        HIVIEW_LOGW("HandleSetAppResourceLimitRequest failed to read type from parcel");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }

    if (!data.ReadInt32(memoryCaller.limitValue)) {
        HIVIEW_LOGW("HandleSetAppResourceLimitRequest failed to read value from parcel");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }

    if (!data.ReadBool(memoryCaller.enabledDebugLog)) {
        HIVIEW_LOGW("HandleSetAppResourceLimitRequest failed to read enabledDebugLog from parcel");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }
    memoryCaller.pid = IPCObjectStub::GetCallingPid();
    if (memoryCaller.pid < 0) {
        return TraceErrCode::ERR_SEND_REQUEST;
    }
    auto ret = SetAppResourceLimit(memoryCaller);
    return WritePracelableToMessage(reply, ret);
}

int32_t HiviewServiceAbilityStub::HandleGetGraphicUsageRequest(MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    int32_t pid = IPCObjectStub::GetCallingPid();
    if (pid < 0) {
        return TraceErrCode::ERR_SEND_REQUEST;
    }
    auto ret = GetGraphicUsage(pid);
    return WritePracelableToMessage(reply, ret);
}
} // namespace HiviewDFX
} // namespace OHOS