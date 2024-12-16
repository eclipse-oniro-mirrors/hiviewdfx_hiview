/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_GRAPHIC_MEMORY_COLLECTOR_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_GRAPHIC_MEMORY_COLLECTOR_H
#include <memory>

#include "collect_result.h"
#include "resource/memory.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class GraphicMemoryCollector {
public:
    GraphicMemoryCollector() = default;
    virtual ~GraphicMemoryCollector() = default;

public:
    static std::shared_ptr<GraphicMemoryCollector> Create();
    virtual CollectResult<int32_t> GetGraphicUsage(int32_t pid, GraphicType type = GraphicType::TOATL) = 0;
}; // GraphicMemoryCollector
} // UCollectUtil
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_GRAPHIC_MEMORY_COLLECTOR_H
