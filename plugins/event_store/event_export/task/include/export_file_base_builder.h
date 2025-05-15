/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EXPORT_FILE_BASE_BUILDER_H
#define HIVIEW_BASE_EVENT_EXPORT_EXPORT_FILE_BASE_BUILDER_H

#include <string>
#include <vector>
#include <unordered_map>

#include "cached_event.h"

namespace OHOS {
namespace HiviewDFX {
// <domain, <name, list if eventJsonStr>>
using CachedEventMap = std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>;
class ExportFileBaseBuilder {
public:
    ExportFileBaseBuilder() = default;
    virtual ~ExportFileBaseBuilder() = default;
    virtual bool Build(const CachedEventMap& eventMap, std::string& buildStr) = 0;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EXPORT_FILE_BASE_BUILDER_H