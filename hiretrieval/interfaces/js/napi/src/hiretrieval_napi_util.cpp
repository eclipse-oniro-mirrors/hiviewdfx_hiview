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

#include "hiretrieval_napi_util.h"

#include <cinttypes>

#include "hilog/log.h"
#include "hiretrieval_common_util.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D10

#undef LOG_TAG
#define LOG_TAG "HIRETRIEVAL_NAPI_UTIL"

using namespace OHOS::HiviewDFX::HiRetrieval;

namespace OHOS::HiviewDFX {
namespace {
constexpr uint32_t BUF_SIZE = 129;

void ThrowError(napi_env env, const int32_t code, const std::string& msg)
{
    if (napi_throw_error(env, std::to_string(code).c_str(), msg.c_str()) != napi_ok) {
        HILOG_ERROR(LOG_CORE, "failed to throw err, code=%{public}d, msg=%{public}s.", code, msg.c_str());
    }
}

napi_valuetype GetValueType(const napi_env env, const napi_value& value)
{
    napi_valuetype valueType = napi_undefined;
    napi_status ret = napi_typeof(env, value, &valueType);
    if (ret != napi_ok) {
        HILOG_ERROR(LOG_CORE, "failed to parse the type of napi value.");
    }
    return valueType;
}

std::string ParseStrVal(const napi_env env, const napi_value& value, std::string defaultValue = "")
{
    char buf[BUF_SIZE] = {0};
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, buf, BUF_SIZE, &bufLength);
    if (status != napi_ok) {
        HILOG_WARN(LOG_CORE, "failed to parse napi value of string type.");
        return defaultValue;
    }
    std::string dest = std::string {buf};
    return dest;
}

void ThrowParamTypeError(napi_env env, const std::string paramName, std::string paramType)
{
    ThrowError(env, CommonErrorCode::ERR_PARAM_CHECK, "Parameter error. The type of " + paramName + " must be "
        + paramType + ".");
}

napi_value GetPropertyByName(const napi_env env, const napi_value& object,
    const std::string& propertyName)
{
    napi_value result = nullptr;
    napi_status status = napi_get_named_property(env, object, propertyName.c_str(), &result);
    if (status != napi_ok) {
        HILOG_WARN(LOG_CORE, "failed to parse property named %{public}s from JS object.", propertyName.c_str());
    }
    return result;
}

napi_status SetNamedProperty(const napi_env env, napi_value& object,
    const std::string& propertyName, napi_value& propertyValue)
{
    napi_status status = napi_set_named_property(env, object, propertyName.c_str(), propertyValue);
    if (status != napi_ok) {
        HILOG_ERROR(LOG_CORE, "set property %{public}s failed.", propertyName.c_str());
    }
    return status;
}

void CreateStrVal(const napi_env env, const std::string& value, napi_value& ret)
{
    napi_status status = napi_create_string_utf8(env, value.c_str(), NAPI_AUTO_LENGTH, &ret);
    if (status != napi_ok) {
        HILOG_ERROR(LOG_CORE, "failed to create napi value of string type.");
    }
}

void SetStrPropertyByName(const napi_env env, napi_value& object, const std::string& propertyName,
    const std::string& strVal)
{
    napi_value strJsVal = nullptr;
    CreateStrVal(env, strVal, strJsVal);
    SetNamedProperty(env, object, propertyName, strJsVal);
}

bool IsValTypeValid(const napi_env env, const napi_value& jsObj, const napi_valuetype typeName)
{
    napi_valuetype valueType = GetValueType(env, jsObj);
    if (valueType != typeName) {
        HILOG_WARN(LOG_CORE, "napi value type not match: valueType=%{public}d, typeName=%{public}d.",
            valueType, typeName);
        return false;
    }
    return true;
}

std::string GetStrTypeAttr(const napi_env env, const napi_value& object, const std::string& propertyName,
    const std::string& defaultVal)
{
    napi_value propertyValue = GetPropertyByName(env, object, propertyName);
    if (!IsValTypeValid(env, propertyValue, napi_valuetype::napi_string)) {
        ThrowParamTypeError(env, propertyName, "string");
        HILOG_DEBUG(LOG_CORE, "type is not napi_string.");
        return defaultVal;
    }
    std::string attrVal = ParseStrVal(env, propertyValue, defaultVal);
    if (attrVal.empty()) {
        return defaultVal;
    }
    return attrVal;
}
}

void HiRetrievalNapiUtil::CheckRetAndThrowError(const napi_env env, int32_t retCode)
{
    auto detail = CommonUtil::GetErrorDetailByRet(retCode);
    if (detail.first == NativeErrorCode::SUCC) {
        return;
    }
    ThrowError(env, detail.first, detail.second);
}

void HiRetrievalNapiUtil::ThrowParamMandatoryError(const napi_env env, const std::string paramName)
{
    ThrowError(env, CommonErrorCode::ERR_PARAM_CHECK, "Parameter error. The " + paramName +
        " parameter is mandatory.");
}

void HiRetrievalNapiUtil::ParseJsHiRetrievalConfig(const napi_env env, const napi_value& jsVal,
    HiRetrievalMgr::Config& cfg)
{
    cfg.userType = GetStrTypeAttr(env, jsVal, CommonDef::USER_TYPE_ATTR_NAME, "");
    cfg.deviceType = GetStrTypeAttr(env, jsVal, CommonDef::DEVICE_TYPE_ATTR_NAME, "");
    cfg.deviceModel = GetStrTypeAttr(env, jsVal, CommonDef::DEVICE_MODEL_ATTR_NAME, "");
}

void HiRetrievalNapiUtil::CreateJsBoolValue(const napi_env env, bool boolVal, napi_value& jsVal)
{
    napi_status status = napi_get_boolean(env, boolVal, &jsVal);
    if (status != napi_ok) {
        HILOG_ERROR(LOG_CORE, "failed to get create napi value of bool type.");
    }
}

void HiRetrievalNapiUtil::CreateJsHiRetrievalConfig(const napi_env env, const HiRetrievalMgr::Config& cfg,
    napi_value& jsVal)
{
    napi_status status = napi_create_object(env, &jsVal);
    if (status != napi_ok) {
        HILOG_ERROR(LOG_CORE, "failed to get create napi value of hiRetrieval config.");
        return;
    }
    SetStrPropertyByName(env, jsVal, CommonDef::USER_TYPE_ATTR_NAME, cfg.userType);
    SetStrPropertyByName(env, jsVal, CommonDef::DEVICE_TYPE_ATTR_NAME, cfg.deviceType);
    SetStrPropertyByName(env, jsVal, CommonDef::DEVICE_MODEL_ATTR_NAME, cfg.deviceModel);
}

void HiRetrievalNapiUtil::CreateJsLongLongValue(const napi_env env, long long lldVal, napi_value& jsVal)
{
    napi_status status = napi_create_bigint_int64(env, lldVal, &jsVal);
    if (status != napi_ok) {
        HILOG_ERROR(LOG_CORE, "failed to create napi value of long long type.");
    }
}
} // namespace OHOS::HiviewDFX