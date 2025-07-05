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
#include "hiebpf_decorator.h"
#include "decorator_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
const std::string HIEBPF_COLLECTOR_NAME = "HiebpfCollector";
StatInfoWrapper HiebpfDecorator::statInfoWrapper_;

CollectResult<bool> HiebpfDecorator::StartHiebpf(int duration,
    const std::string processName,
    const std::string outFile)
{
    auto task = [this, &duration, &processName, &outFile] {
        return hiebpfCollector_->StartHiebpf(duration, processName, outFile);
    };
    return Invoke(task, statInfoWrapper_, HIEBPF_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<bool> HiebpfDecorator::StopHiebpf()
{
    auto task = [this] { return hiebpfCollector_->StopHiebpf(); };
    return Invoke(task, statInfoWrapper_, HIEBPF_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

void HiebpfDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::list<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false, UC_STAT_LOG_PATH);
}

void HiebpfDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
