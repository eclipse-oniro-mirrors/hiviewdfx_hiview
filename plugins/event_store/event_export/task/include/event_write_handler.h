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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EXPORTED_EVENT_WRITE_HANDLER_H
#define HIVIEW_BASE_EVENT_EXPORT_EXPORTED_EVENT_WRITE_HANDLER_H

#include <functional>
#include <list>
#include <tuple>
#include <unordered_map>

#include "export_base_handler.h"
#include "export_db_storage.h"
#include "export_json_file_writer.h"

namespace OHOS {
namespace HiviewDFX {
struct CachedEvent {
    // event version
    EventVersion version;

    // event domain
    std::string domain;

    // event name
    std::string name;

    // event json string
    std::string eventStr;

    CachedEvent(EventVersion& version, std::string& domain, std::string& name, std::string& eventStr)
        : version(version), domain(domain), name(name), eventStr(eventStr) {}
};

struct EventWriteRequest : public BaseRequest {
    // name of export module
    std::string moduleName;

    // item: <system version, domain, sequecen, sysevent content>
    std::list<std::shared_ptr<CachedEvent>> sysEvents;

    // directory configured for export event file to store
    std::string exportDir;
    
    // tag whether the query is completed
    bool isQueryCompleted = false;

    // max size of a single event file
    int64_t maxSingleFileSize = 0;

    EventWriteRequest(std::string& moduleName, std::list<std::shared_ptr<CachedEvent>>& sysEvents,
        std::string& exportDir, bool isQueryCompleted, int64_t maxSingleFileSize)
        : moduleName(moduleName), sysEvents(sysEvents), exportDir(exportDir), isQueryCompleted(isQueryCompleted),
        maxSingleFileSize(maxSingleFileSize) {}
};

class EventWriteHandler : public ExportBaseHandler {
public:
    bool HandleRequest(RequestPtr req) override;

private:
    std::shared_ptr<ExportJsonFileWriter> GetEventWriter(const EventVersion& eventVersion,
        std::shared_ptr<EventWriteRequest> writeReq);
    void CopyTmpZipFilesToDest();
    void Rollback();

private:
    // <key: <module name, event version>, value: writer>
    std::map<std::pair<std::string, std::string>, std::shared_ptr<ExportJsonFileWriter>> allJsonFileWriters_;
    // <tmpZipFilePath, destZipFilePath>
    std::unordered_map<std::string, std::string> zippedExportFileMap_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EXPORTED_EVENT_WRITE_HANDLER_H