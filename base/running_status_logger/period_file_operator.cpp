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

#include "period_file_operator.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <iomanip>
#include <vector>

#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-PeriodInfoFileOperator");
namespace {
constexpr size_t PERIOD_FILE_WROTE_STEP = 100;
}

PeriodInfoFileOperator::PeriodInfoFileOperator(HiviewContext* context, const std::string& fileName)
{
    filePath_ = GetPeriodInfoFilePath(context, fileName);
}

void PeriodInfoFileOperator::ReadPeriodInfoFromFile(size_t splitItemCnt, SplitPeriodInfoHandler periodInfoHandler)
{
    if (!Parameter::IsBetaVersion()) {
        HIVIEW_LOGD("no need to read.");
        return;
    }
    if (periodInfoHandler == nullptr) {
        HIVIEW_LOGE("period info handler is null");
        return;
    }

    std::vector<std::string> allLineContent;
    if (!FileUtil::LoadLinesFromFile(filePath_, allLineContent)) {
        HIVIEW_LOGE("failed to read some lines from %{public}s", filePath_.c_str());
        return;
    }
    for (const auto& lineContent : allLineContent) {
        std::vector<std::string> infoDetails;
        StringUtil::SplitStr(lineContent, " ", infoDetails);
        if (infoDetails.size() != splitItemCnt) {
            HIVIEW_LOGW("incorrect line content: %{public}s", lineContent.c_str());
            break;
        }
        periodInfoHandler(infoDetails);
    }
}

void PeriodInfoFileOperator::WritePeriodInfoToFile(PeriodContentBuilder contentBuilder)
{
    ++optCnt_;
    if (optCnt_ % PERIOD_FILE_WROTE_STEP != 0) {
        return;
    }

    if (contentBuilder == nullptr) {
        HIVIEW_LOGW("content builder is null or content is empty");
        return;
    }

    std::ofstream periodFileStream;
    periodFileStream.open(filePath_, std::ios::trunc);
    if (!periodFileStream.is_open()) {
        HIVIEW_LOGW("failed to open %{public}s", filePath_.c_str());
        return;
    }
    periodFileStream << contentBuilder() << std::endl;
}

std::string PeriodInfoFileOperator::GetPeriodInfoFilePath(HiviewContext* context, const std::string& fileName)
{
    if (context == nullptr) {
        HIVIEW_LOGW("context is null");
        return "";
    }

    std::string periodFilePath = context->GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY);
    periodFilePath = FileUtil::IncludeTrailingPathDelimiter(periodFilePath.append("sys_event_db/count_statistic/"));
    if (!FileUtil::IsDirectory(periodFilePath) && !FileUtil::ForceCreateDirectory(periodFilePath)) {
        HIVIEW_LOGE("failed to init directory %{public}s.", periodFilePath.c_str());
        return "";
    }
    periodFilePath.append(fileName);
    return periodFilePath;
}

} // namespace HiviewDFX
} // namespace OHOS