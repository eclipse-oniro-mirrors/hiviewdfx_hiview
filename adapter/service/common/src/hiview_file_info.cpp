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

#include "hiview_file_info.h"

namespace OHOS {
namespace HiviewDFX {
bool HiviewFileInfo::Marshalling(Parcel& outParcel) const
{
    if (!outParcel.WriteString(name)) {
        return false;
    }
    if (!outParcel.WriteInt64(mtime)) {
        return false;
    }
    if (!outParcel.WriteUint64(size)) {
        return false;
    }
    return true;
}

HiviewFileInfo* HiviewFileInfo::Unmarshalling(Parcel& inParcel)
{
    std::string name;
    if (!inParcel.ReadString(name)) {
        return nullptr;
    }
    time_t mtime = 0;
    if (!inParcel.ReadInt64(mtime)) {
        return nullptr;
    }
    uint64_t size = 0;
    if (!inParcel.ReadUint64(size)) {
        return nullptr;
    }
    return new HiviewFileInfo(name, mtime, size);
}
} // namespace HiviewDFX
} // namespace OHOS