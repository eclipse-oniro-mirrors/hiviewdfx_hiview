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
#include "freezejsongenerator_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;

void FuzzException(FuzzDataSource& source)
{
    std::string name;
    std::string message;
    if (!source.GetString(name, MAX_STR_LEN) || !source.GetString(message, MAX_STR_LEN)) {
        return;
    }
    auto exception = FreezeJsonException::Builder()
                         .InitName(name)
                         .InitMessage(message)
                         .Build();
    (void)exception.JsonStr();
}

void FuzzMemory(FuzzDataSource& source)
{
    uint64_t rss = 0;
    uint64_t pss = 0;
    uint64_t vss = 0;
    uint64_t sysFreeMem = 0;
    (void)source.GetValue(rss);
    (void)source.GetValue(pss);
    (void)source.GetValue(vss);
    (void)source.GetValue(sysFreeMem);
    auto memory = FreezeJsonMemory::Builder()
                      .InitRss(rss)
                      .InitPss(pss)
                      .InitVss(vss)
                      .InitSysFreeMem(sysFreeMem)
                      .Build();
    (void)memory.JsonStr();
}

void FuzzParams(FuzzDataSource& source)
{
    unsigned long long timeVal = 0;
    (void)source.GetValue(timeVal);
    std::string uuid;
    (void)source.GetString(uuid, MAX_STR_LEN);
    std::string freezeType;
    (void)source.GetString(freezeType, MAX_STR_LEN);
    bool foreground = false;
    (void)source.GetValue(foreground);
    std::string bundleName;
    (void)source.GetString(bundleName, MAX_STR_LEN);
    std::string processName;
    (void)source.GetString(processName, MAX_STR_LEN);
    long pid = 0;
    (void)source.GetValue(pid);
    long uid = 0;
    (void)source.GetValue(uid);
    auto params = FreezeJsonParams::Builder()
                      .InitTime(timeVal)
                      .InitUuid(uuid)
                      .InitFreezeType(freezeType)
                      .InitForeground(foreground)
                      .InitBundleName(bundleName)
                      .InitProcessName(processName)
                      .InitPid(pid)
                      .InitUid(uid)
                      .Build();
    (void)params.JsonStr();
}
}

void FuzzFreezeJsonGenerator(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    FuzzException(source);
    FuzzMemory(source);
    FuzzParams(source);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFreezeJsonGenerator(data, size);
    return 0;
}
