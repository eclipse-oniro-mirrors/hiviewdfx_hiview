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

#ifndef HIVIEW_BASE_TRIGGER_EXPORT_ENGINE_H
#define HIVIEW_BASE_TRIGGER_EXPORT_ENGINE_H

#include "export_config_manager.h"
#include "ffrt.h"
#include "sys_event.h"
#include "trigger_export_task.h"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <list>

namespace OHOS {
namespace HiviewDFX {
using TriggerTaskList = std::list<std::shared_ptr<TriggerExportTask>>;
class TriggerExportEngine {
public:
    static TriggerExportEngine& GetInstance();
    void ProcessEvent(std::shared_ptr<SysEvent> sysEvent);
    void SetTaskDelayedSecond(int second);

private:
    TriggerExportEngine();
    ~TriggerExportEngine();
    void BuildNewTaskList(std::shared_ptr<SysEvent> event, std::shared_ptr<ExportConfig> config);
    void CancelExportDelay();
    void GetReportIntervalMatchedConfigs(std::vector<std::shared_ptr<ExportConfig>>& configs,
        int16_t eventReportInterval);
    void InitByAllExportConfigs();
    void InitFfrtQueueRefer();
    void RebuildExistTaskList(TriggerTaskList& taskList, std::shared_ptr<SysEvent> event,
        std::shared_ptr<ExportConfig> config);
    void RemoveTask(std::shared_ptr<TriggerExportTask> task);
    void StartTask(std::shared_ptr<TriggerExportTask> task);

private:
    ffrt_queue_t runningTaskQueue_ = nullptr;
    ffrt_task_attr_t taskAttr_ { 0 };
    ffrt::mutex taskMapMutex_;
    ffrt::mutex delayMutex_;
    std::unordered_map<std::string, TriggerTaskList> taskMap_;
    bool isTaskNeedDelay_ = true;
    int taskDelaySecond_ = 180;
    std::vector<std::shared_ptr<ExportConfig>> exportConfigs_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_TRIGGER_EXPORT_ENGINE_H