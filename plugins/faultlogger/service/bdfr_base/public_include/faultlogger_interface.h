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

#ifndef FAULTLOGGER_INTERFACE_H
#define FAULTLOGGER_INTERFACE_H

#include <cstdint>
#include <memory>

#include "event.h"
#include "faultlog_info_inner.h"

namespace OHOS {
namespace HiviewDFX {
class FaultloggerInterface {
public:
    virtual ~FaultloggerInterface() = default;
    virtual void FaultLogDumpByCommands(int fd, const std::vector<std::string>& cmds) = 0;
    virtual bool ProcessFaultLogEvent(const std::string& eventName, std::shared_ptr<Event>& event) = 0;
    virtual void AddFaultLog(FaultLogType type, FaultLogInfo& info) = 0;
    virtual std::list<FaultLogInfo> QuerySelfFaultLog(int32_t uid, int32_t pid, int32_t faultType, int32_t maxNum) = 0;
    virtual void StartFaultLogBootScan() = 0;
    virtual void SanitizerHandleUnorderedEvent(const Event& msg) = 0;
    virtual bool EnableGwpAsanGrayscale(bool alwaysEnabled,
                                        double sampleRate,
                                        double maxSimutaneousAllocations,
                                        int32_t duration,
                                        int32_t uid) = 0;
    virtual void DisableGwpAsanGrayscale(int32_t uid) = 0;
    virtual uint32_t GetGwpAsanGrayscaleState(int32_t uid) = 0;
    virtual uint64_t GetExtensionDelayTime() = 0;
};

extern "C" {
    constexpr const char* const newBdfrBaseSymbol = "NewFaultloggerInterface";
    typedef FaultloggerInterface* (*NewFaultloggerInterfaceFunc)(void);
    FaultloggerInterface* NewFaultloggerInterface(void);
}
} // namespace HiviewDFX
} // namespace OHOS

#endif // FAULTLOGGER_INTERFACE_H
