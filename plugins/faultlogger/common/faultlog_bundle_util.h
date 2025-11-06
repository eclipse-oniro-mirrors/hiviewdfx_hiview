/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef FAULTLOG_BUNDLE_UTIL_H
#define FAULTLOG_BUNDLE_UTIL_H
#include <algorithm>
#include <string>
#include <vector>
#include <sys/types.h>
namespace OHOS {
namespace HiviewDFX {
constexpr unsigned BUF_SIZE = 1024;
constexpr int MIN_APP_USERID = 10000;
constexpr uint32_t MAX_NAME_LENGTH = 4096;
constexpr int AID_SYSTEM = 1000;
constexpr mode_t DEFAULT_LOG_FILE_MODE = 0644;
constexpr mode_t DEFAULT_LOG_DIR_MODE = 0775;

struct DfxBundleInfo {
    bool isPreInstalled;
    uint32_t versionCode;
    std::string versionName;
    std::string cpuAbi;
    std::string releaseType;
};

bool IsModuleNameValid(const std::string& name);
std::string GetApplicationNameById(int32_t uid);
bool GetDfxBundleInfo(const std::string& bundleName, DfxBundleInfo& bundleInfo);
bool IsNameValid(const std::string& name, const std::string& sep, bool canEmpty);
bool GetIsSystemApp(const std::string &module, int32_t uid);
} // namespace HiviewDFX
} // namespace OHOS
#endif
