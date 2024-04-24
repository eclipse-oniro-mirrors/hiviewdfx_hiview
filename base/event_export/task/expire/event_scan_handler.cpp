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

#include "event_scan_handler.h"

#include <cstdlib>
#include <regex>
#include <fstream>
#include <sys/stat.h>

#include "event_delete_handler.h"
#include "hiview_logger.h"
#include "file_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventScanHandler");
namespace {
constexpr uint64_t DAY_TO_SECONDS = 24 * 60 * 60;

uint64_t GetFileLastModifiedTime(const std::string& file)
{
    struct stat fileInfo;
    stat(file.c_str(), &fileInfo);
    return fileInfo.st_mtime;
}

uint64_t GetDuration(uint64_t from, uint64_t to)
{
    return ((from >= to) ? (from - to) : (to - from));
}

void GetExpiredFileNames(std::vector<std::string>& dest, const std::string& scanDir, uint8_t storedDayCnt)
{
    std::vector<std::string> scannedFiles;
    FileUtil::GetDirFiles(scanDir, scannedFiles);
    if (scannedFiles.empty()) {
        HIVIEW_LOGW("no packaged export event file found.");
        return;
    }
    std::regex reg { ".*/HSE_.*\\.zip$" };
    std::smatch match;
    for (const auto& scannedFile : scannedFiles) {
        HIVIEW_LOGD("scannedFile is %{public}s", scannedFile.c_str());
        if (!std::regex_match(scannedFile, match, reg)) {
            continue;
        }
        auto fileModifyTime = GetFileLastModifiedTime(scannedFile);
        auto currentTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
        HIVIEW_LOGD("current time: %{public}" PRIu64 ", file last modified time: %{public}" PRIu64 "", currentTime,
            fileModifyTime);
        if (GetDuration(currentTime, fileModifyTime) > storedDayCnt * DAY_TO_SECONDS) {
            dest.emplace_back(scannedFile);
        }
    }
}
}

bool EventScanHandler::HandleRequest(RequestPtr req)
{
    auto scanReq = BaseRequest::DownCastTo<EventScanRequest>(req);
    return Scan(scanReq->moduleName, scanReq->scanDir, scanReq->storedDayCnt);
}

bool EventScanHandler::Scan(const std::string& moduleName, const std::string& scanDir, uint8_t storedDayCnt)
{
    auto delReq = std::make_shared<EventDelRequest>();
    delReq->moduleName = moduleName;
    GetExpiredFileNames(delReq->files, scanDir, storedDayCnt);
    if (delReq->files.empty()) {
        HIVIEW_LOGW("no expired event file found");
        return false;
    }
    HIVIEW_LOGW("count of export event zip file is %{public}zu", delReq->files.size());
    if (nextHandler_ != nullptr) {
        nextHandler_->HandleRequest(delReq);
    }
    return true;
}
} // HiviewDFX
} // OHOS