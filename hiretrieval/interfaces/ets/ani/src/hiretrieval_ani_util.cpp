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

#include "hiretrieval_ani_util.h"

#include "hilog/log.h"
#include "hiretrieval_common_util.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D10

#undef LOG_TAG
#define LOG_TAG "HIRETRIEVAL_ANI_UTIL"

using namespace OHOS::HiviewDFX::HiRetrieval;

namespace OHOS::HiviewDFX {
namespace {
constexpr char CLASS_NAME_HIRETRIEVAL_CONFIG[] = "@ohos.hiviewdfx.hiRetrieval.HiRetrievalConfigAni";
constexpr char CLASS_NAME_BUSINESSERROR[] = "@ohos.base.BusinessError";
constexpr char FUNC_NAME_CTOR[] = "<ctor>";

std::string ParseStrVal(ani_env* env, ani_ref aniStrRef)
{
    ani_size strSize = 0;
    if (env->String_GetUTF8Size(static_cast<ani_string>(aniStrRef), &strSize) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to get string length.");
        return "";
    }
    ani_size buffSize = strSize + 1;
    std::vector<char> buffer(buffSize);
    char* utf8Buffer = buffer.data();
    ani_size bytesWritten = 0;
    if (env->String_GetUTF8(static_cast<ani_string>(aniStrRef), utf8Buffer, buffSize, &bytesWritten) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to get string.");
        return "";
    }
    utf8Buffer[bytesWritten] = '\0';
    std::string content = std::string(utf8Buffer);
    return content;
}

bool CreateStrVal(ani_env* env, const std::string& strVal, ani_string& strObj)
{
    if (env->String_NewUTF8(strVal.c_str(), strVal.size(), &strObj) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to create string ani object.");
        return false;
    }
    return true;
}

bool SetStrPropertyByName(ani_env* env, ani_class& aniClass, ani_object& destObj, const char* propertyName,
    const std::string& strVal)
{
    ani_string strObj {};
    if (!CreateStrVal(env, strVal, strObj)) {
        return false;
    }
    if (env->Object_SetPropertyByName_Ref(destObj, propertyName, static_cast<ani_ref>(strObj)) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "faile to set property: %{public}s.", propertyName);
        return false;
    }
    return true;
}

void ThrowAniError(ani_env* env, int32_t code, const std::string& msg)
{
    ani_class cls {};
    if (env->FindClass(CLASS_NAME_BUSINESSERROR, &cls) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to find error class.");
        return;
    }
    ani_method ctor {};
    if (env->Class_FindMethod(cls, FUNC_NAME_CTOR, ":", &ctor) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to find error constructor method.");
        return;
    }
    ani_object error {};
    if (env->Object_New(cls, ctor, &error) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to create error object.");
        return;
    }
    if (env->Object_SetPropertyByName_Int(error, "code_", static_cast<ani_int>(code)) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to set property code_ of error object.");
        return;
    }
    ani_string msgRef {};
    if (env->String_NewUTF8(msg.c_str(), msg.size(), &msgRef) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to create new message string.");
        return;
    }
    if (env->Object_SetPropertyByName_Ref(error, "message", static_cast<ani_ref>(msgRef)) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed set property message of error object.");
        return;
    }
    if (env->ThrowError(static_cast<ani_error>(error)) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to throwError ani error.");
    }
}

std::string GetStrTypeAttr(ani_env* env, ani_object& aniVal, const char* attrName,
    const std::string& defaultVal)
{
    ani_ref attrRef {};
    if (env == nullptr || attrName == nullptr) {
        HILOG_ERROR(LOG_CORE, "ani environment is invalid.");
        return defaultVal;
    }
    if (env->Object_GetPropertyByName_Ref(aniVal, attrName, &attrRef) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to get attr value of %{public}s.", attrName);
        return defaultVal;
    }
    std::string attrVal = ParseStrVal(env, attrRef);
    if (attrVal.empty()) {
        return defaultVal;
    }
    return attrVal;
}
}

void HiRetrievalAniUtil::CheckRetAndThrowError(ani_env* env, int32_t retCode)
{
    if (env == nullptr) {
        HILOG_ERROR(LOG_CORE, "ani environment is invalid.");
        return;
    }
    auto detail = CommonUtil::GetErrorDetailByRet(retCode);
    if (detail.first == NativeErrorCode::SUCC) {
        return;
    }
    ThrowAniError(env, detail.first, detail.second);
}

void HiRetrievalAniUtil::ParseJsHiRetrievalConfig(ani_env* env, ani_object& aniVal, HiRetrievalMgr::Config& cfg)
{
    cfg.userType = GetStrTypeAttr(env, aniVal, CommonDef::USER_TYPE_ATTR_NAME, "");
    cfg.deviceType = GetStrTypeAttr(env, aniVal, CommonDef::DEVICE_TYPE_ATTR_NAME, "");
    cfg.deviceModel = GetStrTypeAttr(env, aniVal, CommonDef::DEVICE_MODEL_ATTR_NAME, "");
}

void HiRetrievalAniUtil::CreateJsHiRetrievalConfig(ani_env* env, const HiRetrievalMgr::Config& cfg, ani_object& cfgObj)
{
    if (env == nullptr) {
        HILOG_ERROR(LOG_CORE, "ani environment is invalid.");
        return;
    }
    ani_class cfgClass {};
    if (env->FindClass(CLASS_NAME_HIRETRIEVAL_CONFIG, &cfgClass) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to find class HiRetrivalConfig.");
        return;
    }
    ani_method constructor {};
    if (env->Class_FindMethod(cfgClass, FUNC_NAME_CTOR, nullptr, &constructor) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to find constructor method of class HiRetrivalConfig.");
        return;
    }
    if (env->Object_New(cfgClass, constructor, &cfgObj) != ANI_OK) {
        HILOG_ERROR(LOG_CORE, "failed to create HiRetrivalConfig.");
        return;
    }
    if (SetStrPropertyByName(env, cfgClass, cfgObj, CommonDef::USER_TYPE_ATTR_NAME, cfg.userType) &&
        SetStrPropertyByName(env, cfgClass, cfgObj, CommonDef::DEVICE_TYPE_ATTR_NAME, cfg.deviceType) &&
        SetStrPropertyByName(env, cfgClass, cfgObj, CommonDef::DEVICE_MODEL_ATTR_NAME, cfg.deviceModel)) {
        return;
    }
    HILOG_WARN(LOG_CORE, "failed to set properties of HiRetrivalConfig.");
}
}; // namespace OHOS::HiviewDFX