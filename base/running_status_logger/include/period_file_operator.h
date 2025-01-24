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

#ifndef OHOS_HIVIEWDFX_PERIOD_INFO_FILE_OPT_H
#define OHOS_HIVIEWDFX_PERIOD_INFO_FILE_OPT_H

#include <fstream>
#include <list>
#include <string>

#include "plugin.h"

namespace OHOS {
namespace HiviewDFX {
class PeriodInfoFileOperator {
public:
    PeriodInfoFileOperator(HiviewContext* context, const std::string& fileName)
        : context_(context), fileName_(fileName) {}
    ~PeriodInfoFileOperator() = default;

public:
    using SplitPeriodInfoHandler = std::function<void(const std::vector<std::string>&)>;
    using PeriodContentBuilder = std::function<std::string()>;
    void ReadPeriodInfoFromFile(size_t splitItemCnt, SplitPeriodInfoHandler periodInfoHandler);
    void WritePeriodInfoToFile(PeriodContentBuilder contentBuilder);

private:
    std::string GetPeriodInfoFilePath();

private:
    HiviewContext* context_;
    std::string fileName_;
    uint64_t optCnt_ = 0;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_PERIOD_INFO_FILE_OPT_H