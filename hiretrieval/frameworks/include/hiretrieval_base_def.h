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

#ifndef HIRETRIEVAL_BASE_DEF_INCLUDE_H
#define HIRETRIEVAL_BASE_DEF_INCLUDE_H

#include <cstdint>

extern "C" struct HiRetrievalConfig {
    const char* userType;
    const char* deviceType;
    const char* deviceModel;
};

namespace OHOS::HiviewDFX::HiRetrieval {
namespace NativeErrorCode {
static constexpr int32_t SUCC = 0;
static constexpr int32_t NOT_INIT = -1;
static constexpr int32_t DLL_FAILED = -1000;
}

namespace CommonDef {
static constexpr char USER_TYPE_ATTR_NAME[] = "userType";
static constexpr char DEVICE_TYPE_ATTR_NAME[] = "deviceType";
static constexpr char DEVICE_MODEL_ATTR_NAME[] = "deviceModel";
static constexpr char PARTICIPATION_TS_KEY[] = "participationTs";
static constexpr char PARTICIPATION_STATUS_KEY[] = "participationStatus";
}
} // namespace OHOS::HiviewDFX::HiRetrieval

#endif // HIRETRIEVAL_BASE_DEF_INCLUDE_H