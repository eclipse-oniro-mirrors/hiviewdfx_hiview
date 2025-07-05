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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_DECORATOR_UTIL_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_DECORATOR_UTIL_H
#include <list>
#include <string>

namespace OHOS::HiviewDFX::UCollectUtil {
void WriteLinesToFile(const std::list<std::string>& stats, bool addBlankLine, const std::string& path);
std::string GetCurrentDate();
}
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_DECORATOR_UTIL_H
