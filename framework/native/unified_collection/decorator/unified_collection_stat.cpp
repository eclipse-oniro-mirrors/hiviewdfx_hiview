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

#include "unified_collection_stat.h"

#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "decorator_util.h"

#ifdef UNIFIED_COLLECTOR_CPU_ENABLE
#include "cpu_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_GPU_ENABLE
#include "gpu_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_GRAPHIC_ENABLE
#include "graphic_memory_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
#include "hiebpf_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_HILOG_ENABLE
#include "hilog_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_IO_ENABLE
#include "io_decorator.h"
#endif

#ifdef HAS_HIPROFILER
#include "mem_profiler_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_MEMORY_ENABLE
#include "memory_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
#include "network_decorator.h"
#endif

#ifdef HAS_HIPERF
#include "perf_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_PROCESS_ENABLE
#include "process_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_THERMAL_ENABLE
#include "thermal_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
#include "trace_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_WM_ENABLE
#include "wm_decorator.h"
#endif

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil-UCStat");

std::string UnifiedCollectionStat::date_ = GetCurrentDate();

void UnifiedCollectionStat::Report()
{
    if (date_ == GetCurrentDate()) {
        return;
    }
    HIVIEW_LOGI("date_=%{public}s, curDate=%{public}s", date_.c_str(), GetCurrentDate().c_str());
    SaveAllStatInfo();
    ResetAllStatInfo();
    date_ = GetCurrentDate();
}

void UnifiedCollectionStat::SaveAllStatInfo()
{
    WriteLinesToFile({UC_STAT_DATE, date_}, true, UC_STAT_LOG_PATH);
    WriteLinesToFile({UC_API_STAT_TITLE, UC_API_STAT_ITEM}, false, UC_STAT_LOG_PATH);

#ifdef UNIFIED_COLLECTOR_CPU_ENABLE
    CpuDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_GPU_ENABLE
    GpuDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_GRAPHIC_ENABLE
    GraphicMemoryDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
    HiebpfDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_HILOG_ENABLE
    HilogDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_IO_ENABLE
    IoDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_MEMORY_ENABLE
    MemoryDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
    NetworkDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_PROCESS_ENABLE
    ProcessDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_THERMAL_ENABLE
    ThermalDecorator::SaveStatCommonInfo();
#endif

#ifdef HAS_HIPROFILER
    MemProfilerDecorator::SaveStatCommonInfo();
#endif

#ifdef HAS_HIPERF
    PerfDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_WM_ENABLE
    WmDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
    TraceDecorator::SaveStatSpecialInfo();
#endif

    int32_t ret = HiSysEventWrite(
        HiSysEvent::Domain::HIVIEWDFX,
        "UC_API_STAT",
        HiSysEvent::EventType::FAULT,
        "STAT_DATE", date_);
    if (ret != 0) {
        HIVIEW_LOGW("report collection stat event fail, ret=%{public}d", ret);
    }
}

void UnifiedCollectionStat::ResetAllStatInfo()
{
#ifdef UNIFIED_COLLECTOR_CPU_ENABLE
    CpuDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_GPU_ENABLE
    GpuDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_GRAPHIC_ENABLE
    GraphicMemoryDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
    HiebpfDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_HILOG_ENABLE
    HilogDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_IO_ENABLE
    IoDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_MEMORY_ENABLE
    MemoryDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
    NetworkDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_PROCESS_ENABLE
    ProcessDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_THERMAL_ENABLE
    ThermalDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
    TraceDecorator::ResetStatInfo();
#endif

#ifdef UNIFIED_COLLECTOR_WM_ENABLE
    WmDecorator::ResetStatInfo();
#endif

#ifdef HAS_HIPROFILER
    MemProfilerDecorator::ResetStatInfo();
#endif

#ifdef HAS_HIPERF
    PerfDecorator::ResetStatInfo();
#endif
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
