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
#ifndef FAULTLOG_MANAGER_SERVICE_H
#define FAULTLOG_MANAGER_SERVICE_H

#include "i_faultlog_manager_service.h"

#include "faultlog_manager.h"
namespace OHOS {
namespace HiviewDFX {
class FaultLogManagerService : public IFaultLogManagerService {
public:
    FaultLogManagerService(std::shared_ptr<EventLoop> workloop, std::shared_ptr<FaultLogManager> faultLogManager)
        : workLoop_(workloop), faultLogManager_(faultLogManager) {}
    void Dump(int fd, const std::vector<std::string> &cmds) override;
    void AddFaultLog(FaultLogInfo& info) override;
    std::unique_ptr<FaultLogQueryResultInner> QuerySelfFaultLog(int32_t uid,
        int32_t pid, int32_t faultType, int32_t maxNum) override;
private:
        std::shared_ptr<EventLoop> workLoop_;
        std::shared_ptr<FaultLogManager> faultLogManager_;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif
