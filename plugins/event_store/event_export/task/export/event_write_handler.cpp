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

#include "event_write_handler.h"

#include "file_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventWriteHandler");
bool EventWriteHandler::HandleRequest(RequestPtr req)
{
    auto writeReq = BaseRequest::DownCastTo<EventWriteRequest>(req);
    for (const auto& sysEvent : writeReq->sysEvents) {
        auto writer = GetEventWriter(sysEvent->version, writeReq);
        if (!writer->AppendEvent(sysEvent->domain, sysEvent->name, sysEvent->eventStr)) {
            HIVIEW_LOGE("failed to append event to event writer");
            Rollback();
            return false;
        }
    }
    if (!writeReq->isQueryCompleted) {
        return true;
    }
    for (const auto& writer : allJsonFileWriters_) {
        if (writer.second == nullptr) {
            continue;
        }
        if (!writer.second->Write()) {
            HIVIEW_LOGE("failed to write export event");
            Rollback();
            return false;
        }
    }
    CopyTmpZipFilesToDest();
    return true;
}

std::shared_ptr<ExportJsonFileWriter> EventWriteHandler::GetEventWriter(const std::string& sysVersion,
    std::shared_ptr<EventWriteRequest> writeReq)
{
    auto writerKey = std::make_pair(writeReq->moduleName, sysVersion);
    auto iter = allJsonFileWriters_.find(writerKey);
    if (iter == allJsonFileWriters_.end()) {
        HIVIEW_LOGI("create json file writer with version %{public}s", sysVersion.c_str());
        auto jsonFileWriter = std::make_shared<ExportJsonFileWriter>(writeReq->moduleName, sysVersion,
            writeReq->exportDir, writeReq->maxSingleFileSize);
        jsonFileWriter->SetExportJsonFileZippedListener([this] (const std::string& srcPath,
            const std::string& destPath) {
            zippedExportFileMap_[srcPath] = destPath;
        });
        allJsonFileWriters_.emplace(writerKey, jsonFileWriter);
        return jsonFileWriter;
    }
    return iter->second;
}

void EventWriteHandler::CopyTmpZipFilesToDest()
{
    // move all tmp zipped event export file to dest dir
    std::for_each(zippedExportFileMap_.begin(), zippedExportFileMap_.end(), [] (const auto& item) {
        if (!FileUtil::RenameFile(item.first, item.second)) {
            HIVIEW_LOGE("failed to move %{private}s to %{private}s", item.first.c_str(), item.second.c_str());
        }
    });
    zippedExportFileMap_.clear();
}

void EventWriteHandler::Rollback()
{
    for (const auto& writer : allJsonFileWriters_) {
        if (writer.second == nullptr) {
            continue;
        }
        writer.second->ClearEventCache();
    }
    // delete all tmp zipped export file
    std::for_each(zippedExportFileMap_.begin(), zippedExportFileMap_.end(), [] (const auto& item) {
        if (!FileUtil::RemoveFile(item.first)) {
            HIVIEW_LOGE("failed to delete %{private}s", item.first.c_str());
        }
    });
    zippedExportFileMap_.clear();
}
} // HiviewDFX
} // OHOS