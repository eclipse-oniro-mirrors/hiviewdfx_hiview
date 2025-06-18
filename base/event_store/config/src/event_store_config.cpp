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
#include "event_store_config.h"

#include <fstream>
#include <map>

#include "cjson_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
namespace {
const char CONFIG_FILE_PATH[] = "/system/etc/hiview/event_store_config.json";
const char KEY_STORE_DAY[] = "StoreDay";
const char KEY_PAGE_SIZE[] = "PageSize";
const char KEY_MAX_SIZE[] = "MaxSize";
const char KEY_MAX_FILE_NUM[] = "MaxFileNum";
const char KEY_MAX_FILE_SIZE[] = "MaxFileSize";
const std::map<std::string, int> EVENT_TYPE_MAP = {
    {"FAULT", 1}, {"STATISTIC", 2}, {"SECURITY", 3}, {"BEHAVIOR", 4}
};

}
EventStoreConfig::EventStoreConfig()
{
    Init();
}

void EventStoreConfig::Init()
{
    cJSON* root = CJsonUtil::ParseJsonRoot(CONFIG_FILE_PATH);
    if (!cJSON_IsObject(root)) {
        return;
    }

    cJSON* subJson = nullptr;
    cJSON_ArrayForEach(subJson, root) {
        if (cJSON_IsObject(subJson) && EVENT_TYPE_MAP.find(subJson->string) != EVENT_TYPE_MAP.end()) {
            StoreConfig config;
            CJsonUtil::GetUintMemberValue(subJson, KEY_STORE_DAY, config.storeDay);
            CJsonUtil::GetUintMemberValue(subJson, KEY_PAGE_SIZE, config.pageSize);
            CJsonUtil::GetUintMemberValue(subJson, KEY_MAX_FILE_SIZE, config.maxFileSize);
            CJsonUtil::GetUintMemberValue(subJson, KEY_MAX_FILE_NUM, config.maxFileNum);
            CJsonUtil::GetUintMemberValue(subJson, KEY_MAX_SIZE, config.maxSize);
            configMap_.emplace(EVENT_TYPE_MAP.at(subJson->string), config);
        }
    }
    cJSON_Delete(root);
}

bool EventStoreConfig::Contain(int eventType)
{
    return configMap_.find(eventType) != configMap_.end();
}

uint32_t EventStoreConfig::GetStoreDay(int eventType)
{
    return Contain(eventType) ? configMap_[eventType].storeDay : 0;
}

uint32_t EventStoreConfig::GetMaxSize(int eventType)
{
    return Contain(eventType) ? configMap_[eventType].maxSize : 0;
}

uint32_t EventStoreConfig::GetMaxFileNum(int eventType)
{
    return Contain(eventType) ? configMap_[eventType].maxFileNum : 0;
}

uint32_t EventStoreConfig::GetPageSize(int eventType)
{
    return Contain(eventType) ? configMap_[eventType].pageSize : 0;
}

uint32_t EventStoreConfig::GetMaxFileSize(int eventType)
{
    return Contain(eventType) ? configMap_[eventType].maxFileSize : 0;
}
} // EventStore
} // HiviewDFX
} // OHOS
