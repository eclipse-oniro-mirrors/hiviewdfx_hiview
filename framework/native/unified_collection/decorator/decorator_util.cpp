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
#include "decorator_util.h"

#include <ctime>

#include "time_util.h"

namespace OHOS::HiviewDFX::UCollectUtil {
void WriteLinesToFile(const std::list<std::string>& stats, bool addBlankLine, const std::string& path)
{
    FILE* fp = fopen(path.c_str(), "a");
    if (fp == nullptr) {
        return;
    }
    for (const auto& record : stats) {
        (void)fprintf(fp, "%s\n", record.c_str());
    }
    if (addBlankLine) {
        (void)fprintf(fp, "\n"); // write a blank line to separate content
    }
    (void)fclose(fp);
}

std::string GetCurrentDate()
{
    return TimeUtil::TimestampFormatToDate(std::time(nullptr), "%Y-%m-%d");
}
}
