/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "io_collector_empty_impl.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
std::shared_ptr<IoCollector> IoCollector::Create()
{
    return std::make_shared<IoCollectorEmptyImpl>();
}

CollectResult<ProcessIo> IoCollectorEmptyImpl::CollectProcessIo(int32_t pid)
{
    return CollectResult<ProcessIo>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> IoCollectorEmptyImpl::CollectRawDiskStats()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::vector<DiskStats>> IoCollectorEmptyImpl::CollectDiskStats(DiskStatsFilter filter, bool isUpdate)
{
    return CollectResult<std::vector<DiskStats>>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> IoCollectorEmptyImpl::ExportDiskStats(DiskStatsFilter filter)
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::vector<EMMCInfo>> IoCollectorEmptyImpl::CollectEMMCInfo()
{
    return CollectResult<std::vector<EMMCInfo>>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> IoCollectorEmptyImpl::ExportEMMCInfo()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::vector<ProcessIoStats>> IoCollectorEmptyImpl::CollectAllProcIoStats(bool isUpdate)
{
    return CollectResult<std::vector<ProcessIoStats>>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> IoCollectorEmptyImpl::ExportAllProcIoStats()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<SysIoStats> IoCollectorEmptyImpl::CollectSysIoStats()
{
    return CollectResult<SysIoStats>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> IoCollectorEmptyImpl::ExportSysIoStats()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
