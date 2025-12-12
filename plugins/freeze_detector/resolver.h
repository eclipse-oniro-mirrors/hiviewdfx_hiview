/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef FREEZE_RESOLVER_H
#define FREEZE_RESOLVER_H

#include <list>
#include <memory>
#include <set>
#include <vector>

#include "db_helper.h"
#include "freeze_common.h"
#include "watch_point.h"
#include "vendor.h"

namespace OHOS {
namespace HiviewDFX {
class FreezeResolver {
public:
    explicit FreezeResolver(std::shared_ptr<FreezeCommon> fc)
        : startTime_(time(nullptr) * MILLISECOND), freezeCommon_(fc) {};
    ~FreezeResolver() {};
    FreezeResolver& operator=(const FreezeResolver&) = delete;
    FreezeResolver(const FreezeResolver&) = delete;

    static const int MILLISECOND = 1000;

    bool Init();
    std::string GetTimeZone() const;
    int ProcessEvent(const WatchPoint &watchPoint) const;

private:
    bool ResolveEvent(const WatchPoint& watchPoint,
    std::vector<WatchPoint>& list, std::vector<FreezeResult>& result) const;
    bool JudgmentResult(const WatchPoint& watchPoint,
        const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const;
    bool IsSysWarning(const WatchPoint& watchPoint,
        const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const;
    bool IsAppFreezeWarningForBetaVersion(const WatchPoint& watchPoint,
        const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const;
    unsigned long startTime_;
    std::shared_ptr<FreezeCommon> freezeCommon_ = nullptr;
    std::shared_ptr<FreezeRuleCluster> freezeRuleCluster_ = nullptr;
    std::unique_ptr<DBHelper> dBHelper_ = nullptr;
    std::unique_ptr<Vendor> vendor_ = nullptr;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // FREEZE_DETECTOR_RESOLVER_H
