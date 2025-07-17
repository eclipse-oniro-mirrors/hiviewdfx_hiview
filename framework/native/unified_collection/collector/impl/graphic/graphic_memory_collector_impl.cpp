/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "graphic_memory_collector_impl.h"

#include <dlfcn.h>

#include "graphic_memory_decorator.h"
#include "hiview_logger.h"
#include "memory.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("GraphicCollector");
constexpr const char *LIB_NAME = "libucollection_graphic.z.so";
constexpr const char *GET_INSTANCE = "GetInstance";
}
std::shared_ptr<GraphicMemoryCollector> GraphicMemoryCollector::Create()
{
    return std::make_shared<GraphicMemoryDecorator>(std::make_shared<GraphicMemoryCollectorImpl>());
}

CollectResult<int32_t> GraphicMemoryCollectorImpl::GetGraphicUsage(int32_t pid, GraphicType type,
    bool isLowLatencyMode)
{
    std::lock_guard<std::mutex> lock(mutexLock_);
    static GraphicMemoryCollector *graphCollectorInstance = nullptr;
    if (graphCollectorInstance != nullptr) {
        return graphCollectorInstance->GetGraphicUsage(pid, type, false);
    }

    CollectResult<int32_t> result;
    void *handler = dlopen(LIB_NAME, RTLD_LAZY);
    if (handler == nullptr) {
        HIVIEW_LOGW("dlopen failed, error: %{public}s", dlerror());
        return result;
    }

    auto getInterface = reinterpret_cast<GraphicMemoryCollector *(*)()>(dlsym(handler, GET_INSTANCE));
    if (getInterface == nullptr) {
        HIVIEW_LOGW("dlsym failed, error: %{public}s", dlerror());
        dlclose(handler);
        return result;
    }
    graphCollectorInstance = getInterface();
    result = graphCollectorInstance->GetGraphicUsage(pid, type, false);
    if (!isLowLatencyMode) {
        dlclose(handler);
        handler = nullptr;
        graphCollectorInstance = nullptr;
    }
    return result;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
