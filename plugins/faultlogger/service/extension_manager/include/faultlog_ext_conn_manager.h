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

#ifndef FAULT_LOG_EXT_CONN_MANAGER_H
#define FAULT_LOG_EXT_CONN_MANAGER_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "ability_manager_interface.h"
#include "bundlemgr/bundle_mgr_interface.h"
#include "faultlog_ext_connection.h"
#include "faultlog_util.h"
#include "ffrt.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogExtConnManager : public OHOS::DelayedRefSingleton<FaultLogExtConnManager> {
public:
    /**
     * @brief on Fault.
     *
     * @return True if success, else false.
     */
    bool OnFault(const FaultLogInfo& info);

private:
    std::string GetExtName(const std::string& bundleName, int32_t userId) const;

    bool IsExistList(const std::string& bundleName) const;
    void AddToList(const std::string& bundleName);
    void RemoveFromList(const std::string& bundleName);
    bool IsExtension(const FaultLogInfo& info) const;

    mutable std::mutex waitStartMtx_;
    std::unordered_set<std::string> waitStartList_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // FAULT_LOG_EXT_CONN_MANAGER_H
