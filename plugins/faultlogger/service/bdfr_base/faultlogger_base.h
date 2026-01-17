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

#ifndef FAULTLOGGER_BASE_H
#define FAULTLOGGER_BASE_H

#include <memory>

#include "faultlogger_interface.h"

namespace OHOS {
namespace HiviewDFX {
class FaultloggerBase : public FaultloggerInterface {
public:
    FaultloggerBase();
    ~FaultloggerBase() override;
    void FaultLogDumpByCommands(int fd, const std::vector<std::string>& cmds) override;
    bool ProcessFaultLogEvent(const std::string& eventName, std::shared_ptr<Event>& event) override;
    void AddFaultLog(FaultLogType type, FaultLogInfo& info) override;
    std::list<FaultLogInfo> QuerySelfFaultLog(int32_t uid, int32_t pid, int32_t faultType, int32_t maxNum) override;
    void StartFaultLogBootScan() override;
    void SanitizerHandleUnorderedEvent(const Event& msg) override;
    bool EnableGwpAsanGrayscale(bool alwaysEnabled,
                                double sampleRate,
                                double maxSimutaneousAllocations,
                                int32_t duration,
                                int32_t uid) override;
    void DisableGwpAsanGrayscale(int32_t uid) override;
    uint32_t GetGwpAsanGrayscaleState(int32_t uid) override;
    uint64_t GetExtensionDelayTime() override;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // FAULTLOGGER_BASE_H
