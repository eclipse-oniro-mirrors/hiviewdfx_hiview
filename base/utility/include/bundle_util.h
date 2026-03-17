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

#ifndef HIVIEW_BUNDLE_UTIL_H
#define HIVIEW_BUNDLE_UTIL_H
#include <cstdint>
#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace BundleUtil {
std::string GetSandBoxPath(int32_t uid, const std::string& mainDir, const std::string& subDir);
std::string GetSandBoxPath(int32_t uid, const std::string& mainDir, const std::string& pathHolder,
    const std::string& subDir);
std::string GetApplicationNameById(int32_t uid);
int32_t GetUidByBundleName(const std::string& bundleName);
bool IsDebugHap(const int32_t uid);
}
}
}
#endif
