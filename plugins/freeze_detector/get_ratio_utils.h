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
 
#ifndef HIVIEWDFX_FREEZE_GETRATIO_H
#define HIVIEWDFX_FREEZE_GETRATIO_H
#include "singleton.h"
 
namespace OHOS {
namespace HiviewDFX {
 
class FreezeGetRatio : public DelayedSingleton<FreezeGetRatio> {
public:
    FreezeGetRatio();
    ~FreezeGetRatio();
    static FreezeGetRatio &GetInStance();
    static float GetAppfreezeTimeoutRatio();
    static float GetAbilitymsTimeoutRatio();
    static bool IsNumeric(const std::string &str);
private:
    static float GetAppfreezeRatio();
    static float GetAbilitymsRatio();
    static float GetRatio(const std::string& key, const std::string& defaultVal, int maxSize, float convertRatio,
        float defaultRatio);
    static float gAppfreezeTimeoutRatio;
    static float gAbilitymsTimeoutRatio;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // HIVIEWDFX_FREEZE_GETRATIO_H