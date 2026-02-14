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

#ifndef HIVIEW_REQUEST_TRACE_CONFIG_PARCELABLE_H
#define HIVIEW_REQUEST_TRACE_CONFIG_PARCELABLE_H

#include "client/trace_collector_client.h"
#include "parcel.h"

namespace OHOS::HiviewDFX {
class TraceConfigParcelable : public Parcelable {
public:
    TraceConfigParcelable(const UCollectClient::TraceConfig& traceConfig) : traceConfig_(traceConfig) {}
    UCollectClient::TraceConfig GetTraceConfig() const;
    bool Marshalling(Parcel& outParcel) const override;
    static TraceConfigParcelable* Unmarshalling(Parcel& inParcel);

private:
    UCollectClient::TraceConfig traceConfig_;
};
}

#endif //REQUEST_TRACE_CONFIG_PARCELABLE_H
