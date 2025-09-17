/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "app_event_publisher_factory.h"

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-AppEventPublisherFactory");
}

std::shared_ptr<std::unordered_map<std::string, bool>> AppEventPublisherFactory::GetPublisherRegistryMap()
{
    static std::shared_ptr<std::unordered_map<std::string, bool>> publisherMap;
    if (publisherMap == nullptr) {
        publisherMap = std::make_shared<std::unordered_map<std::string, bool>>();
    }
    return publisherMap;
}

bool AppEventPublisherFactory::IsPublisher(const std::string& name)
{
    auto publisherMap = GetPublisherRegistryMap();
    auto it = publisherMap->find(name);
    if (it != publisherMap->end()) {
        return it->second;
    }
    return false;
}

void AppEventPublisherFactory::RegisterPublisher(const std::string& name)
{
    if (name.empty()) {
        HIVIEW_LOGW("Register publisher empty name.");
        return;
    }
    // force update publisher constructor
    auto publisherMap = GetPublisherRegistryMap();
    if (publisherMap->find(name) == publisherMap->end()) {
        publisherMap->insert(std::pair<std::string, bool>(name, true));
    } else {
        HIVIEW_LOGW("publisher %{public}s already exists! register publisher failed", name.c_str());
    }
    HIVIEW_LOGI("Register publisher constructor from %{public}s.", name.c_str());
}

void AppEventPublisherFactory::UnregisterPublisher(const std::string& name)
{
    HIVIEW_LOGD("UnregisterPublisher from %{public}s.", name.c_str());
    auto publisherMap = GetPublisherRegistryMap();
    publisherMap->erase(name);
}
} // namespace HiviewDFX
} // namespace OHOS