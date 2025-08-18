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

#include "triggle_export_engine.h"

#include "event_export_util.h"
#include "hiview_logger.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-TriggleExportFlow");
namespace {
constexpr int FIRST_TASK_ID = 1;

int64_t CalculateTimeDuration(int64_t ts1, int64_t ts2)
{
    return (ts1 > ts2) ? (ts1 - ts2) : (ts2 - ts1);
}
}

TriggleExportEngine& TriggleExportEngine::GetInstance()
{
    static TriggleExportEngine instance;
    return instance;
}

void TriggleExportEngine::ProcessEvent(std::shared_ptr<SysEvent> sysEvent)
{
    if (sysEvent == nullptr) {
        HIVIEW_LOGE("triggle event is null");
        return;
    }
    ffrt::submit([this, sysEvent] () {
            std::vector<std::shared_ptr<ExportConfig>> configs;
            GetReportIntervalMatchedConfigs(configs, sysEvent->GetReportInterval());
            std::unique_lock<ffrt::mutex> lock(taskMapMutex_);
            for (auto& config : configs) {
                auto iter = taskMap_.find(config->moduleName);
                if (iter == taskMap_.end()) {
                    BuildNewTaskList(sysEvent, config);
                } else {
                    RebuildExistTaskList(iter->second, sysEvent, config);
                }
            }
        }, { }, {}, ffrt::task_attr().name("process_triggle_event").qos(ffrt::qos_default));
}

void TriggleExportEngine::SetTaskDelayedSecond(int second)
{
    taskDelaySecond_ = second;
}

TriggleExportEngine::TriggleExportEngine()
{
    ExportConfigManager::GetInstance().GetTriggleExportConfigs(exportConfigs_);
    HIVIEW_LOGI("total count of triggle config is %{public}zu", exportConfigs_.size());
    if (exportConfigs_.empty()) {
        return;
    }

    ffrt::submit([this] () {
            InitFfrtQueueRefer();
            InitByAllExportConfigs();
        }, { }, {}, ffrt::task_attr().name("init_triggle_export").qos(ffrt::qos_default));

    // delay 3 mins to export
    ffrt::submit([this] () {
            ffrt::this_task::sleep_for(std::chrono::seconds(taskDelaySecond_));
            CancleExportDelay();
        }, {}, {}, ffrt::task_attr().name("triggle_export_delay_cancel").qos(ffrt::qos_default));
}

TriggleExportEngine::~TriggleExportEngine()
{
    for (auto& config : exportConfigs_) {
        EventExportUtil::UnregisterSettingObserver(config);
    }

    if (runningTaskQueue_ != nullptr) {
        ffrt_queue_destroy(runningTaskQueue_);
    }
}

void TriggleExportEngine::RebuildExistTaskList(TriggleTaskList& taskList, std::shared_ptr<SysEvent> event,
    std::shared_ptr<ExportConfig> config)
{
    std::shared_ptr<TriggleExportTask> lastTask = nullptr;
    if (!taskList.empty()) {
        lastTask = taskList.back();
    }
    if ((lastTask == nullptr) || (CalculateTimeDuration(lastTask->GetTimeStamp(), event->happenTime_) >
        config->taskTriggleCycle * TimeUtil::SEC_TO_MILLISEC)) {
        int newTaskId = (lastTask == nullptr) ? FIRST_TASK_ID : lastTask->GetId() + 1;
        auto newTask = std::make_shared<TriggleExportTask>(config, newTaskId);
        newTask->AppendEvent(event);
        taskList.emplace_back(newTask);
        StartTask(newTask);
    } else {
        lastTask->AppendEvent(event);
    }
}

void TriggleExportEngine::BuildNewTaskList(std::shared_ptr<SysEvent> event, std::shared_ptr<ExportConfig> config)
{
    std::list<std::shared_ptr<TriggleExportTask>> taskList;
    auto newTask = std::make_shared<TriggleExportTask>(config, FIRST_TASK_ID);
    newTask->AppendEvent(event);
    taskList.emplace_back(newTask);
    taskMap_.insert(std::make_pair(config->moduleName, taskList));
    StartTask(newTask);
}

void TriggleExportEngine::CancleExportDelay()
{
    HIVIEW_LOGI("cancel triggle export delay");
    {
        std::unique_lock<ffrt::mutex> lock(delayMutex_);
        isTaskNeedDelay_ = false;
    }
    std::unique_lock<ffrt::mutex> lock(taskMapMutex_);
    for (auto& taskItem : taskMap_) {
        auto& allTaskInSameModule = taskItem.second;
        if (allTaskInSameModule.empty()) {
            continue;
        }
        for (auto& task : allTaskInSameModule) {
            StartTask(task);
        }
    }
}

void TriggleExportEngine::StartTask(std::shared_ptr<TriggleExportTask> task)
{
    {
        std::unique_lock<ffrt::mutex> lock(delayMutex_);
        if (isTaskNeedDelay_ || runningTaskQueue_ == nullptr) {
            return;
        }
    }
    std::function<void()>&& taskFunc = [this, task] () {
        if (task == nullptr) {
            return;
        }
        ffrt::this_task::sleep_for(task->GetTriggleCycle());
        task->Run();

        // remove task cache from list
        RemoveTask(task);
    };
    ffrt_queue_submit(runningTaskQueue_, ffrt::create_function_wrapper(taskFunc, ffrt_function_kind_queue), &taskAttr_);
}

void TriggleExportEngine::RemoveTask(std::shared_ptr<TriggleExportTask> task)
{
    std::unique_lock<ffrt::mutex> lock(taskMapMutex_);
    auto mapIter = taskMap_.find(task->GetModuleName());
    if (mapIter == taskMap_.end()) {
        return;
    }
    auto& taskList = mapIter->second;
    for (auto iter = taskList.begin(); iter != taskList.end(); ++iter) {
        if ((*iter)->GetId() == task->GetId()) {
            taskList.erase(iter);
            break;
        }
    }
}

void TriggleExportEngine::InitByAllExportConfigs()
{
    std::unique_lock<ffrt::mutex> lock(taskMapMutex_);
    for (auto& config : exportConfigs_) {
        // register setting observer
        EventExportUtil::RegisterSettingObserver(config);

        // init task by config
        auto mapIter = taskMap_.find(config->moduleName);
        if (mapIter == taskMap_.end()) {
            std::list<std::shared_ptr<TriggleExportTask>> taskList;
            auto task = std::make_shared<TriggleExportTask>(config, FIRST_TASK_ID);
            taskList.emplace_back(task);
            taskMap_.insert(std::make_pair(config->moduleName, taskList));
        }
    }
}

void TriggleExportEngine::InitFfrtQueueRefer()
{
    // init ffrt task queue
    ffrt_queue_attr_t queueAttr;
    (void)ffrt_queue_attr_init(&queueAttr);
    runningTaskQueue_ = ffrt_queue_create(ffrt_queue_serial, "triggle_export_engine", &queueAttr);

    // init ffrt task
    ffrt_task_attr_init(&taskAttr_);
    ffrt_task_attr_set_qos(&taskAttr_, ffrt_qos_user_initiated);
}

void TriggleExportEngine::GetReportIntervalMatchedConfigs(std::vector<std::shared_ptr<ExportConfig>>& configs,
    int16_t eventReportInterval)
{
    for (auto& config : exportConfigs_) {
        if (config->taskType != eventReportInterval) {
            continue;
        }
        configs.emplace_back(config);
    }
}
} // namespace HiviewDFX
} // namespace OHOS