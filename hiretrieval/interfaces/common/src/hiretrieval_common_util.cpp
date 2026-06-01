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

#include "hiretrieval_common_util.h"

#include <unordered_map>

#include "hiretrieval_base_def.h"

namespace OHOS::HiviewDFX::HiRetrieval {
namespace {
constexpr char NOT_INIT_DES[] = "Initialization error. Possibly caused by invoking this function "
    "before invoking init function.";
constexpr char MULTI_INSTANCE_DES[] = "Multi-instance applications not supported error. "
    "Possibly caused by invoking this function in a multi-instance application.";
}

std::pair<int32_t, std::string> CommonUtil::GetErrorDetailByRet(const int32_t retCode)
{
    static std::unordered_map<int32_t, std::pair<int32_t, std::string>> errMap = {
        {NativeErrorCode::NOT_INIT, {CommonErrorCode::ERR_NOT_INIT, NOT_INIT_DES}},
        {NativeErrorCode::MULTI_INSTANCE, {CommonErrorCode::ERR_MULTI_INSTANCE, MULTI_INSTANCE_DES}}
    };
    auto ret = errMap.find(retCode);
    if (ret == errMap.end()) {
        return std::make_pair(NativeErrorCode::SUCC, "");
    } else {
        return ret->second;
    }
}
} // namespace OHOS::HiviewDFX::HiRetrieval