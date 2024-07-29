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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EVENT_READ_HANDLER_H
#define HIVIEW_BASE_EVENT_EXPORT_EVENT_READ_HANDLER_H

#include <fstream>
#include <functional>
#include <tuple>
#include <unordered_map>

#include "event_write_handler.h"
#include "export_base_handler.h"
#include "export_event_list_parser.h"
#include "sys_event_query.h"

namespace OHOS {
namespace HiviewDFX {
struct EventReadRequest : public BaseRequest {
    // the event sequence start to export
    int64_t beginSeq = 0;

    // the event sequence end to sexport
    int64_t endSeq = 0;

    // name of export module
    std::string moduleName;

    // max size of a single event file
    int64_t maxSize = 0;

    // event config list for query
    ExportEventList eventList;

    // directory configured for export event file to store
    std::string exportDir;
};

class EventReadHandler : public ExportBaseHandler {
public:
    using EventExportedListener = std::function<void(int64_t, int64_t)>;
    void SetEventExportedListener(EventExportedListener listener);

    bool HandleRequest(RequestPtr request) override;

private:
    using QueryCallback = std::function<bool(bool)>;
    bool QuerySysEvent(const int64_t beginSeq, const int64_t endSeq, const ExportEventList& eventList,
        QueryCallback queryCallback);
    bool HandleQueryResult(EventStore::ResultSet& result, QueryCallback queryCallback, const int64_t queryLimit,
        int64_t& totalQueryCnt);
    void RegistExportFilePackagedListener();
    void CopyTmpZipFilesToDest();
    void Rollback();

private:
    std::list<std::shared_ptr<CachedEventItem>> cachedSysEvents_;
    std::string curSysEventVersion_;
    // <tmpZipFilePath, destZipFilePath>
    std::unordered_map<std::string, std::string> zippedEventFileMap_;
    EventExportedListener eventExportedListener_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EVENT_READ_HANDLER_H