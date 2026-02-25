/*
* Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "trace_config_parcelable.h"

namespace OHOS::HiviewDFX {
bool TraceConfigParcelable::Marshalling(Parcel& outParcel) const
{
    if (!outParcel.WriteString(traceConfig_.prefix)) {
        return false;
    }
    if (!outParcel.WriteUint32(traceConfig_.bufferSize)) {
        return false;
    }
    if (!outParcel.WriteUint32(traceConfig_.duration)) {
        return false;
    }
    return true;
}

TraceConfigParcelable* TraceConfigParcelable::Unmarshalling(Parcel& inParcel)
{
    UCollectClient::TraceConfig traceConfig;
    if (!inParcel.ReadString(traceConfig.prefix)) {
        return nullptr;
    }
    if (!inParcel.ReadUint32(traceConfig.bufferSize)) {
        return nullptr;
    }
    if (!inParcel.ReadUint32(traceConfig.duration)) {
        return nullptr;
    }
    return new(std::nothrow) TraceConfigParcelable(traceConfig);
}

UCollectClient::TraceConfig TraceConfigParcelable::GetTraceConfig() const
{
    return traceConfig_;
}
}