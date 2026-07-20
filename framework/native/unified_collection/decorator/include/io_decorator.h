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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_IO_DECORATOR_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_IO_DECORATOR_H

#include <mutex>

#include "io_collector.h"
#include "decorator.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class IoDecorator : public IoCollector, public UCDecorator {
public:
    IoDecorator(std::shared_ptr<IoCollector> collector) : ioCollector_(collector) {};
    virtual ~IoDecorator() = default;

public:
    virtual CollectResult<ProcessIo> CollectProcessIo(int32_t pid) override;
    virtual CollectResult<std::vector<DiskStats>> CollectDiskStats(
        DiskStatsFilter filter = DefaultDiskStatsFilter, bool isUpdate = false) override;
    virtual CollectResult<std::vector<EMMCInfo>> CollectEMMCInfo() override;
    virtual CollectResult<std::vector<ProcessIoStats>> CollectAllProcIoStats(bool isUpdate = false) override;
    virtual CollectResult<SysIoStats> CollectSysIoStats() override;
    static void SaveStatCommonInfo();
    static void ResetStatInfo();

private:
    std::shared_ptr<IoCollector> ioCollector_;
    static StatInfoWrapper statInfoWrapper_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_IO_DECORATOR_H
