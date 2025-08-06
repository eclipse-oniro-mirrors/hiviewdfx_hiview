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

#include "io_decorator.h"
#include "decorator_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
constexpr char IO_COLLECTOR_NAME[] = "IoCollector";
StatInfoWrapper IoDecorator::statInfoWrapper_;

CollectResult<ProcessIo> IoDecorator::CollectProcessIo(int32_t pid)
{
    auto task = [this, &pid] { return ioCollector_->CollectProcessIo(pid); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::string> IoDecorator::CollectRawDiskStats()
{
    auto task = [this] { return ioCollector_->CollectRawDiskStats(); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::vector<DiskStats>> IoDecorator::CollectDiskStats(
    DiskStatsFilter filter, bool isUpdate)
{
    auto task = [this, &filter, &isUpdate] { return ioCollector_->CollectDiskStats(filter, isUpdate); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::string> IoDecorator::ExportDiskStats(DiskStatsFilter filter)
{
    auto task = [this, &filter] { return ioCollector_->ExportDiskStats(filter); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::vector<EMMCInfo>> IoDecorator::CollectEMMCInfo()
{
    auto task = [this] { return ioCollector_->CollectEMMCInfo(); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::string> IoDecorator::ExportEMMCInfo()
{
    auto task = [this] { return ioCollector_->ExportEMMCInfo(); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::vector<ProcessIoStats>> IoDecorator::CollectAllProcIoStats(bool isUpdate)
{
    auto task = [this, &isUpdate] { return ioCollector_->CollectAllProcIoStats(isUpdate); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::string> IoDecorator::ExportAllProcIoStats()
{
    auto task = [this] { return ioCollector_->ExportAllProcIoStats(); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<SysIoStats> IoDecorator::CollectSysIoStats()
{
    auto task = [this] { return ioCollector_->CollectSysIoStats(); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::string> IoDecorator::ExportSysIoStats()
{
    auto task = [this] { return ioCollector_->ExportSysIoStats(); };
    return Invoke(task, statInfoWrapper_, std::string(IO_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

void IoDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::list<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false, UC_STAT_LOG_PATH);
}

void IoDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
