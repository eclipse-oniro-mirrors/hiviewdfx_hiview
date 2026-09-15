/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "data_share_util.h"

#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bundle_mgr_client.h"
#include "file_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-DataShareUtil");
namespace {
constexpr int VALUE_MOD = 200000;
}  // namespace

std::string DataShareUtil::GetSandBoxPathByUid(int32_t uid)
{
    int userId = uid / VALUE_MOD;
    std::string bundleName = OHOS::HiviewDFX::DataShareUtil::GetBundleNameById(uid);
    std::string path;
    path.append("/data/app/el2/")
        .append(std::to_string(userId))
        .append("/base/")
        .append(bundleName)
        .append("/cache/hiview/event");
    return path;
}

int DataShareUtil::CopyFile(const char *src, const char *des)
{
    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        HIVIEW_LOGE("failed to open source file, src=%{public}s", src);
        return -1;
    }

    int dest_fd = open(des, O_WRONLY | O_CREAT | O_NOFOLLOW, S_IWUSR);
    if (dest_fd == -1) {
        perror("open");
        HIVIEW_LOGE("failed to open destination file, des=%{public}s", des);
        close(src_fd);
        return -1;
    }

    struct stat st;
    if (fstat(src_fd, &st) == -1) {
        HIVIEW_LOGE("failed to get source file size");
        close(dest_fd);
        close(src_fd);
        return -1;
    }

    off_t offset = 0;
    ssize_t ret = sendfile(dest_fd, src_fd, &offset, st.st_size);
    if (ret == -1) {
        HIVIEW_LOGE("failed to sendfile");
        close(dest_fd);
        close(src_fd);
        return -1;
    }
    close(src_fd);
    close(dest_fd);
    return 0;
}

std::string DataShareUtil::GetBundleNameById(int32_t uid)
{
    std::string bundleName;
    AppExecFwk::BundleMgrClient client;
    if (client.GetNameForUid(uid, bundleName) != ERR_OK) {
        HIVIEW_LOGE("Failed to query bundleName from bms, uid:%{public}d.", uid);
    } else {
        HIVIEW_LOGE("Succ to get bundleName of uid:%{public}d", uid);
    }
    return bundleName;
}
}  // namespace HiviewDFX
}  // namespace OHOS