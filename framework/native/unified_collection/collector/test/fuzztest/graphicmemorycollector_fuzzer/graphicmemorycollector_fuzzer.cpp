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

#include "graphicmemorycollector_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "fuzz_data_source.h"
#include "graphic_memory_decorator.h"
#include "utility/graphic_memory_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
constexpr size_t BINARY_MOD = 2;
}

static void GraphicMemoryCollectorFuzzTest(const uint8_t* data, size_t size)
{
#if defined(UNIFIED_COLLECTOR_GRAPHIC_ENABLE)
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    int32_t pid = 0;
    (void)source.GetValue(pid);
    uint8_t isLowLatency = 0;
    (void)source.GetValue(isLowLatency);
    bool isLowLatencyMode = (isLowLatency % BINARY_MOD == 0);

    // GraphicMemoryCollector::Create returns a collector decorated by
    // GraphicMemoryDecorator
    auto graphicMemoryCollector = GraphicMemoryCollector::Create();
    if (graphicMemoryCollector == nullptr) {
        return;
    }
    (void)graphicMemoryCollector->GetGraphicUsage(pid, GraphicType::TOTAL, isLowLatencyMode);
    (void)graphicMemoryCollector->GetGraphicUsage(pid, GraphicType::GL, isLowLatencyMode);
    (void)graphicMemoryCollector->GetGraphicUsage(pid, GraphicType::GRAPH, isLowLatencyMode);
    GraphicMemoryDecorator::SaveStatCommonInfo();
    GraphicMemoryDecorator::ResetStatInfo();
#else
    (void)data;
    (void)size;
#endif
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }
    OHOS::HiviewDFX::UCollectUtil::GraphicMemoryCollectorFuzzTest(data, size);
    return 0;
}
