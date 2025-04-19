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
#ifndef I_FAULTLOG_DATABASE_H
#define I_FAULTLOG_DATABASE_H

#include <list>
#include "faultlog_info.h"

namespace OHOS {
namespace HiviewDFX {
class IFaultLogDatabase {
public:
    virtual ~IFaultLogDatabase() = default;
    virtual void SaveFaultLogInfo(FaultLogInfo& info) = 0;
    virtual std::list<FaultLogInfo> GetFaultInfoList(
        const std::string& module, int32_t id, int32_t faultType, int32_t maxNum) = 0;
    virtual bool IsFaultExist(int32_t pid, int32_t uid, int32_t faultType) = 0;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif
