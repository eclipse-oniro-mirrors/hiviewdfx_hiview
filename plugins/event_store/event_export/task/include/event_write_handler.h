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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EVENT_WRITE_HANDLER_H
#define HIVIEW_BASE_EVENT_EXPORT_EVENT_WRITE_HANDLER_H

#include <list>

#include "cached_event.h"
#include "export_base_handler.h"
#include "export_db_storage.h"
#include "export_event_packager.h"

namespace OHOS {
namespace HiviewDFX {
struct EventWriteRequest : public BaseRequest {
    // name of export module
    std::string moduleName;

    // item: <system version, domain, sequecen, sysevent content>
    std::list<std::shared_ptr<CachedEvent>> events;

    // directory configured for export event file to store
    std::string exportDir;
    
    // tag whether the query is completed
    bool isQueryCompleted = false;

    // max size of a single event file
    int64_t maxSingleFileSize = 0;

    EventWriteRequest(std::string& moduleName, std::list<std::shared_ptr<CachedEvent>>& events,
        std::string& exportDir, bool isQueryCompleted, int64_t maxSingleFileSize)
        : moduleName(moduleName), events(events), exportDir(exportDir),
        isQueryCompleted(isQueryCompleted), maxSingleFileSize(maxSingleFileSize) {}
};

class EventWriteHandler : public ExportBaseHandler {
public:
    bool HandleRequest(RequestPtr req) override;

private:
    std::shared_ptr<ExportEventPackager> GetEventPackager(const std::shared_ptr<CachedEvent> event,
        std::shared_ptr<EventWriteRequest> writeReq);
    void Finish();
    void Rollback();

private:
    std::map<std::string, std::shared_ptr<ExportEventPackager>> packagers_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EVENT_WRITE_HANDLER_H