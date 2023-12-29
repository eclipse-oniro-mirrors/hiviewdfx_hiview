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

#include "network_decorator.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
const std::string NET_COLLECTOR_NAME = "NetworkCollector";
StatInfoWrapper NetworkDecorator::statInfoWrapper_;

CollectResult<NetworkRate> NetworkDecorator::CollectRate()
{
    auto task = std::bind(&NetworkCollector::CollectRate, networkCollector_.get());
    return Invoke(task, statInfoWrapper_, NET_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<NetworkPackets> NetworkDecorator::CollectSysPackets()
{
    auto task = std::bind(&NetworkCollector::CollectSysPackets, networkCollector_.get());
    return Invoke(task, statInfoWrapper_, NET_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

void NetworkDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::vector<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false);
}

void NetworkDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
