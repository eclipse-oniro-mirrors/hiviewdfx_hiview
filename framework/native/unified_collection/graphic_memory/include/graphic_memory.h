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

#ifndef HIVIEWDFX_HIVIEW_GRAPHIC_MEMORY_GRAPHIC_MEMORY_H
#define HIVIEWDFX_HIVIEW_GRAPHIC_MEMORY_GRAPHIC_MEMORY_H

#include <cstdint>
#include <map>
#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace Graphic {
enum class Type {
    TOTAL,
    GL,
    GRAPH,
};

enum class ResultCode {
    SUCCESS,
    FAIL,
};

struct CollectResult {
    ResultCode retCode = ResultCode::FAIL;
    int32_t graphicData = 0;
};

CollectResult GetGraphicUsage(int32_t pid, Type type = Type::TOTAL);
} // namespace Graphic
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEWDFX_HIVIEW_GRAPHIC_MEMORY_GRAPHIC_MEMORY_H
