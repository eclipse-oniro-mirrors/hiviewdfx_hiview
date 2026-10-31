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

#ifndef HIVIEW_PLUGINS_RELIABILITY_BOOT_DETECTOR_INCLUDE_BOOT_DETECTOR_HIVIEW_H
#define HIVIEW_PLUGINS_RELIABILITY_BOOT_DETECTOR_INCLUDE_BOOT_DETECTOR_HIVIEW_H

#include <string>

namespace OHOS {
namespace HiviewDFX {
/**
 * Replace the "/data" prefix of the given path with the "/mnt/data_mirror" prefix.
 *
 * The prefix is replaced only when it matches a whole leading path segment, so "/data"
 * and "/data/app/el2/100" are rewritten, while "/database" is kept unchanged.
 *
 * @param path the path to be rewritten
 * @return the rewritten path; a path without the "/data" prefix, including an empty
 *         path, is returned unchanged
 */
std::string BootDetectorReplaceDataPrefix(const std::string &path);
} // namespace HiviewDFX
} // namespace OHOS

#endif
