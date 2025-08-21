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

#ifndef HIVIEW_BASE_TRIGGER_EXPORT_TASK_H
#define HIVIEW_BASE_TRIGGER_EXPORT_TASK_H

#include "event_export_task.h"
#include "ffrt.h"
#include "sys_event.h"
#include "trigger_export_event.h"

#include <list>
#include <memory>
#include <mutex>

namespace OHOS {
namespace HiviewDFX {
class TriggerExportTask : public EventExportTask {
public:
    TriggerExportTask(std::shared_ptr<ExportConfig> config, int taskId);

    void AppendEvent(std::shared_ptr<SysEvent> sysEvent);
    std::string GetModuleName();
    int64_t GetTimeStamp();
    int GetId();
    std::chrono::seconds GetTriggerCycle();

protected:
    void OnTaskRun() override;
    bool ParseExportEventList(ExportEventList& list) override;
    int64_t GetExportRangeEndSeq() override;

private:
    int id_ = 0;
    int64_t endSeq_ = 0;
    ffrt::mutex listMutex_;
    std::list<std::shared_ptr<TriggerExportEvent>> allEvent_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_TRIGGER_EXPORT_TASK_H