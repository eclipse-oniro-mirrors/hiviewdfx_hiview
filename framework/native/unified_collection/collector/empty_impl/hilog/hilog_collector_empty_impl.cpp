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

#include "hilog_collector_empty_impl.h"

namespace OHOS::HiviewDFX::UCollectUtil {
std::shared_ptr<HilogCollector> HilogCollector::Create()
{
    return std::make_shared<HilogCollectorEmptyImpl>();
}

CollectResult<std::string> HilogCollectorEmptyImpl::CollectLastLog(uint32_t pid, uint32_t lineNum)
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}
} // namespace OHOS::HiviewDFX::UCollectUtil