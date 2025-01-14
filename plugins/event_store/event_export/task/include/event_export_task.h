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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EVENT_EXPORT_TASK_H
#define HIVIEW_BASE_EVENT_EXPORT_EVENT_EXPORT_TASK_H

#include <memory>

#include "event_read_handler.h"
#include "event_write_handler.h"
#include "export_base_task.h"
#include "export_event_list_parser.h"

namespace OHOS {
namespace HiviewDFX {
class EventExportTask : public ExportBaseTask {
public:
    EventExportTask(std::shared_ptr<ExportConfig> config, std::shared_ptr<ExportDbManager> dbMgr)
        : ExportBaseTask(config, dbMgr) {}

protected:
    void OnTaskRun() override;

private:
    bool InitReadRequest(std::shared_ptr<EventReadRequest> readReq) const;
    bool ParseExportEventList(ExportEventList& list) const;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EVENT_EXPORT_TASK_H