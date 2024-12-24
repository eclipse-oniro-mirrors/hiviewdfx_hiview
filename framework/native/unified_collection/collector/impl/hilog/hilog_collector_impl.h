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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_HILOG_COLLECTOR_IMPL_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_HILOG_COLLECTOR_IMPL_H

#include "hilog_collector.h"

namespace OHOS::HiviewDFX::UCollectUtil {
class HilogCollectorImpl : public HilogCollector {
public:
    HilogCollectorImpl() = default;
    virtual ~HilogCollectorImpl() = default;
    virtual CollectResult<std::string> CollectLastLog(uint32_t pid, uint32_t num) override;

private:
    void ExecuteHilog(int32_t pid, uint32_t lineCount, int writeFd) const;
    void ReadHilog(int readFd, std::string& log) const;
};
} // namespace OHOS::HiviewDFX::UCollectUtil
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_HILOG_COLLECTOR_IMPL_H