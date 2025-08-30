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

#ifndef LOGLIBRARY_ANI_UTIL_H
#define LOGLIBRARY_ANI_UTIL_H

#include <ani.h>
#include "hilog/log.h"
#include "hilog/log_cpp.h"
#include "hiview_file_info.h"
#include "hiview_err_code.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "loglibrary_ani_parameter_name.h"

namespace OHOS {
namespace HiviewDFX {
class LogLibraryAniUtil {
public:
    static std::string ParseStringValue(ani_env *env, ani_string aniStrRef);
    static bool CreateLogEntryArray(ani_env *env,
        const std::vector<HiviewFileInfo>& fileInfos, ani_array &logEntryArray);
    static ani_object CreateLogEntryObject(ani_env *env);
    static ani_ref ListResult(ani_env *env, const std::vector<HiviewFileInfo>& fileInfos);
    static bool IsSystemAppCall();
    static void ThrowAniError(ani_env *env, int32_t code, const std::string &message);
    static std::pair<int32_t, std::string> GetErrorDetailByRet(const int32_t retCode);
    static ani_object CopyOrMoveFile(ani_env *env,
        ani_string logType, ani_string logName, ani_string dest, bool isMove);
    static bool CheckDirPath(const std::string& path);
    static ani_object CopyOrMoveResult(ani_env *env, std::pair<int32_t, std::string> result);
    static ani_ref GetAniUndefined(ani_env *env);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // LOGLIBRARY_ANI_UTIL_H
