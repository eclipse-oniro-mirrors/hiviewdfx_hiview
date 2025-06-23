/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "hiview_napi_adapter.h"
#include "hiview_napi_util.h"
#include "hiview_service_agent.h"
#include "hiview_logger.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D10, "NAPI_HIVIEW_JS");
namespace {
constexpr size_t LOG_TYPE_INDEX = 0;
constexpr size_t LOG_NAME_INDEX = 1;
constexpr size_t DEST_DIR_INDEX = 2;
}

static napi_value List(napi_env env, napi_callback_info info)
{
    if (!HiviewNapiUtil::IsSystemAppCall()) {
        HiviewNapiUtil::ThrowSystemAppPermissionError(env);
        return nullptr;
    }
    constexpr size_t listParamNum = 1;
    size_t paramNum = listParamNum;
    napi_value params[listParamNum] = { 0 };
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    NAPI_CALL(env, napi_get_cb_info(env, info, &paramNum, params, nullptr, nullptr));
    if (paramNum < listParamNum) {
        HIVIEW_LOGE("num of params is invalid: %{public}zu", paramNum);
        return result;
    }
    std::string logType;
    if (!HiviewNapiUtil::ParseStringValue(env, "logType", params[LOG_TYPE_INDEX], logType)) {
        return result;
    }
    std::vector<HiviewFileInfo> fileInfos;
    int32_t retCode = HiviewServiceAgent::GetInstance().List(logType, fileInfos);
    HIVIEW_LOGI("retCode: %{public}u.", retCode);
    if (retCode == 0) {
        return HiviewNapiUtil::GenerateFileInfoResult(env, fileInfos);
    } else {
        HiviewNapiUtil::ThrowErrorByCode(env, retCode);
        return result;
    }
}

static napi_value CopyOrMoveFile(napi_env env, napi_callback_info info, bool isMove)
{
    if (isMove) {
        HIVIEW_LOGI("call move");
    } else {
        HIVIEW_LOGI("call copy");
    }
    constexpr size_t maxParamNum = 4;
    constexpr size_t paramNumWithoutCallback = 3;
    size_t paramNum = maxParamNum;
    napi_value params[maxParamNum] = { 0 };
    NAPI_CALL(env, napi_get_cb_info(env, info, &paramNum, params, nullptr, nullptr));
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    if (paramNum < paramNumWithoutCallback) {
        HIVIEW_LOGE("num of params is invalid %{public}zu.", paramNum);
        return result;
    }
    std::string logType;
    std::string logName;
    std::string destDir;
    if (!HiviewNapiUtil::ParseStringValue(env, "logType", params[LOG_TYPE_INDEX], logType)
        || !HiviewNapiUtil::ParseStringValue(env, "logName", params[LOG_NAME_INDEX], logName)
        || !HiviewNapiUtil::ParseStringValue(env, "dest", params[DEST_DIR_INDEX], destDir)) {
        return result;
    }
    HIVIEW_LOGI("type:%{public}s, name:%{public}s, dir: %{public}s",
        logType.c_str(), StringUtil::HideSnInfo(logName).c_str(), destDir.c_str());
    if (!HiviewNapiUtil::CheckDirPath(destDir)) {
        HIVIEW_LOGE("dest param is invalid: %{public}s", destDir.c_str());
        HiviewNapiUtil::ThrowParamContentError(env, "dest");
        return result;
    }
    HiviewFileParams* hiviewFileParams = new(std::nothrow) HiviewFileParams(logType, logName, destDir);
    if (hiviewFileParams == nullptr) {
        HIVIEW_LOGE("failed to allocate memory");
        return result;
    } else if (paramNum == maxParamNum) {
        if (!HiviewNapiUtil::IsMatchType(env, params[paramNumWithoutCallback], napi_function)) {
            HIVIEW_LOGE("no valid function param");
            HiviewNapiUtil::ThrowParamTypeError(env, "callback", "function");
            delete hiviewFileParams;
            return result;
        }
        napi_create_reference(env, params[paramNumWithoutCallback], 1, &hiviewFileParams->callback);
    } else {
        napi_create_promise(env, &hiviewFileParams->deferred, &result);
    }
    isMove ? HiviewNapiAdapter::Move(env, hiviewFileParams) : HiviewNapiAdapter::Copy(env, hiviewFileParams);
    return result;
}

static napi_value Copy(napi_env env, napi_callback_info info)
{
    if (!HiviewNapiUtil::IsSystemAppCall()) {
        HiviewNapiUtil::ThrowSystemAppPermissionError(env);
        return nullptr;
    }
    return CopyOrMoveFile(env, info, false);
}

static napi_value Move(napi_env env, napi_callback_info info)
{
    if (!HiviewNapiUtil::IsSystemAppCall()) {
        HiviewNapiUtil::ThrowSystemAppPermissionError(env);
        return nullptr;
    }
    return CopyOrMoveFile(env, info, true);
}

static napi_value Remove(napi_env env, napi_callback_info info)
{
    if (!HiviewNapiUtil::IsSystemAppCall()) {
        HiviewNapiUtil::ThrowSystemAppPermissionError(env);
        return nullptr;
    }
    constexpr size_t removeParamNum = 2;
    size_t paramNum = removeParamNum;
    napi_value params[removeParamNum] = { 0 };
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    NAPI_CALL(env, napi_get_cb_info(env, info, &paramNum, params, nullptr, nullptr));
    if (paramNum < removeParamNum) {
        HIVIEW_LOGE("num of params is invalid: %{public}zu", paramNum);
        return result;
    }
    std::string logType;
    std::string logName;
    if (!HiviewNapiUtil::ParseStringValue(env, "logType", params[LOG_TYPE_INDEX], logType)
        || !HiviewNapiUtil::ParseStringValue(env, "logName", params[LOG_NAME_INDEX], logName)) {
        return result;
    }
    HIVIEW_LOGI("type: %{public}s, name: %{public}s", logType.c_str(), StringUtil::HideSnInfo(logName).c_str());
    int32_t retCode = HiviewServiceAgent::GetInstance().Remove(logType, logName);
    if (retCode != 0) {
        HIVIEW_LOGI("retCode: %{public}u.", retCode);
        HiviewNapiUtil::ThrowErrorByCode(env, retCode);
    }
    return result;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("list", List),
        DECLARE_NAPI_FUNCTION("copy", Copy),
        DECLARE_NAPI_FUNCTION("move", Move),
        DECLARE_NAPI_FUNCTION("remove", Remove)
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
    .nm_modname = "logLibrary",
    .nm_priv = ((void *) 0),
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&_module);
}
} // namespace HiviewDFX
} // namespace OHOS