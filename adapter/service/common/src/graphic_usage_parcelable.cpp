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

#include "graphic_usage_parcelable.h"

namespace OHOS::HiviewDFX {
bool GraphicUsageParcelable::Marshalling(Parcel& outParcel) const
{
    if (!outParcel.WriteInt32(graphicUsage_.gl)) {
        return false;
    }
    if (!outParcel.WriteInt32(graphicUsage_.graph)) {
        return false;
    }
    return true;
}

GraphicUsageParcelable* GraphicUsageParcelable::Unmarshalling(Parcel& inParcel)
{
    UCollectClient::GraphicUsage graphicUsage;
    if (!inParcel.ReadInt32(graphicUsage.gl)) {
        return nullptr;
    }
    if (!inParcel.ReadInt32(graphicUsage.graph)) {
        return nullptr;
    }
    return new(std::nothrow) GraphicUsageParcelable(graphicUsage);
}

UCollectClient::GraphicUsage GraphicUsageParcelable::GetGraphicUsage() const
{
    return graphicUsage_;
}

void GraphicUsageParcelable::UpdateGraphicUsage(const UCollectClient::GraphicUsage& graphicUsage)
{
    graphicUsage_ = graphicUsage;
}
} // namespace OHOS::HiviewDFX