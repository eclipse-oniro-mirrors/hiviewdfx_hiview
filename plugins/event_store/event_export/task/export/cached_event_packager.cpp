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

#include "cached_event_packager.h"

#include "event_write_strategy_factory.h"
#include "export_file_writer.h"
#include "export_json_file_builder.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");
namespace {
constexpr int64_t MB_TO_BYTE = 1024 * 1024;
}

CachedEventPackager::CachedEventPackager(const std::string& moduleName, const std::string& exportDir,
    const EventVersion& eventVersion, int32_t uid, int64_t maxFileSize)
{
    moduleName_ = moduleName;
    exportDir_ = exportDir;
    eventVersion_ = eventVersion;
    uid_ = uid;
    maxFileSize_ = maxFileSize * MB_TO_BYTE;
}

bool CachedEventPackager::AppendCachedEvent(const std::string& domain, const std::string& name,
    const std::string& eventStr)
{
    int64_t eventSize = static_cast<int64_t>(eventStr.size());
    if (totalJsonStrSize_ + eventSize > maxFileSize_ && !Package()) {
        HIVIEW_LOGE("failed to package events");
        return false;
    }
    auto iter = eventCache_.find(domain);
    if (iter == eventCache_.end()) {
        eventCache_.emplace(domain, std::vector<std::pair<std::string, std::string>> {
            std::make_pair(name, eventStr),
        });
        totalJsonStrSize_ += eventSize;
        return true;
    }
    iter->second.emplace_back(name, eventStr);
    totalJsonStrSize_ += eventSize;
    return true;
}

void CachedEventPackager::ClearEventCache()
{
    eventCache_.clear();
    totalJsonStrSize_ = 0;
}

bool CachedEventPackager::Package()
{
    if (eventCache_.empty()) {
        HIVIEW_LOGW("no need to package empty cache");
        return true;
    }
    ExportFileWriter fileWriter;
    fileWriter.SetExportFileWroteListener([this] (const std::string& srcPath,
        const std::string& destPath) {
        packagedFileCache_[srcPath] = destPath;
    });
    WriteStrategyParam param {
        moduleName_,
        exportDir_,
        eventVersion_,
        uid_,
    };
    bool ret = fileWriter.Write(std::make_shared<ExportJsonFileBuilder>(eventVersion_), eventCache_, param);
    if (!ret) {
        HIVIEW_LOGE("failed to package cached events");
    }
    // after package, the event cache must be cleared
    ClearEventCache();
    return ret;
}

void CachedEventPackager::ClearPackagedFileCache()
{
    // delete all temporary pacakaged files
    std::for_each(packagedFileCache_.begin(), packagedFileCache_.end(), [] (const auto& item) {
        if (!FileUtil::RemoveFile(item.first)) {
            HIVIEW_LOGE("failed to delete %{public}s", StringUtil::HideDeviceIdInfo(item.first).c_str());
        }
    });
    packagedFileCache_.clear();
}

void CachedEventPackager::HandlePackagedFileCache()
{
    // move all temporary packaged file to dest directory
    std::for_each(packagedFileCache_.begin(), packagedFileCache_.end(), [] (const auto& item) {
        if (!FileUtil::RenameFile(item.first, item.second)) {
            HIVIEW_LOGE("failed to move %{public}s to %{public}s", StringUtil::HideDeviceIdInfo(item.first).c_str(),
                StringUtil::HideDeviceIdInfo(item.second).c_str());
        }
        HIVIEW_LOGI("renamed file: %{public}s", StringUtil::HideDeviceIdInfo(item.second).c_str());
    });
    packagedFileCache_.clear();
}
} // HiviewDFX
} // OHOS