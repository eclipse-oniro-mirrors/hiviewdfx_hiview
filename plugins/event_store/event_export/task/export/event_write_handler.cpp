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

#include "event_write_handler.h"

#include "event_write_strategy_factory.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");
bool EventWriteHandler::HandleRequest(RequestPtr req)
{
    auto writeReq = BaseRequest::DownCastTo<EventWriteRequest>(req);
    for (const auto& cachedEvent : writeReq->cachedEvents) {
        if (cachedEvent == nullptr) {
            HIVIEW_LOGE("invalid event");
            Rollback();
            return false;
        }
        auto packager = GetCachedEventPackager(cachedEvent, writeReq);
        if (packager == nullptr ||
            !packager->AppendCachedEvent(cachedEvent->domain, cachedEvent->name, cachedEvent->eventStr)) {
            HIVIEW_LOGE("failed to append event to event writer");
            Rollback();
            return false;
        }
    }
    if (!writeReq->isQueryCompleted) {
        return true;
    }
    for (const auto& packagerItem : cachedEventPackagerMap_) {
        if (packagerItem.second == nullptr) {
            continue;
        }
        if (!packagerItem.second->Package()) {
            HIVIEW_LOGE("failed to write export event");
            Rollback();
            return false;
        }
    }
    Finish();
    return true;
}

std::shared_ptr<CachedEventPackager> EventWriteHandler::GetCachedEventPackager(
    const std::shared_ptr<CachedEvent> cachedEvent, std::shared_ptr<EventWriteRequest> writeReq)
{
    auto strategy = EventWriteStrategyFactory::GetWriteStrategy(StrategyType::ZIP_FILE);
    std::string packagerKey = strategy->GetPackagerKey(cachedEvent);
    if (packagerKey.empty()) {
        HIVIEW_LOGW("pacakger key is empty");
        return nullptr;
    }
    auto iter = cachedEventPackagerMap_.find(packagerKey);
    if (iter == cachedEventPackagerMap_.end()) {
        HIVIEW_LOGI("create json file writer with system version[%{public}s] and patch version[%{public}s]",
            cachedEvent->version.systemVersion.c_str(), cachedEvent->version.patchVersion.c_str());
        auto cachedEventPackager = std::make_shared<CachedEventPackager>(writeReq->moduleName, writeReq->exportDir,
            cachedEvent->version, cachedEvent->uid, writeReq->maxSingleFileSize);
        cachedEventPackagerMap_.emplace(packagerKey, cachedEventPackager);
        return cachedEventPackager;
    }
    return iter->second;
}

void EventWriteHandler::Finish()
{
    for (const auto& packagerItem : cachedEventPackagerMap_) {
        if (packagerItem.second == nullptr) {
            continue;
        }
        packagerItem.second->HandlePackagedFileCache();
    }
}

void EventWriteHandler::Rollback()
{
    for (const auto& packagerItem : cachedEventPackagerMap_) {
        if (packagerItem.second == nullptr) {
            continue;
        }
        packagerItem.second->ClearPackagedFileCache();
    }
}
} // HiviewDFX
} // OHOS