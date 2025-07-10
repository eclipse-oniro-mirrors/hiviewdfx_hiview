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

#include "write_zip_file_strategy.h"

#include <chrono>
#include <cstdio>

#include "event_export_util.h"
#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "hiview_zip_util.h"
#include "parameter.h"
#include "parameter_ex.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");
namespace {
constexpr char EXPORT_JSON_FILE_NAME[] = "HiSysEvent.json";
constexpr char SYSEVENT_EXPORT_TMP_DIR[] = "tmp";
constexpr char SYSEVENT_EXPORT_DIR[] = "sys_event_export";
constexpr char ZIP_FILE_DELIM[] = "_";
constexpr mode_t EVENT_EXPORT_DIR_MODE = S_IRWXU | S_IRWXG; // rwxrwx---
constexpr mode_t EVENT_EXPORT_FILE_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP; // rw-rw----
constexpr uint32_t LOG_GID = 1007;

void AppendZipFile(std::string& dir, int32_t uid)
{
    dir.append("HSE").append(ZIP_FILE_DELIM)
        .append(Parameter::GetBrandStr()).append(std::string(ZIP_FILE_DELIM))
        .append(Parameter::GetProductModelStr()).append(std::string(ZIP_FILE_DELIM))
        .append(Parameter::GetSysVersionStr()).append(std::string(ZIP_FILE_DELIM))
        .append(EventExportUtil::GetDeviceId()).append(std::string(ZIP_FILE_DELIM))
        .append(TimeUtil::GetFormattedTimestampEndWithMilli()).append(std::string(ZIP_FILE_DELIM))
        .append("U").append(std::to_string(uid)).append(std::string(".zip"));
}

std::string GetWroteTempDir(const std::string& moduleName, const EventVersion& version)
{
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        return "";
    }
    std::string tmpDir = context->GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY);
    tmpDir = FileUtil::IncludeTrailingPathDelimiter(tmpDir.append(SYSEVENT_EXPORT_DIR));
    tmpDir = FileUtil::IncludeTrailingPathDelimiter(tmpDir.append(SYSEVENT_EXPORT_TMP_DIR));
    tmpDir = FileUtil::IncludeTrailingPathDelimiter(tmpDir.append(moduleName));
    tmpDir = FileUtil::IncludeTrailingPathDelimiter(tmpDir.append(version.systemVersion));
    if (!FileUtil::IsDirectory(tmpDir) && !FileUtil::ForceCreateDirectory(tmpDir)) {
        HIVIEW_LOGE("failed to init directory %{public}s.", tmpDir.c_str());
        return "";
    }
    return tmpDir;
}

std::string GetTmpZipFile(const std::string& exportDir, const std::string& moduleName,
    const EventVersion& version, int32_t uid)
{
    std::string dir = FileUtil::IncludeTrailingPathDelimiter(exportDir);
    dir = FileUtil::IncludeTrailingPathDelimiter(dir.append(SYSEVENT_EXPORT_TMP_DIR));
    dir = FileUtil::IncludeTrailingPathDelimiter(dir.append(moduleName));
    dir = FileUtil::IncludeTrailingPathDelimiter(dir.append(version.systemVersion));
    if (!FileUtil::IsDirectory(dir) && !FileUtil::ForceCreateDirectory(dir)) {
        HIVIEW_LOGE("failed to init directory %{public}s.", dir.c_str());
        return "";
    }
    AppendZipFile(dir, uid);
    return dir;
}

bool ChangeFileModeAndGid(const std::string& file, mode_t mode, uint32_t gid)
{
    if (!FileUtil::ChangeModeFile(file, mode)) {
        HIVIEW_LOGE("failed to change file mode of %{public}s.", StringUtil::HideDeviceIdInfo(file).c_str());
        return false;
    }
    if (chown(file.c_str(), -1, gid) != 0) {
        HIVIEW_LOGE("failed to change file owner of %{public}s.", StringUtil::HideDeviceIdInfo(file).c_str());
        return false;
    }
    return true;
}

std::string GetZipFile(const std::string& exportDir, int32_t uid)
{
    std::string dir = FileUtil::IncludeTrailingPathDelimiter(exportDir);
    if (!FileUtil::IsDirectory(dir) && !FileUtil::ForceCreateDirectory(dir)) {
        HIVIEW_LOGE("failed to init directory %{public}s.", dir.c_str());
        return "";
    }
    if (!ChangeFileModeAndGid(dir, EVENT_EXPORT_DIR_MODE, LOG_GID)) {
        return "";
    }
    AppendZipFile(dir, uid);
    return dir;
}

bool ZipExportFile(const std::string& src, const std::string& dest)
{
    HiviewZipUnit zipUnit(dest);
    if (int32_t ret = zipUnit.AddFileInZip(src, ZipFileLevel::KEEP_NONE_PARENT_PATH); ret != 0) {
        HIVIEW_LOGW("zip db failed, ret: %{public}d.", ret);
        return false;
    }
    if (!ChangeFileModeAndGid(dest, EVENT_EXPORT_FILE_MODE, LOG_GID)) {
        return false;
    }
    // delete json file
    FileUtil::RemoveFile(src);
    return true;
}

void WriteContentToFile(std::string& content, const std::string& localFile)
{
    FILE* file = fopen(localFile.c_str(), "w+");
    if (file == nullptr) {
        HIVIEW_LOGE("failed to open file: %{public}s.", localFile.c_str());
        return;
    }
    (void)fprintf(file, "%s", content.c_str());
    (void)fclose(file);
}
}

std::string WriteZipFileStrategy::GetPackagerKey(std::shared_ptr<CachedEvent> event)
{
    if (event == nullptr) {
        return "";
    }
    std::string packagerKey;
    packagerKey.append(event->version.systemVersion).append("_");
    packagerKey.append(event->version.patchVersion).append("_");
    packagerKey.append(std::to_string(event->uid));
    return packagerKey;
}

bool WriteZipFileStrategy::Write(std::string& exportContent, WroteCallback callback)
{
    auto wroteFileName = GetWroteTempDir(param_.moduleName, param_.version).append(EXPORT_JSON_FILE_NAME);
    WriteContentToFile(exportContent, wroteFileName);
    // zip json file into a temporary zip file
    auto tmpZipFile = GetTmpZipFile(param_.exportDir, param_.moduleName, param_.version, param_.uid);
    if (!ZipExportFile(wroteFileName, tmpZipFile)) {
        HIVIEW_LOGE("failed to zip %{public}s to %{public}s", wroteFileName.c_str(),
            StringUtil::HideDeviceIdInfo(tmpZipFile).c_str());
        return false;
    }
    auto zipFile = GetZipFile(param_.exportDir, param_.uid);
    HIVIEW_LOGD("dest file: %{public}s", StringUtil::HideDeviceIdInfo(zipFile).c_str());
    if (callback != nullptr) {
        callback(tmpZipFile, zipFile);
    }
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS