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
#ifndef I_FAULTLOG_MANAGER_SERVICE_H
#define I_FAULTLOG_MANAGER_SERVICE_H

#include <vector>
#include <string>
#include "faultlog_info.h"
#include "faultlog_query_result_inner.h"

namespace OHOS {
namespace HiviewDFX {
class IFaultLogManagerService {
public:
    // dump debug infos through cmdline
    virtual void Dump(int fd, const std::vector<std::string> &cmds) = 0;
    virtual void AddFaultLog(FaultLogInfo& info) = 0;
    virtual std::unique_ptr<FaultLogQueryResultInner> QuerySelfFaultLog(int32_t uid,
        int32_t pid, int32_t faultType, int32_t maxNum) = 0;

    virtual ~IFaultLogManagerService() = default;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif
