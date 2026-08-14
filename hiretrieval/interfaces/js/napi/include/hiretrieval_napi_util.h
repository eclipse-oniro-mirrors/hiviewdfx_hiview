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

#ifndef HIRETIREVAL_NAPI_UTIL_INCLUDE_H
#define HIRETIREVAL_NAPI_UTIL_INCLUDE_H

#include <stdint.h>
#include <string>

#include "hiretrieval_base_def.h"
#include "hiretrieval_mgr.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS::HiviewDFX {
class HiRetrievalNapiUtil {
public:
    static void ThrowParamMandatoryError(const napi_env env, const std::string paramName);
    static void CheckRetAndThrowError(const napi_env env, int32_t retCode);
    static void ParseJsHiRetrievalConfig(const napi_env env, const napi_value& jsVal, HiRetrievalMgr::Config& cfg);
    static void CreateJsBoolValue(const napi_env env, bool boolVal, napi_value& jsVal);
    static void CreateJsHiRetrievalConfig(const napi_env env, const HiRetrievalMgr::Config& cfg, napi_value& jsVal);
    static void CreateJsLongLongValue(const napi_env env, long long lldVal, napi_value& jsVal);
    static void CreateJsUndefinedValue(const napi_env env, napi_value& jsVal);
};
} // namespace OHOS::HiviewDFX

#endif // HIRETIREVAL_NAPI_UTIL_INCLUDE_H