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

#include "zip_util.h"

#include <contrib/minizip/zip.h>
#include <contrib/minizip/unzip.h>

#include "file_util.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
namespace ZipUtil {
namespace {
constexpr uint32_t BUFFER_SIZE = 100 * 1024;
}

void ZipFile(const std::string& srcFile, const std::string& destZipFile)
{
    FILE* src = fopen(srcFile.c_str(), "rb");
    if (src == nullptr) {
        return;
    }
    zip_fileinfo zInfo;
    errno_t ret = memset_s(&zInfo, sizeof(zInfo), 0, sizeof(zInfo));
    if (ret != EOK) {
        (void)fclose(src);
        return;
    }
    zipFile zipFile = zipOpen(destZipFile.c_str(), APPEND_STATUS_CREATE);
    if (zipFile == nullptr) {
        (void)fclose(src);
        return;
    }
    std::string srcFileName = FileUtil::ExtractFileName(srcFile);
    zipOpenNewFileInZip(
        zipFile, srcFileName.c_str(), &zInfo, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    char buf[BUFFER_SIZE] = {0};
    while (!feof(src)) {
        int readBytesCnt = fread(buf, sizeof(char), sizeof(buf), src);
        if (readBytesCnt <= 0) {
            break;
        }
        zipWriteInFileInZip(zipFile, buf, static_cast<unsigned int>(readBytesCnt));
        if (ferror(src)) {
            break;
        }
    }
    zipCloseFileInZip(zipFile);
    zipClose(zipFile, nullptr);
    (void)fclose(src);
}
}
} // HiviewDFX
} // OHOS