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
 
#include "get_ratio_utils.h"
 
#include <charconv>
#include "hiview_logger.h"
#include "file_util.h"
#include "time_util.h"
#include "string_util.h"
#include "parameter.h"
#include "parameters.h"
 
namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr const char* KEY_APPFREEZE_TIMEOUT_RATIO = "const.sys.dfx.appfreeze.timeout_unit_time_ratio";
    constexpr const char* KEY_ABILITYMS_TIMEOUT_RATIO = "persist.sys.abilityms.timeout_unit_time_ratio";
    constexpr float FLOAT_EPSILON = 0.01f;
    constexpr int32_t TIME_CONVERT_RATIO = 1000;
    constexpr int32_t MAX_APPFREEZE_RATIO_SIZE = 4;
    constexpr int32_t MAX_ABILITYMS_RATIO_SIZE = 2;
}
 
float FreezeGetRatio::g_appfreezeTimeoutRatio = 0.0f;
float FreezeGetRatio::g_abilitymsTimeoutRatio = 0.0f;
 
FreezeGetRatio::FreezeGetRatio()
{
    g_appfreezeTimeoutRatio = getAppfreezeRatio();
    g_abilitymsTimeoutRatio = getAbilitymsRatio();
}
 
FreezeGetRatio::~FreezeGetRatio()
{}
 
FreezeGetRatio &FreezeGetRatio::GetInStance()
{
    static FreezeGetRatio instance;
    return instance;
}
 
float FreezeGetRatio::getAppfreezeTimeoutRatio()
{
    if (g_appfreezeTimeoutRatio > FLOAT_EPSILON) {
        return g_appfreezeTimeoutRatio;
    }
    return 1.0f;
}
 
float FreezeGetRatio::getAbilitymsTimeoutRatio()
{
    if (g_abilitymsTimeoutRatio > FLOAT_EPSILON) {
        return g_abilitymsTimeoutRatio;
    }
    return 1.0f;
}
 
float FreezeGetRatio::getAppfreezeRatio()
{
    return getRatio(KEY_APPFREEZE_TIMEOUT_RATIO, "1000", MAX_APPFREEZE_RATIO_SIZE, 1.0f / TIME_CONVERT_RATIO, 1.0f);
}
 
float FreezeGetRatio::getAbilitymsRatio()
{
    return getRatio(KEY_ABILITYMS_TIMEOUT_RATIO, "1", MAX_ABILITYMS_RATIO_SIZE, 1.0f, 1.0f);
}
 
float FreezeGetRatio::getRatio(const std::string& key, const std::string& defaultVal, int maxSize,
    float convertRatio = 1.0f, float defaultRatio = 1.0f)
{
    std::string ratioStr = OHOS::system::GetParameter(key, defaultVal);
    if (ratioStr.empty() || !IsNumeric(ratioStr)) {
        return defaultRatio;
    }
 
    if (ratioStr.size() > maxSize) {
        return defaultRatio;
    }
 
    uint64_t ratioVal = 0;
    auto [ptr, ec] = std::from_chars(ratioStr.data(), ratioStr.data() + ratioStr.size(), ratioVal);
    if (ec != std::errc()) {
        return defaultRatio;
    }
 
    float result = ratioVal * convertRatio;
    return (result < FLOAT_EPSILON) ? defaultRatio : result;
}
 
bool FreezeGetRatio::IsNumeric(const std::string &str)
{
    return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}
}  // namespace HiviewDFX
}  // namespace OHOS