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

#ifndef HIVIEW_UTILITY_HISYSEVENT_UTIL
#define HIVIEW_UTILITY_HISYSEVENT_UTIL

#include <vector>

#include "hisysevent_c.h"
#include "hisysevent.h"

namespace OHOS {
namespace HiviewDFX {
#define PARAM_STR(CONST_STR) const_cast<char*>((CONST_STR).c_str())

#define BUILD_PARAM(KEY, TYPE, MEM_DEF, VAL) \
    { .name = (KEY), .t = (TYPE), .v = { .MEM_DEF = (VAL) }, .arraySize = 0 }

#define BUILD_ARRAY_PARAM(KEY, TYPE, ARR_ITEM_TYPE, VAL) \
    { .name = (KEY), .t = (TYPE), \
    .v = { .array = reinterpret_cast<void*>(const_cast<ARR_ITEM_TYPE*>((VAL).data())) }, \
    .arraySize = (VAL).size() }

HiSysEventEventType TranslateEventType(HiSysEvent::EventType type);
void TranslateStrVector(const std::vector<std::string>& src, std::vector<char*>& dest);
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_UTILITY_HISYSEVENT_UTIL