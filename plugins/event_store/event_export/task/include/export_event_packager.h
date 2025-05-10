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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EVENT_PACAKGER_H
#define HIVIEW_BASE_EVENT_EXPORT_EVENT_PACAKGER_H

#include <functional>
#include <string>
#include <unordered_map>

#include "cached_event.h"
#include "export_file_base_builder.h"

namespace OHOS {
namespace HiviewDFX {
class ExportEventPackager {
public:
    ExportEventPackager(const std::string& moduleName, const std::string& exportDir, const EventVersion& eventVersion,
        int32_t uid, int64_t maxFileSize);

    bool AppendEvent(const std::string& domain, const std::string& name, const std::string& eventStr);
    bool Package();
    void ClearPackagedFiles();
    void HandlePackagedFiles();

private:
    void ClearPackagedEvents();

private:
    std::string moduleName_;
    std::string exportDir_;
    EventVersion eventVersion_;
    uint64_t maxFileSize_ = 0;
    uint64_t totalJsonStrSize_ = 0;
    int32_t uid_ = 0;
    // <domain, <name, eventContentString>>
    CachedEventMap packagedEvents_;
    // <tmpZipFilePath, destZipFilePath>
    std::unordered_map<std::string, std::string> packagedFiles_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EVENT_PACAKGER_H