/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_PLUGINS_UNIFIED_COLLECTOR_INCLUDE_UNIFIED_COLLECTOR_H
#define HIVIEW_PLUGINS_UNIFIED_COLLECTOR_INCLUDE_UNIFIED_COLLECTOR_H

#include <atomic>
#include <memory>

#include "app_caller_event.h"
#include "app_trace_context.h"
#include "cpu_collection_task.h"
#include "plugin.h"
#include "sys_event.h"
#include "trace_cache_monitor.h"
#include "unified_collection_stat.h"

namespace OHOS {
namespace HiviewDFX {
class UnifiedCollector : public Plugin {
public:
    void OnLoad() override;
    void OnUnload() override;
    bool OnEvent(std::shared_ptr<Event>& event) override;
    void OnEventListeningCallback(const Event& event) override;
    void Dump(int fd, const std::vector<std::string>& cmds) override;
    static bool IsEnableRecordTrace() { return isEnableRecordTrace_; }
    static void SetRecordTraceStatus(bool isEnable) { isEnableRecordTrace_ = isEnable; }

private:
    void Init();
    void InitWorkLoop();
    void InitWorkPath();
    void RunCpuCollectionTask();
    void CpuCollectionFfrtTask();
    void RegisterWorker();
    void RunIoCollectionTask();
    void RunUCollectionStatTask();
    void IoCollectionTask();
    void UCollectionStatTask();
    void RunCacheMonitorLoop();
    void ExitCacheMonitorLoop();
    void CleanDataFiles();
    void OnMainThreadJank(SysEvent& sysEvent);
    bool OnStartAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent);
    bool OnStopAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent);
    bool OnDumpAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent);
    void RunRecordTraceTask();
    static void OnSwitchStateChanged(const char* key, const char* value, void* context);

private:
    std::string workPath_;
    std::shared_ptr<CpuCollectionTask> cpuCollectionTask_;
    std::shared_ptr<TraceCacheMonitor> traceCacheMonitor_;
    std::list<uint64_t> taskList_;
    volatile bool isCpuTaskRunning_;
    static bool isEnableRecordTrace_;
    std::shared_ptr<AppTraceContext> appTraceContext_;
}; // UnifiedCollector
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_UNIFIED_COLLECTOR_INCLUDE_UNIFIED_COLLECTOR_H
