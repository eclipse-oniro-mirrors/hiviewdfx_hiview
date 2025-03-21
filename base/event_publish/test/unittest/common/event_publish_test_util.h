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

#ifndef EVENT_PUBLISH_TEST_UTIL_H
#define EVENT_PUBLISH_TEST_UTIL_H

#include <string>

namespace OHOS {
namespace HiviewDFX {
void InstallTestHap(const std::string& hapName);

void UninstallTestHap(const std::string& hapName);

int32_t LaunchTestHap(const std::string& abilityName, const std::string& bundleName);

void StopTestHap(const std::string& hapName);

int32_t GetPidByBundleName(const std::string& bundleName);

int32_t GetUidByPid(const int32_t pid);

bool FileExists(const std::string& filePath);

bool LoadlastLineFromFile(const std::string& filePath, std::string& lastLine);

std::string GetFileMd5Sum(const std::string& filePath, int delayTime = 0);
} // end of HiviewDFX
} // end of OHOS
#endif // EVENT_PUBLISH_TEST_UTIL_H
