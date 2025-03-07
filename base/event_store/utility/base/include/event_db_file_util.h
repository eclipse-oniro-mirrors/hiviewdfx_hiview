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

#ifndef HIVIEW_BASE_EVENT_STORE_UTILITY_DB_FILE_UTIL_H
#define HIVIEW_BASE_EVENT_STORE_UTILITY_DB_FILE_UTIL_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace HiviewDFX {
using ParseType = uint8_t;
constexpr ParseType ALL_INFO = 0x0F;
constexpr ParseType NAME_ONLY = 0x01;
constexpr ParseType TYPE_ONLY = 0x02;
constexpr ParseType LEVEL_ONLY = 0x04;
constexpr ParseType SEQ_ONLY = 0x08;

struct EventDbInfo {
    std::string name;
    uint8_t type = 0;
    std::string level;
    int64_t seq = 0;
};

class EventDbFileUtil {
public:
    static bool IsValidDbDir(const std::string& dir);
    static bool IsValidDbFilePath(const std::string& filePath);
    static bool ParseEventInfoFromDbFileName(const std::string& fileName, ParseType tag, EventDbInfo& info);
};
}
}

#endif // HIVIEW_BASE_EVENT_STORE_UTILITY_DB_FILE_UTIL_H