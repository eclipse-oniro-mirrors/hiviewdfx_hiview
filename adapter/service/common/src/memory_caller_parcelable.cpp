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

#include "memory_caller_parcelable.h"

namespace OHOS::HiviewDFX {
bool MemoryCallerParcelable::Marshalling(Parcel& outParcel) const
{
    if (!outParcel.WriteInt32(memoryCaller_.pid)) {
        return false;
    }
    if (!outParcel.WriteString(memoryCaller_.resourceType)) {
        return false;
    }
    if (!outParcel.WriteInt32(memoryCaller_.limitValue)) {
        return false;
    }
    if (!outParcel.WriteBool(memoryCaller_.enabledDebugLog)) {
        return false;
    }
    return true;
}

MemoryCallerParcelable* MemoryCallerParcelable::Unmarshalling(Parcel& inParcel)
{
    UCollectClient::MemoryCaller memoryCaller;
    if (!inParcel.ReadInt32(memoryCaller.pid)) {
        return nullptr;
    }
    if (!inParcel.ReadString(memoryCaller.resourceType)) {
        return nullptr;
    }
    if (!inParcel.ReadInt32(memoryCaller.limitValue)) {
        return nullptr;
    }
    if (!inParcel.ReadBool(memoryCaller.enabledDebugLog)) {
        return nullptr;
    }
    return new(std::nothrow) MemoryCallerParcelable(memoryCaller);
}

UCollectClient::MemoryCaller MemoryCallerParcelable::GetMemoryCaller() const
{
    return memoryCaller_;
}
} // namespace OHOS::HiviewDFX