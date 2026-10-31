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

#include "boot_detector_hiview.h"

#include <cstddef>

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char DATA_PREFIX[] = "/data";
constexpr char DATA_MIRROR_PREFIX[] = "/mnt/data_mirror";
constexpr size_t DATA_PREFIX_LEN = sizeof(DATA_PREFIX) - 1;

bool IsDataPrefixed(const std::string &path)
{
    if (path.size() < DATA_PREFIX_LEN) {
        return false;
    }
    if (path.compare(0, DATA_PREFIX_LEN, DATA_PREFIX) != 0) {
        return false;
    }
    return path.size() == DATA_PREFIX_LEN || path[DATA_PREFIX_LEN] == '/';
}
} // namespace

std::string BootDetectorReplaceDataPrefix(const std::string &path)
{
    if (!IsDataPrefixed(path)) {
        return path;
    }
    return DATA_MIRROR_PREFIX + path.substr(DATA_PREFIX_LEN);
}
} // namespace HiviewDFX
} // namespace OHOS
