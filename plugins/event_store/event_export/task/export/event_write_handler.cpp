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
    for (const auto& event : writeReq->events) {
        if (event == nullptr) {
            HIVIEW_LOGE("invalid event");
            Rollback();
            return false;
        }
        auto packager = GetEventPackager(event, writeReq);
        if (packager == nullptr ||
            !packager->AppendEvent(event->domain, event->name, event->eventStr)) {
            HIVIEW_LOGE("failed to append event to event writer");
            Rollback();
            return false;
        }
    }
    if (!writeReq->isQueryCompleted) {
        return true;
    }
    for (const auto& packager : packagers_) {
        if (packager.second == nullptr) {
            continue;
        }
        if (!packager.second->Package()) {
            HIVIEW_LOGE("failed to write export event");
            Rollback();
            return false;
        }
    }
    Finish();
    return true;
}

std::shared_ptr<ExportEventPackager> EventWriteHandler::GetEventPackager(
    const std::shared_ptr<CachedEvent> event, std::shared_ptr<EventWriteRequest> writeReq)
{
    auto strategy = EventWriteStrategyFactory::GetWriteStrategy(StrategyType::ZIP_JSON_FILE);
    if (strategy == nullptr) {
        HIVIEW_LOGW("write strategy is null");
        return nullptr;
    }
    std::string packagerKey = strategy->GetPackagerKey(event);
    if (packagerKey.empty()) {
        HIVIEW_LOGW("pacakger key is empty");
        return nullptr;
    }
    auto iter = packagers_.find(packagerKey);
    if (iter == packagers_.end()) {
        HIVIEW_LOGI("create packager: [%{public}s][%{public}s][%{public}" PRId32 "]",
            event->version.systemVersion.c_str(), event->version.patchVersion.c_str(), event->uid);
        auto packager = std::make_shared<ExportEventPackager>(writeReq->moduleName, writeReq->exportDir,
            event->version, event->uid, writeReq->maxSingleFileSize);
            packagers_.emplace(packagerKey, packager);
        return packager;
    }
    return iter->second;
}

void EventWriteHandler::Finish()
{
    for (const auto& packager : packagers_) {
        if (packager.second == nullptr) {
            continue;
        }
        packager.second->HandlePackagedFiles();
    }
}

void EventWriteHandler::Rollback()
{
    for (const auto& packager : packagers_) {
        if (packager.second == nullptr) {
            continue;
        }
        packager.second->ClearPackagedFiles();
    }
}
} // HiviewDFX
} // OHOS
