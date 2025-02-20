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

#ifndef HIVIEW_MEMORY_CALLER_PARCELABLE_H
#define HIVIEW_MEMORY_CALLER_PARCELABLE_H

#include "client/memory_collector_client.h"
#include "parcel.h"

namespace OHOS::HiviewDFX {
class MemoryCallerParcelable : public Parcelable {
public:
    MemoryCallerParcelable(const UCollectClient::MemoryCaller& memoryCaller) : memoryCaller_(memoryCaller) {}
    UCollectClient::MemoryCaller GetMemoryCaller() const;

    bool Marshalling(Parcel& outParcel) const override;
    static MemoryCallerParcelable* Unmarshalling(Parcel &inParcel);

private:
    UCollectClient::MemoryCaller memoryCaller_;
};
} // namespace OHOS::HiviewDFX
#endif // HIVIEW_MEMORY_CALLER_PARCELABLE_H