/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "hilog_decorator.h"
#include "decorator_util.h"
#include <functional>

namespace OHOS::HiviewDFX::UCollectUtil {
constexpr char HILOG_COLLECTOR_NAME[] = "HilogCollector";
StatInfoWrapper HilogDecorator::statInfoWrapper_;

CollectResult<std::string> HilogDecorator::CollectLastLog(uint32_t pid, uint32_t num)
{
    auto task = [this, &pid, &num] { return hilogCollector_->CollectLastLog(pid, num); };
    return Invoke(task, statInfoWrapper_, std::string(HILOG_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

void HilogDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::list<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false, UC_STAT_LOG_PATH);
}

void HilogDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}
} // namespace OHOS::HiviewDFX::UCollectUtil
