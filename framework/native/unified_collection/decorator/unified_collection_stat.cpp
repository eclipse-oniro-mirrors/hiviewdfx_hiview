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
#include "time_util.h"

#include "cpu_decorator.h"
#include "gpu_decorator.h"
#include "graphic_memory_decorator.h"

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
#include "hiebpf_decorator.h"
#endif

#include "hilog_decorator.h"
#include "io_decorator.h"
#ifdef HAS_HIPROFILER
#include "mem_profiler_decorator.h"
#endif
#include "memory_decorator.h"

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
#include "network_decorator.h"
#endif

#ifdef HAS_HIPERF
#include "perf_decorator.h"
#endif

#include "process_decorator.h"
#include "thermal_decorator.h"
#include "trace_decorator.h"
#include "wm_decorator.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil-UCStat");
constexpr const char* const UC_STAT_DATE = "Date:";
constexpr const char* const UC_API_STAT_TITLE = "API statistics:";
constexpr const char* const UC_API_STAT_ITEM =
    "API TotalCall FailCall AvgLatency(us) MaxLatency(us) TotalTimeSpent(us)";

namespace {
std::string GetCurrentDate()
{
    return TimeUtil::TimestampFormatToDate(std::time(nullptr), "%Y-%m-%d");
}
}

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
    UCDecorator::WriteLinesToFile({UC_STAT_DATE, date_}, true);
    UCDecorator::WriteLinesToFile({UC_API_STAT_TITLE, UC_API_STAT_ITEM}, false);
    CpuDecorator::SaveStatCommonInfo();
    GpuDecorator::SaveStatCommonInfo();
    GraphicMemoryDecorator::SaveStatCommonInfo();

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
    HiebpfDecorator::SaveStatCommonInfo();
#endif

    HilogDecorator::SaveStatCommonInfo();
    IoDecorator::SaveStatCommonInfo();
    MemoryDecorator::SaveStatCommonInfo();

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
    NetworkDecorator::SaveStatCommonInfo();
#endif

    ProcessDecorator::SaveStatCommonInfo();
    ThermalDecorator::SaveStatCommonInfo();
    TraceDecorator::SaveStatCommonInfo();
#ifdef HAS_HIPROFILER
    MemProfilerDecorator::SaveStatCommonInfo();
#endif

#ifdef HAS_HIPERF
    PerfDecorator::SaveStatCommonInfo();
#endif

    WmDecorator::SaveStatCommonInfo();

    TraceDecorator::SaveStatSpecialInfo();

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
    CpuDecorator::ResetStatInfo();
    GpuDecorator::ResetStatInfo();
    GraphicMemoryDecorator::ResetStatInfo();

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
    HiebpfDecorator::ResetStatInfo();
#endif

    HilogDecorator::ResetStatInfo();
    IoDecorator::ResetStatInfo();
    MemoryDecorator::ResetStatInfo();

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
    NetworkDecorator::ResetStatInfo();
#endif

    ProcessDecorator::ResetStatInfo();
    ThermalDecorator::ResetStatInfo();
    TraceDecorator::ResetStatInfo();
    WmDecorator::ResetStatInfo();
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
