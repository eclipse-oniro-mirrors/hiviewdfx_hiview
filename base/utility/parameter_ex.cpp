/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "parameter_ex.h"
#include <cstdint>

#include "parameter.h"
#include "parameters.h"
#include "sysversion.h"

namespace OHOS {
namespace HiviewDFX {
namespace Parameter {
namespace {
constexpr char DEFAULT_DES[] = "unknown";
constexpr char PATCH_VERSION[] = "ro.patchversion";
}
std::string GetString(const std::string& key, const std::string& defaultValue)
{
    return OHOS::system::GetParameter(key, defaultValue);
}

int64_t GetInteger(const std::string& key, const int64_t defaultValue)
{
    return OHOS::system::GetIntParameter(key, defaultValue);
}

uint64_t GetUnsignedInteger(const std::string& key, const uint64_t defaultValue)
{
    return OHOS::system::GetUintParameter(key, defaultValue);
}

bool GetBoolean(const std::string& key, const bool defaultValue)
{
    return OHOS::system::GetBoolParameter(key, defaultValue);
}

bool SetProperty(const std::string& key, const std::string& defaultValue)
{
    return OHOS::system::SetParameter(key, defaultValue);
}

int WaitParamSync(const char *key, const char *value, int timeout)
{
    return WaitParameter(key, value, timeout);
}

int WatchParamChange(const char *keyPrefix, ParameterChgPtr callback, void *context)
{
    return WatchParameter(keyPrefix, callback, context);
}

std::string GetDisplayVersionStr()
{
    static std::string displayedVersionStr = std::string(GetDisplayVersion());
    return displayedVersionStr;
}

bool IsBetaVersion()
{
    static bool isBetaVersion = GetString(KEY_HIVIEW_VERSION_TYPE, "unknown").find("beta") != std::string::npos;
    return isBetaVersion;
}

bool IsDeveloperMode()
{
    return GetBoolean(KEY_DEVELOPER_MODE_STATE, false);
}

bool IsOversea()
{
    // if param not been set, take it as china version
    static bool isOversea = (GetString("const.global.region", "CN") != "CN");
    return isOversea;
}

UserType GetUserType()
{
    if (IsOversea()) {
        return IsBetaVersion() ? USER_TYPE_OVERSEA_BETA : USER_TYPE_OVERSEA_COMMERCIAL;
    } else {
        return IsBetaVersion() ? USER_TYPE_CHINA_BETA : USER_TYPE_CHINA_COMMERCIAL;
    }
}

bool IsLeakStateMode()
{
    auto leakState = OHOS::system::GetParameter(KEY_LEAKDECTOR_MODE_STATE, "unknown");
    return (leakState.find("enable") != std::string::npos);
}

bool IsUserMode()
{
    return OHOS::system::GetParameter(KEY_IS_DEBUGGABLE, "0") == "0";
}

bool IsUCollectionSwitchOn()
{
    std::string ucollectionState = GetString(HIVIEW_UCOLLECTION_STATE, "false");
    return ucollectionState == "true";
}

bool IsTraceCollectionSwitchOn()
{
    std::string traceCollectionState = GetString(DEVELOP_HIVIEW_TRACE_RECORDER, "false");
    return traceCollectionState == "true";
}

std::string GetDeviceTypeStr()
{
    static std::string deviceType = OHOS::system::GetDeviceType();
    return deviceType;
}

std::string GetBrandStr()
{
    static std::string brandStr = std::string(GetBrand());
    return brandStr;
}

std::string GetManufactureStr()
{
    static std::string manufactureStr = std::string(GetManufacture());
    return manufactureStr;
}

std::string GetMarketNameStr()
{
    static std::string marketNameStr = std::string(GetMarketName());
    return marketNameStr;
}

std::string GetDeviceModelStr()
{
    std::string displayedVersionStr = std::string(GetDisplayVersion());
    auto pos = displayedVersionStr.find(' ');
    if (pos == displayedVersionStr.npos) {
        return DEFAULT_DES;
    }
    return displayedVersionStr.substr(0, pos);
}

std::string GetSysVersionStr()
{
    std::string displayedVersionStr = Parameter::GetDisplayVersionStr();
    auto pos = displayedVersionStr.find(' ');
    if (pos == displayedVersionStr.npos) {
        return displayedVersionStr;
    }
    return displayedVersionStr.substr(pos + 1, displayedVersionStr.size());
}

std::string GetDistributionOsVersionStr()
{
    static std::string osVersion = std::string(GetDistributionOSVersion());
    return osVersion;
}

std::string GetProductModelStr()
{
    static std::string productModelStr = std::string(GetProductModel());
    return productModelStr;
}

std::string GetSysVersionDetailsStr()
{
    std::string sysVersionDetails = std::to_string(GetMajorVersion());
    sysVersionDetails += ".";
    sysVersionDetails += std::to_string(GetSeniorVersion());
    sysVersionDetails += ".";
    sysVersionDetails += std::to_string(GetFeatureVersion());
    return sysVersionDetails;
}

std::string GetVersionTypeStr()
{
    return IsBetaVersion() ? "Beta" : "Commercial";
}

std::string GetPatchVersionStr()
{
    return GetString(PATCH_VERSION, "");
}
} // namespace Parameter
} // namespace HiviewDFX
} // namespace OHOS
