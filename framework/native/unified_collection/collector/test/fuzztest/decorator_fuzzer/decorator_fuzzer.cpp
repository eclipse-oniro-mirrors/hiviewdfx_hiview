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

#include "decorator_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>

#include "collect_result.h"
#include "decorator.h"
#include "decorator_util.h"
#include "fuzz_data_source.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
constexpr size_t BINARY_MOD = 2;
constexpr size_t MAX_STR_LEN = 100;

// cover the int-return branch of UCDecorator::Invoke, which is
// implemented in decorator.cpp
int FuzzIntTask(int32_t seed)
{
    return seed % static_cast<int32_t>(BINARY_MOD);
}

// cover the CollectResult-return branch of UCDecorator::Invoke
CollectResult<int32_t> FuzzResultTask(int32_t seed)
{
    CollectResult<int32_t> result;
    result.retCode = (seed % static_cast<int32_t>(BINARY_MOD) == 0) ?
        UCollect::UcError::SUCCESS : UCollect::UcError::SYSTEM_ERROR;
    result.data = seed;
    return result;
}
}

static void StatInfoWrapperFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    uint32_t statValue = 0;
    (void)source.GetValue(statValue);
    uint32_t callCount = 0;
    (void)source.GetValue(callCount);
    std::string funcName;
    (void)source.GetString(funcName, MAX_STR_LEN);
    StatInfoWrapper statInfoWrapper;
    statInfoWrapper.UpdateStatInfo(statValue, callCount, funcName, true);
    statInfoWrapper.UpdateStatInfo(statValue, callCount, funcName, false);
    auto statInfos = statInfoWrapper.GetStatInfo();
    for (const auto& [name, statInfo] : statInfos) {
        (void)statInfo.ToString();
    }
    statInfoWrapper.ResetStatInfo();
}

static void UcDecoratorInvokeFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    std::string funcName;
    (void)source.GetString(funcName, MAX_STR_LEN);
    StatInfoWrapper statInfoWrapper;
    UCDecorator decorator;
    int32_t seed = 0;
    (void)source.GetValue(seed);
    (void)decorator.Invoke([seed] { return FuzzIntTask(seed); }, statInfoWrapper, funcName);
    (void)decorator.Invoke([seed] { return FuzzResultTask(seed); }, statInfoWrapper, funcName);
    statInfoWrapper.ResetStatInfo();
}

static void DecoratorUtilFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    std::string strData;
    (void)source.GetString(strData, MAX_STR_LEN);
    (void)GetCurrentDate();
    std::list<std::string> stats = { strData, strData };
    uint8_t appendMode = 0;
    (void)source.GetValue(appendMode);
    // write to a fixed temp file to avoid arbitrary file access driven by fuzz input
    WriteLinesToFile(stats, (appendMode % BINARY_MOD == 0), "/data/local/tmp/hiview_decorator_fuzz_test");
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::UCollectUtil::StatInfoWrapperFuzzTest(data, size);
    OHOS::HiviewDFX::UCollectUtil::UcDecoratorInvokeFuzzTest(data, size);
    OHOS::HiviewDFX::UCollectUtil::DecoratorUtilFuzzTest(data, size);
    return 0;
}
