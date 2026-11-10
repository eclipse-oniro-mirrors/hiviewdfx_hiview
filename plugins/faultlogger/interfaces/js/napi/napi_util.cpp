/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
#include "napi_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger-napi");
napi_value NapiUtil::CreateErrorMessage(napi_env env, std::string msg)
{
    napi_value result = nullptr;
    napi_value message = nullptr;
    if (napi_create_string_utf8(env, msg.c_str(), msg.length(), &message) != napi_ok) {
        HIVIEW_LOGE("failed to create string");
        return nullptr;
    }
    napi_value codeValue = nullptr;
    std::string errCode = std::to_string(-1);
    if (napi_create_string_utf8(env, errCode.c_str(), errCode.length(), &codeValue) != napi_ok) {
        HIVIEW_LOGE("failed to create string");
        return nullptr;
    }
    if (napi_create_error(env, codeValue, message, &result) != napi_ok) {
        HIVIEW_LOGE("failed to create error");
        return nullptr;
    }
    return result;
}

napi_value NapiUtil::CreateUndefined(napi_env env)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    return result;
}

void NapiUtil::SetPropertyInt32(napi_env env, napi_value object, std::string name, int32_t value)
{
    napi_value propertyValue = nullptr;
    if (napi_create_int32(env, value, &propertyValue) != napi_ok) {
        HIVIEW_LOGE("failed to create int32");
        return;
    }
    napi_set_named_property(env, object, name.c_str(), propertyValue);
}

void NapiUtil::SetPropertyInt64(napi_env env, napi_value object, std::string name, int64_t value)
{
    napi_value propertyValue = nullptr;
    if (napi_create_int64(env, value, &propertyValue) != napi_ok) {
        HIVIEW_LOGE("failed to create int64");
        return;
    }
    napi_set_named_property(env, object, name.c_str(), propertyValue);
}

void NapiUtil::SetPropertyStringUtf8(napi_env env, napi_value object, std::string name, std::string value)
{
    napi_value propertyValue = nullptr;
    if (napi_create_string_utf8(env, value.c_str(), value.length(), &propertyValue) != napi_ok) {
        HIVIEW_LOGE("failed to create string");
        return;
    }
    napi_set_named_property(env, object, name.c_str(), propertyValue);
}

bool NapiUtil::IsMatchType(napi_env env, napi_value value, napi_valuetype type)
{
    napi_valuetype paramType;
    napi_typeof(env, value, &paramType);
    if (paramType == type) {
        return true;
    }
    return false;
}

napi_value NapiUtil::CreateString(const napi_env env, const std::string& str)
{
    napi_value strValue = nullptr;
    if (napi_create_string_utf8(env, str.c_str(), NAPI_AUTO_LENGTH, &strValue) != napi_ok) {
        HIVIEW_LOGE("failed to create string");
        return nullptr;
    }
    return strValue;
}

void NapiUtil::ThrowError(napi_env env, int code, const std::string& msg, bool isThrow)
{
    // no error needs to be thrown before api 9
    if (!isThrow) {
        return;
    }

    if (napi_throw_error(env, std::to_string(code).c_str(), msg.c_str()) != napi_ok) {
        HIVIEW_LOGE("failed to throw error, code=%{public}d, msg=%{public}s", code, msg.c_str());
    }
}

std::string NapiUtil::CreateServiceErrMsg()
{
    return "FaultLogger service is not running or broken.";
}

std::string NapiUtil::CreateParamCntErrMsg(size_t expectedMinCount, size_t expectedMaxCount, size_t actualCount)
{
    return "Parameter error. Incorrect parameter count. Expected: " + std::to_string(expectedMinCount) + "-" +
        std::to_string(expectedMaxCount) + ", Actual: " + std::to_string(actualCount) + ".";
}

std::string NapiUtil::CreateParamTypeErrMsg(size_t index, const std::string& expectedType,
    const std::string& actualType)
{
    return "Parameter error. Parm[" + std::to_string(index) + "] type mismatch. Expected: " + expectedType +
        ", Actual: " + actualType + ".";
}

std::string NapiUtil::CreateParamRequiredErrMsg(size_t index)
{
    return "Parameter error. Parm[" + std::to_string(index) + "] is required, cannot be null or undefined.";
}

std::string NapiUtil::GetTypeName(napi_env env, napi_value value)
{
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, value, &valueType) != napi_ok) {
        HIVIEW_LOGE("failed to get value type");
        return "unknown";
    }
    switch (valueType) {
        case napi_undefined:
            return "undefined";
        case napi_null:
            return "null";
        case napi_boolean:
            return "boolean";
        case napi_number:
            return "number";
        case napi_string:
            return "string";
        case napi_symbol:
            return "symbol";
        case napi_object:
            return "object";
        case napi_function:
            return "function";
        case napi_external:
            return "external";
        case napi_bigint:
            return "bigint";
        default:
            return "unknown";
    }
}
}  // namespace HiviewDFX
}  // namespace OHOS
