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

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

#include "faultlog_info_inner.h"
#include "faultlogger_interface.h"
#include "faultlogbaseinterface_fuzzer.h"
#include "fuzz_data_source.h"
#include "sys_event.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;
constexpr int32_t MAX_CMD_COUNT = 5;

void FuzzDumpAndQuery(FaultloggerInterface* interface, FuzzDataSource& source)
{
    int32_t cmdCount = 0;
    (void)source.GetValue(cmdCount);
    cmdCount = cmdCount % (MAX_CMD_COUNT + 1);
    std::vector<std::string> cmds;
    for (int32_t i = 0; i < cmdCount; ++i) {
        std::string cmd;
        if (!source.GetString(cmd, MAX_STR_LEN)) {
            break;
        }
        cmds.push_back(cmd);
    }
    int fd = open("/dev/null", O_WRONLY);
    interface->FaultLogDumpByCommands(fd, cmds);
    if (fd >= 0) {
        close(fd);
    }
    int32_t uid = 0;
    int32_t pid = 0;
    int32_t faultType = 0;
    int32_t maxNum = 0;
    (void)source.GetValue(uid);
    (void)source.GetValue(pid);
    (void)source.GetValue(faultType);
    (void)source.GetValue(maxNum);
    (void)interface->QuerySelfFaultLog(uid, pid, faultType % FaultLogType::MAX_TYPE, maxNum);
    interface->StartFaultLogBootScan();
    (void)interface->GetExtensionDelayTime();
    (void)interface->CheckCallerIsAllowed();
}

void FuzzAddAndProcessEvent(FaultloggerInterface* interface, FuzzDataSource& source)
{
    int32_t faultLogType = 0;
    (void)source.GetValue(faultLogType);
    FaultLogInfo info;
    (void)source.GetValue(info.time);
    (void)source.GetValue(info.pid);
    (void)source.GetValue(info.id);
    info.faultLogType = faultLogType % FaultLogType::MAX_TYPE;
    std::string module;
    (void)source.GetString(module, MAX_STR_LEN);
    info.module = module;
    std::string reason;
    (void)source.GetString(reason, MAX_STR_LEN);
    info.reason = reason;
    interface->AddFaultLog(static_cast<FaultLogType>(faultLogType % FaultLogType::MAX_TYPE), info);
    std::string eventName;
    (void)source.GetString(eventName, MAX_STR_LEN);
    std::string domain;
    (void)source.GetString(domain, MAX_STR_LEN);
    SysEventCreator creator(domain, eventName, SysEventCreator::FAULT);
    creator.SetKeyValue("name_", eventName);
    auto sysEvent = std::make_shared<SysEvent>("desc", nullptr, creator);
    auto event = std::dynamic_pointer_cast<Event>(sysEvent);
    (void)interface->ProcessFaultLogEvent(eventName, event);
    interface->SanitizerHandleUnorderedEvent(*event);
}

void FuzzGwpAsan(FaultloggerInterface* interface, FuzzDataSource& source)
{
    GwpAsanParams params;
    (void)source.GetValue(params.alwaysEnabled);
    (void)source.GetValue(params.isRecover);
    (void)source.GetValue(params.sampleRate);
    (void)source.GetValue(params.maxSimutaneousAllocations);
    (void)source.GetValue(params.duration);
    int32_t uid = 0;
    (void)source.GetValue(uid);
    (void)interface->EnableGwpAsanGrayscale(params, uid);
    interface->DisableGwpAsanGrayscale(uid);
    (void)interface->GetGwpAsanGrayscaleState(uid);
    std::string processName;
    (void)source.GetString(processName, MAX_STR_LEN);
    (void)interface->EnableGwpAsanInner(params, processName);
}
}

void FuzzFaultlogBaseInterface(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    auto* interface = NewFaultloggerInterface();
    if (interface == nullptr) {
        return;
    }
    FuzzDumpAndQuery(interface, source);
    FuzzAddAndProcessEvent(interface, source);
    FuzzGwpAsan(interface, source);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFaultlogBaseInterface(data, size);
    return 0;
}
