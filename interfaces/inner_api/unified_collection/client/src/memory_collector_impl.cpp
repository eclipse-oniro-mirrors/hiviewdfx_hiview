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

#include "memory_collector.h"
#include "hiview_service_memory_delegate.h"

using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectClient {

class MemoryCollectorImpl : public MemoryCollector {
public:
    MemoryCollectorImpl() = default;
    virtual ~MemoryCollectorImpl() = default;
public:
    CollectResult<int32_t> SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller) override;
    CollectResult<int32_t> GetGraphicUsage() override;
};

std::shared_ptr<MemoryCollector> MemoryCollector::Create()
{
    return std::make_shared<MemoryCollectorImpl>();
}

CollectResult<int32_t> MemoryCollectorImpl::SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller)
{
    return HiViewServiceMemoryDelegate::SetAppResourceLimit(memoryCaller);
}

CollectResult<int32_t> MemoryCollectorImpl::GetGraphicUsage()
{
    return HiViewServiceMemoryDelegate::GetGraphicUsage();
}
}
}
}
