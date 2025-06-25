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

#ifndef HIVIEW_BASE_EVENT_STORE_CONFIG_EVENT_STORE_CONFIG_H
#define HIVIEW_BASE_EVENT_STORE_CONFIG_EVENT_STORE_CONFIG_H

#include <cstdint>
#include <unordered_map>

#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
class EventStoreConfig : public OHOS::DelayedRefSingleton<EventStoreConfig> {
public:
    EventStoreConfig();
    ~EventStoreConfig() {}
    uint32_t GetStoreDay(int eventType);
    uint32_t GetMaxSize(int eventType);
    uint32_t GetMaxFileNum(int eventType);
    uint32_t GetPageSize(int eventType);
    uint32_t GetMaxFileSize(int eventType);

private:
    struct StoreConfig {
        uint32_t storeDay;
        uint32_t pageSize;
        uint32_t maxFileSize;
        uint32_t maxFileNum;
        uint32_t maxSize;
    };
    void Init();
    bool Contain(int eventType);

    std::unordered_map<int, StoreConfig> configMap_;
};
} // EventStore
} // HiviewDFX
} // OHOS
#endif // HIVIEW_BASE_EVENT_STORE_CONFIG_EVENT_STORE_CONFIG_H
