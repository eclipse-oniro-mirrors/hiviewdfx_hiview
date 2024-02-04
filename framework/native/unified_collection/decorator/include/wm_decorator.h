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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_WM_DECORATOR_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_WM_DECORATOR_H

#include <mutex>

#include "decorator.h"
#include "wm_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class WmDecorator : public WmCollector, public UCDecorator {
public:
    WmDecorator(std::shared_ptr<WmCollector> collector) : wmCollector_(collector) {};
    virtual ~WmDecorator() = default;

public:
    virtual CollectResult<std::string> ExportWindowsInfo() override;
    virtual CollectResult<std::string> ExportWindowsMemory() override;
    static void SaveStatCommonInfo();
    static void ResetStatInfo();

private:
    std::shared_ptr<WmCollector> wmCollector_;
    static StatInfoWrapper statInfoWrapper_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_WM_DECORATOR_H
