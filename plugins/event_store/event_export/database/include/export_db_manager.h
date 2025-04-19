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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EXPORT_DB_MGR_H
#define HIVIEW_BASE_EVENT_EXPORT_EXPORT_DB_MGR_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "export_db_storage.h"

namespace OHOS {
namespace HiviewDFX {
class ExportDbManager {
public:
    ExportDbManager(const std::string& dbStoreDir);
    ~ExportDbManager() = default;

public:
    int64_t GetExportEnabledSeq(const std::string& moduleName);
    int64_t GetExportBeginSeq(const std::string& moduleName);
    void HandleExportSwitchChanged(const std::string& moduleName, int64_t curSeq);
    void HandleExportTaskFinished(const std::string& moduleName, int64_t eventSeq);
    bool IsUnrecordedModule(const std::string& moduleName);
    std::string GetEventInheritFlagPath();

private:
    ExportDetailRecord GetExportDetailRecord(const std::string& moduleName);

private:
    std::shared_ptr<ExportDbStorage> storage_;
    std::string dbStoreDir_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_MODULE_EXPORT_MGR_H