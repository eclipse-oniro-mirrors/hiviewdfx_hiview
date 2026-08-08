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
#ifndef HIVIEW_FREEZE_STACK_SUMMARY_H
#define HIVIEW_FREEZE_STACK_SUMMARY_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace OHOS {
namespace HiviewDFX {

struct FreezeStackSummary {
    std::string status;
    int32_t totalSamples {0};
    int32_t busiestCount {0};
    int32_t busiestRatioPermille {0};
    uint64_t firstSnapshotTime {0};
    uint64_t stackId {0};
    std::vector<std::string> frames;
};

bool ParseFreezeStackSummary(const std::string& content, FreezeStackSummary& summary);
bool ApplyFreezeStackSummary(const std::string& pathList, std::map<std::string, std::string>& eventInfos);

}  // namespace HiviewDFX
}  // namespace OHOS
#endif  // HIVIEW_FREEZE_STACK_SUMMARY_H
