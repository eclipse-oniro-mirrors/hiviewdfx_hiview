/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_IO_COLLECTOR_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_IO_COLLECTOR_H
#include <cinttypes>
#include <functional>
#include <memory>
#include <vector>

#include "collect_result.h"
#include "resource/io.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
using DiskStatsFilter = std::function<bool(const DiskStats &)>;

class IoCollector {
public:
    IoCollector() = default;
    virtual ~IoCollector() = default;

public:
    virtual CollectResult<ProcessIo> CollectProcessIo(int32_t pid) = 0;
    virtual CollectResult<std::string> CollectRawDiskStats() = 0;
    virtual CollectResult<std::vector<DiskStats>> CollectDiskStats(
        DiskStatsFilter filter = DefaultDiskStatsFilter, bool isUpdate = false) = 0;
    virtual CollectResult<std::string> ExportDiskStats(DiskStatsFilter filter = DefaultDiskStatsFilter) = 0;
    virtual CollectResult<std::vector<EMMCInfo>> CollectEMMCInfo() = 0;
    virtual CollectResult<std::string> ExportEMMCInfo() = 0;
    virtual CollectResult<std::vector<ProcessIoStats>> CollectAllProcIoStats(bool isUpdate = false) = 0;
    virtual CollectResult<std::string> ExportAllProcIoStats() = 0;
    virtual CollectResult<SysIoStats> CollectSysIoStats() = 0;
    virtual CollectResult<std::string> ExportSysIoStats() = 0;
    static std::shared_ptr<IoCollector> Create();
    static bool DefaultDiskStatsFilter(const DiskStats &stats);
}; // IoCollector
} // UCollectUtil
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_IO_COLLECTOR_H