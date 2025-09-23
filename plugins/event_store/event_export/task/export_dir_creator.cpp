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

#include "export_dir_creator.h"

#include <unistd.h>

#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");
namespace {
constexpr mode_t EVENT_EXPORT_DIR_MODE = S_IRWXU | S_IRWXG; //rwxrwx---
constexpr uint32_t LOG_GID = 1007;

bool ChangeDirModeAndGid(const std::string& dir, mode_t mode, uint32_t gid)
{
    if (!FileUtil::ChangeModeFile(dir, mode)) {
        HIVIEW_LOGE("failed to change directory mode of %{public}s.", StringUtil::HideDeviceIdInfo(dir).c_str());
        return false;
    }
    if (chown(dir.c_str(), -1, gid) != 0) {
        HIVIEW_LOGE("failed to change directory owner of %{public}s.", StringUtil::HideDeviceIdInfo(dir).c_str());
        return false;
    }
    return true;
}
}

ExportDirCreator& ExportDirCreator::GetInstance()
{
    static ExportDirCreator instance;
    return instance;
}

bool ExportDirCreator::CreateExportDir(const std::string& dir)
{
    std::unique_lock<ffrt::mutex> lock(dirMtx_);
    if (FileUtil::FileExists(dir)) {
        return ChangeDirModeAndGid(dir, EVENT_EXPORT_DIR_MODE, LOG_GID);
    }

    if (!FileUtil::ForceCreateDirectory(dir) || !ChangeDirModeAndGid(dir, EVENT_EXPORT_DIR_MODE, LOG_GID)) {
        HIVIEW_LOGE("failed to init export directory");
        return false;
    }
    return true;
}
} // HiviewDFX
} // OHOS