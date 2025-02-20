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

#include "app_caller_parcelable.h"

namespace OHOS::HiviewDFX {
bool AppCallerParcelable::Marshalling(Parcel& outParcel) const
{
    if (!outParcel.WriteInt32(appCaller_.actionId)) {
        return false;
    }
    if (!outParcel.WriteString(appCaller_.bundleName)) {
        return false;
    }
    if (!outParcel.WriteString(appCaller_.bundleVersion)) {
        return false;
    }
    if (!outParcel.WriteString(appCaller_.threadName)) {
        return false;
    }
    if (!outParcel.WriteInt32(appCaller_.foreground)) {
        return false;
    }
    if (!outParcel.WriteInt32(appCaller_.uid)) {
        return false;
    }
    if (!outParcel.WriteInt32(appCaller_.pid)) {
        return false;
    }
    if (!outParcel.WriteInt64(appCaller_.happenTime)) {
        return false;
    }
    if (!outParcel.WriteInt64(appCaller_.beginTime)) {
        return false;
    }
    if (!outParcel.WriteInt64(appCaller_.endTime)) {
        return false;
    }
    if (!outParcel.WriteBool(appCaller_.isBusinessJank)) {
        return false;
    }
    return true;
}

AppCallerParcelable* AppCallerParcelable::Unmarshalling(Parcel& inParcel)
{
    UCollectClient::AppCaller appCaller;
    if (!inParcel.ReadInt32(appCaller.actionId)) {
        return nullptr;
    }
    if (!inParcel.ReadString(appCaller.bundleName)) {
        return nullptr;
    }
    if (!inParcel.ReadString(appCaller.bundleVersion)) {
        return nullptr;
    }
    if (!inParcel.ReadString(appCaller.threadName)) {
        return nullptr;
    }
    if (!inParcel.ReadInt32(appCaller.foreground)) {
        return nullptr;
    }
    if (!inParcel.ReadInt32(appCaller.uid)) {
        return nullptr;
    }
    if (!inParcel.ReadInt32(appCaller.pid)) {
        return nullptr;
    }
    if (!inParcel.ReadInt64(appCaller.happenTime)) {
        return nullptr;
    }
    if (!inParcel.ReadInt64(appCaller.beginTime)) {
        return nullptr;
    }
    if (!inParcel.ReadInt64(appCaller.endTime)) {
        return nullptr;
    }
    if (!inParcel.ReadBool(appCaller.isBusinessJank)) {
        return nullptr;
    }
    return new(std::nothrow) AppCallerParcelable(appCaller);
}

UCollectClient::AppCaller AppCallerParcelable::GetAppCaller() const
{
    return appCaller_;
}
} // namespace OHOS::HiviewDFX