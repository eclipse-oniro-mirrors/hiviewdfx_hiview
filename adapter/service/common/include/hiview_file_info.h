/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_FILE_INFO_H
#define HIVIEW_FILE_INFO_H

#include "parcel.h"

namespace OHOS {
namespace HiviewDFX {
struct HiviewFileInfo : public Parcelable {
    HiviewFileInfo(std::string name, time_t mtime, uint64_t size)
        : name(name), mtime(mtime), size(size) {};

    bool Marshalling(Parcel& outParcel) const override;
    static HiviewFileInfo* Unmarshalling(Parcel& inParcel);

    std::string name;
    time_t mtime;
    uint64_t size;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif