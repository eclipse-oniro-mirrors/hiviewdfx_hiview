/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_BASE_UTILITY_SQL_UTIL_H
#define HIVIEW_BASE_UTILITY_SQL_UTIL_H

#include <string>
#include <utility>
#include <vector>

namespace OHOS {
namespace HiviewDFX {
namespace SqlUtil {
inline constexpr char COLUMN_TYPE_INT[] = "INTEGER";
inline constexpr char COLUMN_TYPE_STR[] = "TEXT";
inline constexpr char COLUMN_TYPE_DOU[] = "REAL";

std::string GenerateCreateSql(const std::string& table,
    const std::vector<std::pair<std::string, std::string>>& fields);
std::string GenerateExistSql(const std::string& table);
std::string GenerateDropSql(const std::string& table);
} // namespace SqlUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_UTILITY_SQL_UTIL_H
