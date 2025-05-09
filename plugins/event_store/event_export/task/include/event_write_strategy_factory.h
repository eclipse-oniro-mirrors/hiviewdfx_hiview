/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_EVENT_WRITE_BASE_STRAGEGY_H
#define HIVIEW_BASE_EVENT_WRITE_BASE_STRAGEGY_H

#include <string>
#include <functional>

#include "cached_event.h"

namespace OHOS {
namespace HiviewDFX {
struct WriteStrategyParam {
    std::string moduleName;
    std::string exportDir;
    EventVersion version;
    int32_t uid = 0;
};

using WroteCallback = std::function<void(const std::string&, const std::string&)>;
class EventWriteBaseStrategy {
public:
    EventWriteBaseStrategy() = default;
    virtual ~EventWriteBaseStrategy() = default;

    virtual void SetWriteStrategyParam(WriteStrategyParam& param);

    virtual std::string GetPackagerKey(std::shared_ptr<CachedEvent> cachedEvent);
    virtual bool Write(std::string& exportContent, WroteCallback callback);

protected:
    WriteStrategyParam param_;
};

enum StrategyType {
    ZIP_JSON_FILE,
};

class EventWriteStrategyFactory {
public:
    static std::shared_ptr<EventWriteBaseStrategy> GetWriteStrategy(StrategyType type);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_BASE_STRAGEGY_H