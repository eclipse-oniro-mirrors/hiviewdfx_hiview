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

#ifndef OHOS_HIVIEWDFX_USER_DATA_SIZE_REPORTER_H
#define OHOS_HIVIEWDFX_USER_DATA_SIZE_REPORTER_H

#include <vector>
#include <string>
#include <utility>

#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class UserDataSizeReporter : public OHOS::DelayedRefSingleton<UserDataSizeReporter> {
public:
    void ReportUserDataSize(int32_t uid, const std::string& pathHolder, const std::string& eventName);

private:
    bool ShouldReport(const std::string& pathHolder);
    void ClearOverTimeRecord();

    std::mutex recordMutex_;
    std::vector<std::pair<std::string, uint64_t>> reportLimitRecords_; // pathHolder_eventName, stamp
};
} // namespace HiviewDFX
} // namespace OHOS

#endif
