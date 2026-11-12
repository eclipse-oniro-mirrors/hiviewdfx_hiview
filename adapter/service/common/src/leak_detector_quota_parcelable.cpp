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

#include "leak_detector_quota_parcelable.h"

namespace OHOS::HiviewDFX {
bool LeakDetectorQuotaParcelable::Marshalling(Parcel& outParcel) const
{
    if (!outParcel.WriteString(info_.processName)) {
        return false;
    }
    if (!outParcel.WriteString(info_.resType)) {
        return false;
    }
    if (!outParcel.WriteInt32(info_.processType)) {
        return false;
    }
    if (!outParcel.WriteInt64(info_.resource)) {
        return false;
    }
    if (!outParcel.WriteInt64(info_.duration)) {
        return false;
    }
    if (!outParcel.WriteString(info_.action)) {
        return false;
    }
    if (!outParcel.WriteString(info_.preCleanProcess)) {
        return false;
    }
    if (!outParcel.WriteUInt32Vector(info_.condition)) {
        return false;
    }
    return true;
}

LeakDetectorQuotaParcelable* LeakDetectorQuotaParcelable::Unmarshalling(Parcel& inParcel)
{
    UCollectClient::LeakDetectorQuotaInfo info;
    if (!inParcel.ReadString(info.processName)) {
        return nullptr;
    }
    if (!inParcel.ReadString(info.resType)) {
        return nullptr;
    }
    if (!inParcel.ReadInt32(info.processType)) {
        return nullptr;
    }
    if (!inParcel.ReadInt64(info.resource)) {
        return nullptr;
    }
    if (!inParcel.ReadInt64(info.duration)) {
        return nullptr;
    }
    if (!inParcel.ReadString(info.action)) {
        return nullptr;
    }
    if (!inParcel.ReadString(info.preCleanProcess)) {
        return nullptr;
    }
    if (!inParcel.ReadUInt32Vector(&info.condition)) {
        return nullptr;
    }
    return new(std::nothrow) LeakDetectorQuotaParcelable(info);
}
} // namespace OHOS::HiviewDFX
