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
#include <string>

#include "freeze_json_generator.h"
#include "freezejsonparamsextra_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;

std::string ReadStr(FuzzDataSource& source)
{
    std::string val;
    (void)source.GetString(val, MAX_STR_LEN);
    return val;
}

void FuzzExtraMemory(FuzzDataSource& source)
{
    uint64_t sysAvailMem = 0;
    uint64_t sysTotalMem = 0;
    uint64_t vmHeapTotalSize = 0;
    uint64_t vmHeapUsedSize = 0;
    uint64_t vmHeapSharedSize = 0;
    (void)source.GetValue(sysAvailMem);
    (void)source.GetValue(sysTotalMem);
    (void)source.GetValue(vmHeapTotalSize);
    (void)source.GetValue(vmHeapUsedSize);
    (void)source.GetValue(vmHeapSharedSize);
    auto memory = FreezeJsonMemory::Builder()
        .InitSysAvailMem(sysAvailMem)
        .InitSysTotalMem(sysTotalMem)
        .InitVmHeapTotalSize(vmHeapTotalSize)
        .InitVmHeapUsedSize(vmHeapUsedSize)
        .InitVmHeapSharedSize(vmHeapSharedSize)
        .Build();
    (void)memory.JsonStr();
}

void FuzzExtraParams(FuzzDataSource& source)
{
    auto bundleVersion = ReadStr(source);
    auto bundleVersionCode = ReadStr(source);
    uint64_t processLifeTime = 0;
    (void)source.GetValue(processLifeTime);
    auto externalLog = ReadStr(source);
    auto cpuAbi = ReadStr(source);
    auto releaseType = ReadStr(source);
    auto appRunningUniqueId = ReadStr(source);
    auto exception = ReadStr(source);
    auto hilog = ReadStr(source);
    auto eventHandler = ReadStr(source);
    auto eventHandlerSize3s = ReadStr(source);
    auto eventHandlerSize6s = ReadStr(source);
    auto peerBinder = ReadStr(source);
    auto threads = ReadStr(source);
    auto memory = ReadStr(source);
    auto thermalLevel = ReadStr(source);
    auto externalCallbackLog = ReadStr(source);
    auto applicationGCInfo = ReadStr(source);
    auto applicationIOInfo = ReadStr(source);
    auto params = FreezeJsonParams::Builder()
        .InitBundleVersion(bundleVersion)
        .InitBundleVersionCode(bundleVersionCode)
        .InitProcessLifeTime(processLifeTime)
        .InitExternalLog(externalLog)
        .InitCpuAbi(cpuAbi)
        .InitReleaseType(releaseType)
        .InitAppRunningUniqueId(appRunningUniqueId)
        .InitException(exception)
        .InitHilog(hilog)
        .InitEventHandler(eventHandler)
        .InitEventHandlerSize3s(eventHandlerSize3s)
        .InitEventHandlerSize6s(eventHandlerSize6s)
        .InitPeerBinder(peerBinder)
        .InitThreads(threads)
        .InitMemory(memory)
        .InitThermalLevel(thermalLevel)
        .InitExternalCallbackLog(externalCallbackLog)
        .InitApplicationGCInfo(applicationGCInfo)
        .InitApplicationIOInfo(applicationIOInfo)
        .Build();
    (void)params.JsonStr();
}
}

void FuzzFreezeJsonParamsExtra(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    FuzzExtraMemory(source);
    FuzzExtraParams(source);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFreezeJsonParamsExtra(data, size);
    return 0;
}
