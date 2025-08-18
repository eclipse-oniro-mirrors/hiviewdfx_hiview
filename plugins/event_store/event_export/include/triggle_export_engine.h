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

#ifndef HIVIEW_BASE_TRIGGLE_EXPORT_ENGINE_H
#define HIVIEW_BASE_TRIGGLE_EXPORT_ENGINE_H

#include "export_config_manager.h"
#include "ffrt.h"
#include "sys_event.h"
#include "triggle_export_task.h"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <list>

namespace OHOS {
namespace HiviewDFX {
using TriggleTaskList = std::list<std::shared_ptr<TriggleExportTask>>;
class TriggleExportEngine {
public:
    static TriggleExportEngine& GetInstance();
    void ProcessEvent(std::shared_ptr<SysEvent> sysEvent);
    void SetTaskDelayedSecond(int second);

private:
    TriggleExportEngine();
    ~TriggleExportEngine();
    void BuildNewTaskList(std::shared_ptr<SysEvent> event, std::shared_ptr<ExportConfig> config);
    void CancleExportDelay();
    void GetReportIntervalMatchedConfigs(std::vector<std::shared_ptr<ExportConfig>>& configs,
        int16_t eventReportInterval);
    void InitByAllExportConfigs();
    void InitFfrtQueueRefer();
    void RebuildExistTaskList(TriggleTaskList& taskList, std::shared_ptr<SysEvent> event,
        std::shared_ptr<ExportConfig> config);
    void RemoveTask(std::shared_ptr<TriggleExportTask> task);
    void StartTask(std::shared_ptr<TriggleExportTask> task);

private:
    ffrt_queue_t runningTaskQueue_;
    ffrt_task_attr_t taskAttr_;
    ffrt::mutex taskMapMutex_;
    ffrt::mutex delayMutex_;
    std::unordered_map<std::string, TriggleTaskList> taskMap_;
    bool isTaskNeedDelay_ = true;
    int taskDelaySecond_ = 180;
    std::vector<std::shared_ptr<ExportConfig>> exportConfigs_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_TRIGGLE_EXPORT_ENGINE_H