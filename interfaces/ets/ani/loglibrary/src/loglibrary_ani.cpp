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

#include "loglibrary_ani.h"
#include "loglibrary_ani_util.h"
#include "hiview_service_agent.h"

using namespace OHOS::HiviewDFX;
namespace {
DEFINE_LOG_LABEL(0xD002D03, "LogLibraryAni");
}

ani_ref LogLibraryAni::List(ani_env *env, ani_string logType)
{
    ani_ref undefined = LogLibraryAniUtil::GetAniUndefined(env);
    if (!LogLibraryAniUtil::IsSystemAppCall()) {
        LogLibraryAniUtil::ThrowAniError(env, HiviewNapiErrCode::ERR_NON_SYS_APP_PERMISSION,
            "Permission denied, non-system app called system api.");
        return undefined;
    }
    std::string logTypeTemp = LogLibraryAniUtil::ParseStringValue(env, logType);
    std::vector<HiviewFileInfo> fileInfos;
    int32_t retCode = HiviewServiceAgent::GetInstance().List(logTypeTemp, fileInfos);
    HIVIEW_LOGI("retCode: %{public}u.", retCode);
    if (retCode == 0) {
        return LogLibraryAniUtil::ListResult(env, fileInfos);
    } else {
        LogLibraryAniUtil::ThrowAniError(env, LogLibraryAniUtil::GetErrorDetailByRet(retCode).first,
            LogLibraryAniUtil::GetErrorDetailByRet(retCode).second);
        return undefined;
    }
}

ani_object LogLibraryAni::Copy(ani_env *env, ani_string logType, ani_string logName, ani_string dest)
{
    if (!LogLibraryAniUtil::IsSystemAppCall()) {
        return LogLibraryAniUtil::CopyOrMoveResult(env,
            LogLibraryAniUtil::GetErrorDetailByRet(HiviewNapiErrCode::ERR_NON_SYS_APP_PERMISSION));
    }
    return LogLibraryAniUtil::CopyOrMoveFile(env, logType, logName, dest, false);
}

ani_object LogLibraryAni::Move(ani_env *env, ani_string logType, ani_string logName, ani_string dest)
{
    if (!LogLibraryAniUtil::IsSystemAppCall()) {
        return LogLibraryAniUtil::CopyOrMoveResult(env,
            LogLibraryAniUtil::GetErrorDetailByRet(HiviewNapiErrCode::ERR_NON_SYS_APP_PERMISSION));
    }
    return LogLibraryAniUtil::CopyOrMoveFile(env, logType, logName, dest, true);
}

void LogLibraryAni::Remove(ani_env *env, ani_string logType, ani_string logName)
{
    if (!LogLibraryAniUtil::IsSystemAppCall()) {
        LogLibraryAniUtil::ThrowAniError(env, HiviewNapiErrCode::ERR_NON_SYS_APP_PERMISSION,
            "Permission denied, non-system app called system api.");
        return;
    }

    std::string logTypeTemp = LogLibraryAniUtil::ParseStringValue(env, logType);
    std::string logNameTemp = LogLibraryAniUtil::ParseStringValue(env, logName);
    HIVIEW_LOGI("type: %{public}s, name: %{public}s", logTypeTemp.c_str(),
        StringUtil::HideSnInfo(logNameTemp).c_str());
    int32_t retCode = HiviewServiceAgent::GetInstance().Remove(logTypeTemp, logNameTemp);
    if (retCode != 0) {
        HIVIEW_LOGI("retCode: %{public}u.", retCode);
        LogLibraryAniUtil::ThrowAniError(env, LogLibraryAniUtil::GetErrorDetailByRet(retCode).first,
            LogLibraryAniUtil::GetErrorDetailByRet(retCode).second);
    }
}

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ani_env *env = nullptr;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        HILOG_ERROR(LOG_CORE, "Unsupported ANI_VERSION_1");
        return ANI_ERROR;
    }

    ani_namespace ns {};
    if (ANI_OK != env->FindNamespace(CLASS_NAME_LOGLIBRARY, &ns)) {
        HILOG_ERROR(LOG_CORE, "FindNamespace %{public}s failed", CLASS_NAME_LOGLIBRARY);
        return ANI_ERROR;
    }

    std::array methods = {
        ani_native_function {"list", nullptr, reinterpret_cast<void *>(LogLibraryAni::List)},
        ani_native_function {"copySync", nullptr, reinterpret_cast<void *>(LogLibraryAni::Copy)},
        ani_native_function {"moveSync", nullptr, reinterpret_cast<void *>(LogLibraryAni::Move)},
        ani_native_function {"remove", nullptr, reinterpret_cast<void *>(LogLibraryAni::Remove)},
    };

    if (ANI_OK != env->Namespace_BindNativeFunctions(ns, methods.data(), methods.size())) {
        HILOG_ERROR(LOG_CORE, "Namespace %{public}s BindNativeFunctions failed", CLASS_NAME_LOGLIBRARY);
        return ANI_ERROR;
    };

    *result = ANI_VERSION_1;
    return ANI_OK;
}
