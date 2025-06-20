/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "hiview_napi_util.h"

#include <map>

#include "hiview_err_code.h"
#include "hiview_logger.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D10, "HiviewNapiUtil");
namespace {
constexpr char FILE_NAME_KEY[] = "name";
constexpr char FILE_TIME_KEY[] = "mtime";
constexpr char FILE_SIZE_KEY[] = "size";
constexpr int32_t BUF_SIZE = 1024;
}

void HiviewNapiUtil::CreateUndefined(const napi_env env, napi_value& ret)
{
    if (napi_get_undefined(env, &ret) != napi_ok) {
        HIVIEW_LOGE("failed to create undefined value.");
    }
}

void HiviewNapiUtil::CreateInt32Value(const napi_env env, int32_t value, napi_value& ret)
{
    if (napi_create_int32(env, value, &ret) != napi_ok) {
        HIVIEW_LOGE("failed to create int32 napi value.");
    }
}

void HiviewNapiUtil::CreateInt64Value(const napi_env env, int64_t value, napi_value& ret)
{
    if (napi_create_int64(env, value, &ret) != napi_ok) {
        HIVIEW_LOGE("failed to create int64 napi value.");
    }
}

void HiviewNapiUtil::CreateStringValue(const napi_env env, const std::string& value, napi_value& ret)
{
    if (napi_create_string_utf8(env, value.c_str(), NAPI_AUTO_LENGTH, &ret) != napi_ok) {
        HIVIEW_LOGE("failed to create string napi value.");
    }
}

void HiviewNapiUtil::CreateErrorByRet(napi_env env, const int32_t retCode, napi_value& ret)
{
    auto detail = GetErrorDetailByRet(env, retCode);
    napi_value napiCode = nullptr;
    CreateStringValue(env, std::to_string(detail.first), napiCode);
    napi_value napiStr = nullptr;
    CreateStringValue(env, detail.second, napiStr);
    if (napi_create_error(env, napiCode, napiStr, &ret) != napi_ok) {
        HIVIEW_LOGE("failed to create napi error");
    }
}

std::pair<int32_t, std::string> HiviewNapiUtil::GetErrorDetailByRet(napi_env env, const int32_t retCode)
{
    HIVIEW_LOGI("origin result code is %{public}d.", retCode);
    const std::map<int32_t, std::pair<int32_t, std::string>> errMap = {
        {HiviewNapiErrCode::ERR_PERMISSION_CHECK, {HiviewNapiErrCode::ERR_PERMISSION_CHECK,
            "Permission denied. The app does not have the necessary permissions."}},
        {HiviewNapiErrCode::ERR_INNER_INVALID_LOGTYPE,
            {HiviewNapiErrCode::ERR_PARAM_CHECK, "Parameter error. The value of logType is invalid."}},
        {HiviewNapiErrCode::ERR_INNER_READ_ONLY,
            {HiviewNapiErrCode::ERR_PARAM_CHECK, "Parameter error. The specified logType is read-only."}},
        {HiviewNapiErrCode::ERR_SOURCE_FILE_NOT_EXIST,
            {HiviewNapiErrCode::ERR_SOURCE_FILE_NOT_EXIST, "Source file does not exists."}}
    };
    return errMap.find(retCode) == errMap.end() ?
        std::make_pair(HiviewNapiErrCode::ERR_DEFAULT, "Environment is abnormal.") : errMap.at(retCode);
}

bool HiviewNapiUtil::IsMatchType(napi_env env, const napi_value& value, napi_valuetype type)
{
    napi_valuetype paramType;
    napi_typeof(env, value, &paramType);
    return paramType == type;
}

void HiviewNapiUtil::SetNamedProperty(
    const napi_env env, napi_value& object, const std::string& propertyName, napi_value& propertyValue)
{
    if (napi_set_named_property(env, object, propertyName.c_str(), propertyValue) != napi_ok) {
        HIVIEW_LOGE("set %{public}s property failed", propertyName.c_str());
    }
}

bool HiviewNapiUtil::ParseStringValue(
    const napi_env env, const std::string& paramName, const napi_value& value, std::string& retValue)
{
    if (!IsMatchType(env, value, napi_string)) {
        HIVIEW_LOGE("parameter %{public}s type isn't string", paramName.c_str());
        ThrowParamTypeError(env, paramName, "string");
        return false;
    }
    char buf[BUF_SIZE] = {0};
    size_t bufLength = 0;
    if (napi_get_value_string_utf8(env, value, buf, BUF_SIZE - 1, &bufLength) != napi_ok) {
        HIVIEW_LOGE("failed to parse napi value of string type.");
    } else {
        retValue = std::string {buf};
    }
    return true;
}

void HiviewNapiUtil::CreateJsFileInfo(const napi_env env, const HiviewFileInfo& fileInfo, napi_value& val)
{
    napi_value name;
    CreateStringValue(env, fileInfo.name, name);
    SetNamedProperty(env, val, FILE_NAME_KEY, name);

    napi_value mtime;
    CreateInt64Value(env, fileInfo.mtime, mtime);
    SetNamedProperty(env, val, FILE_TIME_KEY, mtime);

    napi_value size;
    CreateInt64Value(env, fileInfo.size, size);
    SetNamedProperty(env, val, FILE_SIZE_KEY, size);
}

napi_value HiviewNapiUtil::GenerateFileInfoResult(const napi_env env, const std::vector<HiviewFileInfo>& fileInfos)
{
    napi_value result;
    auto length = fileInfos.size();
    napi_create_array_with_length(env, length, &result);
    for (decltype(length) i = 0; i < length; ++i) {
        napi_value item;
        if (napi_create_object(env, &item) != napi_ok) {
            HIVIEW_LOGE("failed to create a new napi object.");
            break;
        }
        CreateJsFileInfo(env, fileInfos[i], item);
        napi_set_element(env, result, i, item);
    }
    return result;
}

bool HiviewNapiUtil::CheckDirPath(const std::string& path)
{
    return path.empty() || path.find("..") == std::string::npos;
}

void HiviewNapiUtil::ThrowErrorByCode(napi_env env, int32_t errCode)
{
    auto detail = GetErrorDetailByRet(env, errCode);
    ThrowError(env, detail.first, detail.second);
}

void HiviewNapiUtil::ThrowParamContentError(napi_env env, const std::string& paramName)
{
    ThrowError(env, HiviewNapiErrCode::ERR_PARAM_CHECK,
        "Parameter error. The content of " + paramName + " is invalid.");
}

void HiviewNapiUtil::ThrowParamTypeError(napi_env env, const std::string& paramName, const std::string& paramType)
{
    ThrowError(env, HiviewNapiErrCode::ERR_PARAM_CHECK,
        "Parameter error. The type of " + paramName + " must be " + paramType + ".");
}

void HiviewNapiUtil::ThrowError(napi_env env, const int32_t code, const std::string& msg)
{
    if (napi_throw_error(env, std::to_string(code).c_str(), msg.c_str()) != napi_ok) {
        HIVIEW_LOGE("failed to throw error, code=%{public}d, msg=%{public}s", code, msg.c_str());
    }
}

void HiviewNapiUtil::ThrowSystemAppPermissionError(napi_env env)
{
    ThrowError(env, HiviewNapiErrCode::ERR_NON_SYS_APP_PERMISSION,
        "Permission denied, non-system app called system api.");
}

bool HiviewNapiUtil::IsSystemAppCall()
{
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    return Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(tokenId);
}
} // namespace HiviewDFX
} // namespace OHOS