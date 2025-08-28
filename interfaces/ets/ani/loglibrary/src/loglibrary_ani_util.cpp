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

#include <map>
#include "loglibrary_ani_util.h"
#include "hiview_service_agent.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"

using namespace OHOS::HiviewDFX;
namespace {
DEFINE_LOG_LABEL(0xD002D10, "LogLibraryAniUtil");
static constexpr int32_t SUCCESS = 0;
}

std::string LogLibraryAniUtil::ParseStringValue(ani_env *env, ani_string aniStrRef)
{
    ani_size strSize = 0;
    if (ANI_OK != env->String_GetUTF8Size(aniStrRef, &strSize)) {
        HILOG_ERROR(LOG_CORE, "String_GetUTF8Size Failed");
        return "";
    }
    std::vector<char> buffer(strSize + 1);
    char* utf8Buffer = buffer.data();
    ani_size bytesWritten = 0;
    if (ANI_OK != env->String_GetUTF8(aniStrRef, utf8Buffer, strSize + 1, &bytesWritten)) {
        HILOG_ERROR(LOG_CORE, "String_GetUTF8 Failed");
        return "";
    }
    utf8Buffer[bytesWritten] = '\0';
    std::string content = std::string(utf8Buffer);
    return content;
}

bool LogLibraryAniUtil::CreateLogEntryArray(ani_env *env,
    const std::vector<HiviewFileInfo>& fileInfos, ani_array_ref &logEntryArray)
{
    ani_size index = 0;
    for (const auto& value: fileInfos) {
        std::string name = value.name;
        ani_string name_string{};
        env->String_NewUTF8(name.c_str(), name.size(), &name_string);
        ani_object logEntryObj = LogLibraryAniUtil::CreateLogEntryObject(env);
        if (ANI_OK != env->Object_SetPropertyByName_Ref(logEntryObj, "name", name_string)) {
            HILOG_ERROR(LOG_CORE, "Set LogEntry name Fail: %{public}s", CLASS_NAME_LOGENTRY);
            return false;
        }
        if (ANI_OK != env->Object_SetPropertyByName_Double(logEntryObj, "mtime", value.mtime)) {
            HILOG_ERROR(LOG_CORE, "Set LogEntry mtime Fail: %{public}s", CLASS_NAME_LOGENTRY);
            return false;
        }
        if (ANI_OK != env->Object_SetPropertyByName_Double(logEntryObj, "size", value.size)) {
            HILOG_ERROR(LOG_CORE, "Set LogEntry size Fail: %{public}s", CLASS_NAME_LOGENTRY);
            return false;
        }
        if (ANI_OK != env->Array_Set_Ref(logEntryArray, index, static_cast<ani_ref>(logEntryObj))) {
            HILOG_ERROR(LOG_CORE, "Set logEntryObj to array Fail: %{public}s", CLASS_NAME_LOGENTRY);
            return false;
        }
        index++;
    }
    return true;
}

ani_object LogLibraryAniUtil::CreateLogEntryObject(ani_env *env)
{
    ani_class cls {};
    ani_object logEntryObj {};
    if (ANI_OK != env->FindClass(CLASS_NAME_LOGENTRY, &cls)) {
        HILOG_ERROR(LOG_CORE, "FindClass %{public}s Failed", CLASS_NAME_LOGENTRY);
        return logEntryObj;
    }

    ani_method logEntryConstructor {};
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", nullptr, &logEntryConstructor)) {
        HILOG_ERROR(LOG_CORE, "get %{public}s ctor Failed", CLASS_NAME_LOGENTRY);
        return logEntryObj;
    }

    if (ANI_OK != env->Object_New(cls, logEntryConstructor, &logEntryObj)) {
        HILOG_ERROR(LOG_CORE, "Create Object Failed: %{public}s", CLASS_NAME_LOGENTRY);
        return logEntryObj;
    }
    return logEntryObj;
}

ani_ref LogLibraryAniUtil::ListResult(ani_env *env, const std::vector<HiviewFileInfo>& fileInfos)
{
    ani_class cls {};
    ani_array_ref logEntryArray {};
    if (ANI_OK != env->FindClass(CLASS_NAME_LOGENTRY, &cls)) {
        HILOG_ERROR(LOG_CORE, "FindClass %{public}s Failed", CLASS_NAME_LOGENTRY);
        return logEntryArray;
    }

    ani_ref undefinedRef = LogLibraryAniUtil::GetAniUndefined(env);
    if (ANI_OK != env->Array_New_Ref(cls, fileInfos.size(), undefinedRef, &logEntryArray)) {
        HILOG_ERROR(LOG_CORE, "Array_New_Ref Failed");
        return logEntryArray;
    }

    if (fileInfos.empty()) {
        return logEntryArray;
    }

    if (!CreateLogEntryArray(env, fileInfos, logEntryArray)) {
        HILOG_ERROR(LOG_CORE, "CreateLogEntryArray Failed");
        return logEntryArray;
    }
    return logEntryArray;
}

bool LogLibraryAniUtil::IsSystemAppCall()
{
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    return Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(tokenId);
}

void LogLibraryAniUtil::ThrowAniError(ani_env *env, int32_t code, const std::string &message)
{
    ani_class cls {};
    if (ANI_OK != env->FindClass(CLASS_NAME_BUSINESSERROR, &cls)) {
        HILOG_ERROR(LOG_CORE, "find class %{public}s failed", CLASS_NAME_BUSINESSERROR);
        return;
    }
    ani_method ctor {};
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", ":V", &ctor)) {
        HILOG_ERROR(LOG_CORE, "find method BusinessError constructor failed");
        return;
    }
    ani_object error {};
    if (ANI_OK != env->Object_New(cls, ctor, &error)) {
        HILOG_ERROR(LOG_CORE, "new object %{public}s failed", CLASS_NAME_BUSINESSERROR);
        return;
    }
    if (ANI_OK != env->Object_SetPropertyByName_Double(error, "code", static_cast<ani_double>(code))) {
        HILOG_ERROR(LOG_CORE, "set property BusinessError.code failed");
        return;
    }
    ani_string messageRef {};
    if (ANI_OK != env->String_NewUTF8(message.c_str(), message.size(), &messageRef)) {
        HILOG_ERROR(LOG_CORE, "new message string failed");
        return;
    }
    if (ANI_OK != env->Object_SetPropertyByName_Ref(error, "message", static_cast<ani_ref>(messageRef))) {
        HILOG_ERROR(LOG_CORE, "set property BusinessError.message failed");
        return;
    }
    if (ANI_OK != env->ThrowError(static_cast<ani_error>(error))) {
        HILOG_ERROR(LOG_CORE, "throwError ani_error object failed");
    }
}

std::pair<int32_t, std::string> LogLibraryAniUtil::GetErrorDetailByRet(const int32_t retCode)
{
    HIVIEW_LOGI("origin result code is %{public}d.", retCode);
    const std::map<int32_t, std::pair<int32_t, std::string>> errMap = {
        {SUCCESS, {SUCCESS, "Success."}},
        {HiviewNapiErrCode::ERR_PERMISSION_CHECK, {HiviewNapiErrCode::ERR_PERMISSION_CHECK,
            "Permission denied. The app does not have the necessary permissions."}},
        {HiviewNapiErrCode::ERR_NON_SYS_APP_PERMISSION, {HiviewNapiErrCode::ERR_NON_SYS_APP_PERMISSION,
            "Permission denied, non-system app called system api."}},
        {HiviewNapiErrCode::ERR_PARAM_CHECK,
            {HiviewNapiErrCode::ERR_PARAM_CHECK, "Parameter error. The content of dest is invalid."}},
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

ani_object LogLibraryAniUtil::CopyOrMoveFile(ani_env *env,
    ani_string logType, ani_string logName, ani_string dest, bool isMove)
{
    isMove ? HIVIEW_LOGI("call move") : HIVIEW_LOGI("call copy");
    std::string logTypeTemp = LogLibraryAniUtil::ParseStringValue(env, logType);
    std::string logNameTemp = LogLibraryAniUtil::ParseStringValue(env, logName);
    std::string destTemp = LogLibraryAniUtil::ParseStringValue(env, dest);
    HIVIEW_LOGI("type:%{public}s", logTypeTemp.c_str());

    if (!LogLibraryAniUtil::CheckDirPath(destTemp)) {
        HIVIEW_LOGE("dest param is invalid: %{public}s", destTemp.c_str());
        return LogLibraryAniUtil::CopyOrMoveResult(env,
            LogLibraryAniUtil::GetErrorDetailByRet(HiviewNapiErrCode::ERR_PARAM_CHECK));
    }

    if (isMove) {
        int32_t result = HiviewServiceAgent::GetInstance().Move(logTypeTemp, logNameTemp, destTemp);
        return LogLibraryAniUtil::CopyOrMoveResult(env, LogLibraryAniUtil::GetErrorDetailByRet(result));
    } else {
        int32_t result = HiviewServiceAgent::GetInstance().Copy(logTypeTemp, logNameTemp, destTemp);
        return LogLibraryAniUtil::CopyOrMoveResult(env, LogLibraryAniUtil::GetErrorDetailByRet(result));
    }
}

bool LogLibraryAniUtil::CheckDirPath(const std::string& path)
{
    return path.empty() || path.find("..") == std::string::npos;
}

ani_object LogLibraryAniUtil::CopyOrMoveResult(ani_env *env, std::pair<int32_t, std::string> result)
{
    ani_object results_obj {};
    ani_class cls {};
    if (ANI_OK != env->FindClass(CLASS_NAME_RESULTS, &cls)) {
        HILOG_ERROR(LOG_CORE, "failed to find class %{public}s", CLASS_NAME_RESULTS);
        return results_obj;
    }

    ani_method ctor {};
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", nullptr, &ctor)) {
        HILOG_ERROR(LOG_CORE, "get method %{public}s <ctor> failed", CLASS_NAME_RESULTS);
        return results_obj;
    }

    if (ANI_OK != env->Object_New(cls, ctor, &results_obj)) {
        HILOG_ERROR(LOG_CORE, "create object %{public}s failed", CLASS_NAME_RESULTS);
        return results_obj;
    }

    ani_method codeSetter {};
    if (ANI_OK != env->Class_FindMethod(cls, "<set>code", nullptr, &codeSetter)) {
        HILOG_ERROR(LOG_CORE, "get method codeSetter %{public}s failed", CLASS_NAME_RESULTS);
        return results_obj;
    }

    if (ANI_OK != env->Object_CallMethod_Void(results_obj, codeSetter, static_cast<ani_double>(result.first))) {
        HILOG_ERROR(LOG_CORE, "call method codeSetter %{public}s failed", CLASS_NAME_RESULTS);
        return results_obj;
    }

    ani_method messageSetter {};
    if (ANI_OK != env->Class_FindMethod(cls, "<set>message", nullptr, &messageSetter)) {
        HILOG_ERROR(LOG_CORE, "find method messageSetter %{public}s failed", CLASS_NAME_RESULTS);
        return results_obj;
    }

    std::string message = result.second;
    ani_string message_string {};
    env->String_NewUTF8(message.c_str(), message.size(), &message_string);

    if (ANI_OK != env->Object_CallMethod_Void(results_obj, messageSetter, message_string)) {
        HILOG_ERROR(LOG_CORE, "call method messageSetter Fail %{public}s", CLASS_NAME_RESULTS);
        return results_obj;
    }

    return results_obj;
}

ani_ref LogLibraryAniUtil::GetAniUndefined(ani_env *env)
{
    ani_ref result {};
    if (ANI_OK != env->GetUndefined(&result)) {
        HILOG_ERROR(LOG_CORE, "GetUndefined Fail");
        return result;
    }
    return result;
}
