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

#include "event_export_util.h"

#include "parameter.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
std::string GenerateDeviceId()
{
    constexpr int32_t deviceIdLength = 65;
    char id[deviceIdLength] = {0};
    if (GetDevUdid(id, deviceIdLength) == 0) {
        return std::string(id);
    }
    return "";
}
}

std::string EventExportUtil::GetDeviceId()
{
    static std::string deviceId = Parameter::GetUserType() == Parameter::USER_TYPE_OVERSEA_COMMERCIAL ?
        Parameter::GetString("persist.hiviewdfx.priv.packid", "") : GenerateDeviceId();
    return deviceId;
}
} // namespace HiviewDFX
} // namespace OHOS