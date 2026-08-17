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

#include <cinttypes>

#include "hilog/log.h"
#include "hiretrieval_common_util.h"
#include "hiretrieval_mgr.h"
#include "hiretrieval_napi_util.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D10

#undef LOG_TAG
#define LOG_TAG "HIRETRIEVAL_NAPI"

using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::HiRetrieval;

namespace {
constexpr size_t PARAM_MAX_CNT = 1;
constexpr int PARTICIPATE_PARAM_INDEX = 0;
}

static napi_value Init(napi_env env, napi_callback_info info)
{
    int ret = HiRetrievalMgr::GetInstance().Init();
    HiRetrievalNapiUtil::CheckRetAndThrowError(env, ret);
    return nullptr;napi_value val = nullptr;
    HiRetrievalNapiUtil::CreateJsUndefinedValue(env, val);
    return val;
}

static napi_value Participate(napi_env env, napi_callback_info info)
{
    size_t paramCnt = PARAM_MAX_CNT;
    napi_value params[PARAM_MAX_CNT] = {0};
    napi_value thisArg = nullptr;
    void* data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &paramCnt, params, &thisArg, &data));
    napi_value val = nullptr;
    napi_get_undefined(env, &val);
    if (paramCnt < PARAM_MAX_CNT || paramCnt <= PARTICIPATE_PARAM_INDEX) {
        HILOG_ERROR(LOG_CORE, "param is invalid");
        HiRetrievalNapiUtil::ThrowParamMandatoryError(env, "config");
        return val;
    }

    HiRetrievalMgr::Config cfg;
    HiRetrievalNapiUtil::ParseJsHiRetrievalConfig(env, params[PARTICIPATE_PARAM_INDEX], cfg);
    int ret = HiRetrievalMgr::GetInstance().Participate(cfg);
    HiRetrievalNapiUtil::CheckRetAndThrowError(env, ret);
    return val;
}

static napi_value Quit(napi_env env, napi_callback_info info)
{
    int ret = HiRetrievalMgr::GetInstance().Quit();
    HiRetrievalNapiUtil::CheckRetAndThrowError(env, ret);
    napi_value val = nullptr;
    HiRetrievalNapiUtil::CreateJsUndefinedValue(env, val);
    return val;
}

static napi_value IsParticipant(napi_env env, napi_callback_info info)
{
    bool isParticipant = HiRetrievalMgr::GetInstance().IsParticipant();
    napi_value val = nullptr;
    HiRetrievalNapiUtil::CreateJsBoolValue(env, isParticipant, val);
    return val;
}

static napi_value GetLastParticipationTs(napi_env env, napi_callback_info info)
{
    auto lastParticipationTs = HiRetrievalMgr::GetInstance().GetLastParticipationTs();
    napi_value val = nullptr;
    HiRetrievalNapiUtil::CreateJsLongLongValue(env, lastParticipationTs, val);
    return val;
}

static napi_value Run(napi_env env, napi_callback_info info)
{
    int32_t ret = HiRetrievalMgr::GetInstance().Run();
    HiRetrievalNapiUtil::CheckRetAndThrowError(env, ret);
    napi_value val = nullptr;
    HiRetrievalNapiUtil::CreateJsUndefinedValue(env, val);
    return val;
}

static napi_value GetCurrentConfig(napi_env env, napi_callback_info info)
{
    auto curConfig = HiRetrievalMgr::GetInstance().GetCurrentConfig();
    napi_value val = nullptr;
    HiRetrievalNapiUtil::CreateJsHiRetrievalConfig(env, curConfig, val);
    return val;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION(ApiDef::INIT, Init),
        DECLARE_NAPI_FUNCTION(ApiDef::PARTICIPATE, Participate),
        DECLARE_NAPI_FUNCTION(ApiDef::QUIT, Quit),
        DECLARE_NAPI_FUNCTION(ApiDef::IS_PARTICIPANT, IsParticipant),
        DECLARE_NAPI_FUNCTION(ApiDef::GET_LAST_PARTICIPATION_TS, GetLastParticipationTs),
        DECLARE_NAPI_FUNCTION(ApiDef::RUN, Run),
        DECLARE_NAPI_FUNCTION(ApiDef::GET_CUR_CFG, GetCurrentConfig),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(napi_property_descriptor), desc));

    return exports;
}
EXTERN_C_END

static napi_module _module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "hiRetrieval",
    .nm_priv = ((void*)0),
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&_module);
}
