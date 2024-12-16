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

#include "hiview_service_ability_proxy.h"

#include "ash_memory_utils.h"
#include "collect_result.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiviewServiceAbilityProxy");
}

int32_t HiviewServiceAbilityProxy::List(const std::string& logType, std::vector<HiviewFileInfo>& fileInfos)
{
    HIVIEW_LOGI("type = %{public}s.", logType.c_str());
    auto remote = Remote();
    if (remote == nullptr) {
        HIVIEW_LOGE("remote service is null.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    MessageParcel data;
    if (!data.WriteInterfaceToken(HiviewServiceAbilityProxy::GetDescriptor())
        || !data.WriteString(logType)) {
        HIVIEW_LOGE("write data failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    MessageParcel reply;
    MessageOption option;
    int32_t res = remote->SendRequest(
        static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_LIST), data, reply, option);
    if (res != ERR_OK) {
        HIVIEW_LOGE("send request failed, error is %{public}d.", res);
        return res;
    }
    std::vector<uint32_t> allSize;
    if (!reply.ReadUInt32Vector(&allSize)) {
        HIVIEW_LOGE("read size error.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    sptr<Ashmem> ashmem = reply.ReadAshmem();
    if (ashmem == nullptr) {
        HIVIEW_LOGE("read ashmem failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    if (!ashmem->MapReadAndWriteAshmem()) {
        HIVIEW_LOGE("map ash failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    if (!AshMemoryUtils::ReadBulkData<HiviewFileInfo>(ashmem, allSize, fileInfos)) {
        HIVIEW_LOGE("ReadBulkData failed");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    HIVIEW_LOGW("file list num:%{public}zu", fileInfos.size());
    return ERR_OK;
}

int32_t HiviewServiceAbilityProxy::Copy(const std::string& logType, const std::string& logName,
    const std::string& dest)
{
    return CopyOrMoveFile(logType, logName, dest, false);
}

int32_t HiviewServiceAbilityProxy::Move(const std::string& logType, const std::string& logName,
    const std::string& dest)
{
    return CopyOrMoveFile(logType, logName, dest, true);
}

int32_t HiviewServiceAbilityProxy::CopyOrMoveFile(
    const std::string& logType, const std::string& logName, const std::string& dest, bool isMove)
{
    auto remote = Remote();
    if (remote == nullptr) {
        HIVIEW_LOGE("remote service is null.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    MessageParcel data;
    if (!data.WriteInterfaceToken(HiviewServiceAbilityProxy::GetDescriptor())
        || !data.WriteString(logType) || !data.WriteString(logName) || !data.WriteString(dest)) {
        HIVIEW_LOGE("write data failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    MessageParcel reply;
    MessageOption option;
    int32_t res = remote->SendRequest(
        isMove ? static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_MOVE) :
        static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_COPY), data, reply, option);
    if (res != ERR_OK) {
        HIVIEW_LOGE("send request failed, error is %{public}d.", res);
        return res;
    }
    int32_t result = 0;
    if (!reply.ReadInt32(result)) {
        HIVIEW_LOGE("parcel read result failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    return result;
}

int32_t HiviewServiceAbilityProxy::Remove(const std::string& logType, const std::string& logName)
{
    auto remote = Remote();
    if (remote == nullptr) {
        HIVIEW_LOGE("remote service is null.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    MessageParcel data;
    if (!data.WriteInterfaceToken(HiviewServiceAbilityProxy::GetDescriptor())
        || !data.WriteString(logType) || !data.WriteString(logName)) {
        HIVIEW_LOGE("write data failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    MessageParcel reply;
    MessageOption option;
    int32_t res = remote->SendRequest(
        static_cast<uint32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_REMOVE), data, reply, option);
    if (res != ERR_OK) {
        HIVIEW_LOGE("send request failed, error is %{public}d.", res);
        return res;
    }
    int32_t result = 0;
    if (!reply.ReadInt32(result)) {
        HIVIEW_LOGE("parcel read result failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    return result;
}

CollectResultParcelable<int32_t> HiviewServiceAbilityProxy::OpenSnapshotTrace(
    const std::vector<std::string>& tagGroups)
{
    auto parcelHandler = [&tagGroups] (MessageParcel& data) {
        return data.WriteStringVector(tagGroups);
    };
    return SendTraceRequest<int32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_OPEN_SNAPSHOT_TRACE,
        parcelHandler);
}

CollectResultParcelable<std::vector<std::string>> HiviewServiceAbilityProxy::DumpSnapshotTrace(int32_t caller)
{
    auto parcelHandler = [caller] (MessageParcel& data) {
        return data.WriteInt32(caller);
    };
    return SendTraceRequest<std::vector<std::string>>(
        HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_DUMP_SNAPSHOT_TRACE, parcelHandler);
}

CollectResultParcelable<int32_t> HiviewServiceAbilityProxy::OpenRecordingTrace(const std::string& tags)
{
    auto parcelHandler = [&tags] (MessageParcel& data) {
        return data.WriteString(tags);
    };
    return SendTraceRequest<int32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_OPEN_RECORDING_TRACE,
        parcelHandler);
}

CollectResultParcelable<int32_t> HiviewServiceAbilityProxy::RecordingTraceOn()
{
    auto parcelHandler = [] (MessageParcel& data) {
        return true;
    };
    return SendTraceRequest<int32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECORDING_TRACE_ON,
        parcelHandler);
}

CollectResultParcelable<std::vector<std::string>> HiviewServiceAbilityProxy::RecordingTraceOff()
{
    auto parcelHandler = [] (MessageParcel& data) {
        return true;
    };
    return SendTraceRequest<std::vector<std::string>>(
        HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECORDING_TRACE_OFF, parcelHandler);
}

CollectResultParcelable<int32_t> HiviewServiceAbilityProxy::CloseTrace()
{
    auto parcelHandler = [] (MessageParcel& data) {
        return true;
    };
    return SendTraceRequest<int32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_CLOSE_TRACE,
        parcelHandler);
}

CollectResultParcelable<int32_t> HiviewServiceAbilityProxy::RecoverTrace()
{
    auto parcelHandler = [] (MessageParcel& data) {
        return true;
    };
    return SendTraceRequest<int32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_RECOVER_TRACE,
        parcelHandler);
}

static bool WriteAppCallerBase(MessageParcel& data, UCollectClient::AppCaller &appCaller, std::string &errField)
{
    if (!data.WriteInt32(appCaller.actionId)) {
        errField = "actionId";
        return false;
    }

    if (!data.WriteString(appCaller.bundleName)) {
        errField = "bundleName";
        return false;
    }

    if (!data.WriteString(appCaller.bundleVersion)) {
        errField = "bundleVersion";
        return false;
    }

    if (!data.WriteString(appCaller.threadName)) {
        errField = "threadName";
        return false;
    }

    if (!data.WriteInt32(appCaller.foreground)) {
        errField = "foreground";
        return false;
    }
    return true;
}

static bool WriteAppCallerExternal(MessageParcel& data, UCollectClient::AppCaller &appCaller, std::string &errField)
{
    if (!data.WriteInt32(appCaller.uid)) {
        errField = "uid";
        return false;
    }

    if (!data.WriteInt32(appCaller.pid)) {
        errField = "pid";
        return false;
    }

    if (!data.WriteInt64(appCaller.happenTime)) {
        errField = "happenTime";
        return false;
    }

    if (!data.WriteInt64(appCaller.beginTime)) {
        errField = "beginTime";
        return false;
    }

    if (!data.WriteInt64(appCaller.endTime)) {
        errField = "endTime";
        return false;
    }

    if (!data.WriteBool(appCaller.isBusinessJank)) {
        errField = "isBusinessJank";
        return false;
    }
    return true;
}

CollectResultParcelable<int32_t> HiviewServiceAbilityProxy::CaptureDurationTrace(UCollectClient::AppCaller &appCaller)
{
    auto parcelHandler = [&appCaller] (MessageParcel& data) {
        std::string errField;
        do {
            if (!WriteAppCallerBase(data, appCaller, errField)) {
                break;
            }
            if (!WriteAppCallerExternal(data, appCaller, errField)) {
                break;
            }
        } while (0);

        if (!errField.empty()) {
            HIVIEW_LOGE("write field %{public}s failed", errField.c_str());
            return false;
        }
        return true;
    };
    return SendTraceRequest<int32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_GET_APP_TRACE,
        parcelHandler);
}

CollectResultParcelable<double> HiviewServiceAbilityProxy::GetSysCpuUsage()
{
    auto parcelHandler = [] (MessageParcel& data) {
        return true;
    };
    return SendTraceRequest<double>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_GET_SYSTEM_CPU_USAGE,
        parcelHandler);
}

CollectResultParcelable<int32_t> HiviewServiceAbilityProxy::SetAppResourceLimit(
    UCollectClient::MemoryCaller& memoryCaller)
{
    auto parcelHandler = [&memoryCaller] (MessageParcel& data) {
        if (!data.WriteInt32(memoryCaller.pid)) {
            HIVIEW_LOGE("SetAppResourceLimit failed. write pid failed");
            return false;
        }

        if (!data.WriteString(memoryCaller.resourceType)) {
            HIVIEW_LOGE("SetAppResourceLimit failed. write type failed");
            return false;
        }

        if (!data.WriteInt32(memoryCaller.limitValue)) {
            HIVIEW_LOGE("SetAppResourceLimit failed. write value failed");
            return false;
        }

        if (!data.WriteBool(memoryCaller.enabledDebugLog)) {
            HIVIEW_LOGE("SetAppResourceLimit failed. write enabledDebugLog failed");
            return false;
        }
        return true;
    };
    return SendTraceRequest<int32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_SET_APPRESOURCE_LIMIT,
        parcelHandler);
}

CollectResultParcelable<int32_t> HiviewServiceAbilityProxy::GetGraphicUsage()
{
    auto parcelHandler = [] (MessageParcel& data) {
        return true;
    };
    return SendTraceRequest<int32_t>(HiviewServiceInterfaceCode::HIVIEW_SERVICE_ID_GET_GRAPHIC_USAGE, parcelHandler);
}
} // namespace HiviewDFX
} // namespace OHOS
