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

#include "graphic_memory_collector_entity.h"

#include "graphic_memory.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
#ifdef __cplusplus
extern "C" {
#endif

GraphicMemoryCollector *GetInstance()
{
    static GraphicMemoryCollectorEntity instance;
    return &instance;
}

#ifdef __cplusplus
}
#endif

CollectResult<int32_t> GraphicMemoryCollectorEntity::GetGraphicUsage(int32_t pid, GraphicType type,
    bool /* isLowLatencyMode */)
{
    CollectResult<int32_t> result;
    Graphic::CollectResult data;
    switch (type) {
        case GraphicType::TOATL:
            data = Graphic::GetGraphicUsage(pid);
            break;
        case GraphicType::GL:
            data = Graphic::GetGraphicUsage(pid, Graphic::Type::GL);
            break;
        case GraphicType::GRAPH:
            data = Graphic::GetGraphicUsage(pid, Graphic::Type::GRAPH);
            break;
        default:
            return result;
    }
    if (data.retCode == Graphic::ResultCode::SUCCESS) {
        result.retCode = UCollect::SUCCESS;
        result.data = data.graphicData;
    }
    return result;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
