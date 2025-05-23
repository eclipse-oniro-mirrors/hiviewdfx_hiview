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
#ifndef HIVIEW_OAL_SYSTEM_PROPERTY_H
#define HIVIEW_OAL_SYSTEM_PROPERTY_H
#include <cstdint>
#include <string>
namespace OHOS {
namespace HiviewDFX {
constexpr char KEY_BUILD_CHARACTER[] = "ro.build.characteristics";
constexpr char KEY_HIVIEW_VERSION_TYPE[] = "const.logsystem.versiontype";
constexpr char KEY_DEVELOPER_MODE_STATE[] = "const.security.developermode.state";
constexpr char HIVIEW_UCOLLECTION_STATE[] = "hiviewdfx.ucollection.switchon";
constexpr char DEVELOP_HIVIEW_TRACE_RECORDER[] = "persist.hiview.trace_recorder";
constexpr char KEY_LEAKDECTOR_MODE_STATE[] = "persist.hiview.leak_detector";
constexpr char KEY_IS_DEBUGGABLE[] = "const.debuggable";
namespace Parameter {
enum UserType: uint8_t {
    USER_TYPE_CHINA_COMMERCIAL = 1,
    USER_TYPE_CHINA_BETA = 3,
    USER_TYPE_OVERSEA_BETA = 5,
    USER_TYPE_OVERSEA_COMMERCIAL = 6
};

std::string GetString(const std::string& key, const std::string& defaultValue);
int64_t GetInteger(const std::string& key, const int64_t defaultValue);
uint64_t GetUnsignedInteger(const std::string& key, const uint64_t defaultValue);
bool GetBoolean(const std::string& key, const bool defaultValue);
bool SetProperty(const std::string& key, const std::string& defaultValue);
int WaitParamSync(const char *key, const char *value, int timeout);
typedef void (*ParameterChgPtr)(const char *key, const char *value, void *context);
int WatchParamChange(const char *keyPrefix, ParameterChgPtr callback, void *context);
int RemoveParameterWatcherEx(const char *keyPrefix, ParameterChgPtr callback, void *context);
bool IsBetaVersion();
bool IsDeveloperMode();
bool IsOversea();
UserType GetUserType();
bool IsUCollectionSwitchOn();
bool IsTraceCollectionSwitchOn();
bool IsTestAppTraceOn();
bool IsLeakStateMode();
bool IsUserMode();
std::string GetDeviceTypeStr();
std::string GetDisplayVersionStr();
std::string GetBrandStr();
std::string GetManufactureStr();
std::string GetMarketNameStr();
std::string GetDeviceModelStr();
std::string GetSysVersionStr();
std::string GetDistributionOsVersionStr();
std::string GetProductModelStr();
std::string GetSysVersionDetailsStr();
std::string GetVersionTypeStr();
std::string GetPatchVersionStr();
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_OAL_SYSTEM_PROPERTY_H
