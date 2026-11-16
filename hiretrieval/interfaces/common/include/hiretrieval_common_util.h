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

#ifndef NAPI_HIRETIREVAL_COMMON_UTIL_INCLUDE_H
#define NAPI_HIRETIREVAL_COMMON_UTIL_INCLUDE_H

#include <cstdint>
#include <string>
#include <utility>

namespace OHOS::HiviewDFX::HiRetrieval {
namespace ApiDef {
static constexpr char INIT[] = "init";
static constexpr char PARTICIPATE[] = "participate";
static constexpr char QUIT[] = "quit";
static constexpr char IS_PARTICIPANT[] = "isParticipant";
static constexpr char GET_LAST_PARTICIPATION_TS[] = "getLastParticipationTimestamp";
static constexpr char RUN[] = "run";
static constexpr char GET_CUR_CFG[] = "getCurrentConfig";
}

namespace CommonErrorCode {
static constexpr int32_t ERR_PARAM_CHECK = 401;
static constexpr int32_t ERR_NOT_INIT = 36000001;
}

class CommonUtil {
public:
    static std::pair<int32_t, std::string> GetErrorDetailByRet(const int32_t retCode);
};
} // namespace OHOS::HiviewDFX::HiRetrieval

#endif // NAPI_HIRETIREVAL_COMMON_UTIL_INCLUDE_H