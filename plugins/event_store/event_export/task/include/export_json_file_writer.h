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

#ifndef HIVIEW_BASE_EVENT_EXPORT_JSON_WRITER_H
#define HIVIEW_BASE_EVENT_EXPORT_JSON_WRITER_H

#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "cJSON.h"
#include "export_db_storage.h"

namespace OHOS {
namespace HiviewDFX {
struct EventVersion {
    // system version
    std::string systemVersion;

    // patch version
    std::string patchVersion;
};

class ExportJsonFileWriter {
public:
    ExportJsonFileWriter(const std::string& moduleName, const EventVersion& eventVersion, const std::string& exportDir,
        int64_t maxFileSize);

public:
    using ExportJsonFileZippedListener = std::function<void(const std::string&, const std::string&)>;
    void SetExportJsonFileZippedListener(ExportJsonFileZippedListener listener);

    void ClearEventCache();

public:
    bool Write();
    bool AppendEvent(const std::string& domain, const std::string& name, const std::string& eventStr);

private:
    std::string moduleName_;
    EventVersion eventVersion_;
    std::string exportDir_;
    int64_t maxFileSize_ = 0;
    int64_t totalJsonStrSize_ = 0;
    ExportJsonFileZippedListener exportJsonFileZippedListener_;
    // <domain, <name, eventContentString>>
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> sysEventMap_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_JSON_WRITER_H