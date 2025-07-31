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

#ifndef LOGLIBRARY_ANI_H
#define LOGLIBRARY_ANI_H

#include <ani.h>

namespace OHOS {
namespace HiviewDFX {

class LogLibraryAni {
public:
    static ani_ref List(ani_env *env, ani_string logType);
    static ani_object Copy(ani_env *env, ani_string logType, ani_string logName, ani_string dest);
    static ani_object Move(ani_env *env, ani_string logType, ani_string logName, ani_string dest);
    static void Remove(ani_env *env, ani_string logType, ani_string logName);
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // LOGLIBRARY_ANI_H
