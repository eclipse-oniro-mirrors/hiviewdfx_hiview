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

#include "process_decorator.h"
#include "decorator_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
constexpr char PROCESS_COLLECTOR_NAME[] = "ProcessCollector";
StatInfoWrapper ProcessDecorator::statInfoWrapper_;

CollectResult<std::unordered_set<int32_t>> ProcessDecorator::GetMemCgProcesses()
{
    auto task = [this] { return processCollector_->GetMemCgProcesses(); };
    return Invoke(task, statInfoWrapper_, std::string(PROCESS_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<bool> ProcessDecorator::IsMemCgProcess(int32_t pid)
{
    auto task = [this, pid] { return processCollector_->IsMemCgProcess(pid); };
    return Invoke(task, statInfoWrapper_, std::string(PROCESS_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::string> ProcessDecorator::ExportMemCgProcesses()
{
    auto task = [this] { return processCollector_->ExportMemCgProcesses(); };
    return Invoke(task, statInfoWrapper_, std::string(PROCESS_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

void ProcessDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::list<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false, UC_STAT_LOG_PATH);
}

void ProcessDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
