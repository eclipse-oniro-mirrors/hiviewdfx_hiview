/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "faultlog_info_ohos.h"

#include "hiview_logger.h"
#include "parcel.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "FaultLogInfoOhos");
bool FaultLogInfoOhos::Marshalling(Parcel& parcel) const
{
    if (!parcel.WriteInt64(time) || !parcel.WriteInt32(uid) ||
        !parcel.WriteInt32(pid) || !parcel.WriteInt32(faultLogType)) {
        HIVIEW_LOGE("Parcel failed to write int number.");
        return false;
    }
    if (!parcel.WriteString(module)) {
        HIVIEW_LOGE("Parcel failed to write module name(%{public}s).", module.c_str());
        return false;
    }
    if (!parcel.WriteString(reason)) {
        HIVIEW_LOGE("Parcel failed to write reason(%{public}s).", reason.c_str());
        return false;
    }
    if (!parcel.WriteString(summary)) {
        HIVIEW_LOGE("Parcel failed to write summary(%{public}s).", summary.c_str());
        return false;
    }
    if (!parcel.WriteString(logPath)) {
        HIVIEW_LOGE("Parcel failed to write log path(%{public}s).", logPath.c_str());
        return false;
    }
    if (!parcel.WriteString(registers)) {
        HIVIEW_LOGE("Parcel failed to write registers(%{public}s).", registers.c_str());
        return false;
    }
    uint32_t size = sectionMaps.size();
    if (!parcel.WriteUint32(size)) {
        return false;
    }
    for (const auto& [key, value] : sectionMaps) {
        if (!parcel.WriteString(key)) {
            HIVIEW_LOGE("Parcel failed to write key of sectionMaps(%{public}s).", key.c_str());
            return false;
        }
        if (!parcel.WriteString(value)) {
            HIVIEW_LOGE("Parcel failed to write value of sectionMaps(%{public}s).", value.c_str());
            return false;
        }
    }
    return true;
}

bool FaultLogInfoOhos::ReadString(Parcel& parcel, std::string strItem, std::string& strValue)
{
    if (!parcel.ReadString(strValue)) {
        HIVIEW_LOGE("Parcel failed to read %{public}s(%{public}s).", strItem.c_str(), strValue.c_str());
        return false;
    }
    return true;
}

sptr<FaultLogInfoOhos> FaultLogInfoOhos::Unmarshalling(Parcel& parcel)
{
    sptr<FaultLogInfoOhos> FaultLogInfo = new FaultLogInfoOhos();

    if (!parcel.ReadInt64(FaultLogInfo->time) || !parcel.ReadInt32(FaultLogInfo->uid) ||
        !parcel.ReadInt32(FaultLogInfo->pid) || !parcel.ReadInt32(FaultLogInfo->faultLogType)) {
        HIVIEW_LOGE("Parcel failed to read int number.");
        return nullptr;
    }
    if (!FaultLogInfoOhos::ReadString(parcel, "module", FaultLogInfo->module) ||
        !FaultLogInfoOhos::ReadString(parcel, "reason", FaultLogInfo->reason) ||
        !FaultLogInfoOhos::ReadString(parcel, "summary", FaultLogInfo->summary) ||
        !FaultLogInfoOhos::ReadString(parcel, "logPath", FaultLogInfo->logPath) ||
        !FaultLogInfoOhos::ReadString(parcel, "registers", FaultLogInfo->registers)) {
        return nullptr;
    }

    const uint32_t maxSize = 128;
    uint32_t size = 0;
    if (!parcel.ReadUint32(size) || (size > maxSize)) {
        return nullptr;
    }
    for (uint32_t i = 0; i < size; i++) {
        std::string key;
        std::string value;
        if (!parcel.ReadString(key)) {
            HIVIEW_LOGE("Parcel failed to read key of sectionMaps(%{public}s).", key.c_str());
            return nullptr;
        }
        if (!parcel.ReadString(value)) {
            HIVIEW_LOGE("Parcel failed to read value of sectionMaps(%{public}s).", value.c_str());
            return nullptr;
        }
        FaultLogInfo->sectionMaps[key] = value;
    }

    return FaultLogInfo;
}
}  // namespace hiview
}  // namespace OHOS
