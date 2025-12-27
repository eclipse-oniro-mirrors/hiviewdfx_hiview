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

#include "cpu_collection_task.h"
#include "plugin.h"
#include "sys_event.h"
#ifdef HIVIEW_LOW_MEM_THRESHOLD
#include "trace_cache_monitor.h"
#endif
#include "unified_collection_stat.h"

namespace OHOS {
namespace HiviewDFX {
class UnifiedCollector : public Plugin {
public:
    void OnLoad() override;
    void OnUnload() override;
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
    void OnEventListeningCallback(const Event& event) override;
    void Dump(int fd, const std::vector<std::string>& cmds) override;
#endif

private:
    void Init();
    void InitWorkPath();
    void RunCpuCollectionTask();
    void CpuCollectionFfrtTask();
    void RunIoCollectionTask();
    void RunUCollectionStatTask();
    void IoCollectionTask();
    void UCollectionStatTask();
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
    void LoadTraceSwitch();
    static void OnFreezeDetectorParamChanged(const char* key, const char* value, void* context);
    static void OnSwitchRecordTraceStateChanged(const char* key, const char* value, void* context);
    void OnMainThreadJank(SysEvent& sysEvent);
#endif
#ifdef HIVIEW_LOW_MEM_THRESHOLD
    void RunCacheMonitorLoop();
    void ExitCacheMonitorLoop();
#endif
    void CleanDataFiles();

    static void OnSwitchStateChanged(const char* key, const char* value, void* context);

private:
    std::string workPath_;
    std::shared_ptr<CpuCollectionTask> cpuCollectionTask_;
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
    std::shared_ptr<EventListener> telemetryListener_;
#endif
#ifdef HIVIEW_LOW_MEM_THRESHOLD
    std::shared_ptr<TraceCacheMonitor> traceCacheMonitor_;
#endif
    std::list<uint64_t> taskList_;
}; // UnifiedCollector
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_UNIFIED_COLLECTOR_INCLUDE_UNIFIED_COLLECTOR_H
