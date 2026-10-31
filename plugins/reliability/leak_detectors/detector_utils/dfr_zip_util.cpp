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
#include "dfr_zip_util.h"

#include <zlib.h>
#include <zip.h>
#include <securec.h>
#include "fault_common.h"
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "DfrZipUtil");
using namespace std;

namespace {
constexpr int FILE_PATH_LENGTH = 100 * 1024;
}

zipFile DfrZipUtil::CreateZipFile(const string &zipPath)
{
    return zipOpen(zipPath.c_str(), APPEND_STATUS_CREATE);
}

string DfrZipUtil::GetSrcFileName(const string &srcFile, const string &dstFileName)
{
    string file = srcFile;
    string result = file;
    string::size_type pos = file.rfind("/");
    if (pos != string::npos && pos != file.length() - 1) {
        result = file.substr(pos + 1);
    }
    return result;
}

const FILE* DfrZipUtil::GetFileHanlderDefault(const string &file)
{
    const string &realFile = DfrUtil::GetInstance().RealPath(file);
    if (realFile.empty()) {
        HIVIEW_LOGE("failed to realpath file: %{public}s", file.c_str());
        return nullptr;
    }
    FILE* srcFp = fopen(realFile.c_str(), "rb");
    return srcFp;
}

bool DfrZipUtil::AddFileInZip(zipFile zipfile, string file, const string &dstFileName)
{
    FILE* srcFp = nullptr;
    zip_fileinfo zipInfo;
    int result = memset_s(&zipInfo, sizeof(zipInfo), 0, sizeof(zipInfo));
    if (result != 0) {
        HIVIEW_LOGE("failed to memset_s");
        return false;
    }

    string srcFileName = GetSrcFileName(file, dstFileName);
    srcFp = const_cast<FILE*>(GetFileHanlderDefault(file));
    if (srcFp == nullptr) {
        HIVIEW_LOGE("srcFp is nullptr");
        return false;
    }

    zipOpenNewFileInZip(zipfile, srcFileName.c_str(), &zipInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED,
                        Z_DEFAULT_COMPRESSION);

    char buf[FILE_PATH_LENGTH] = {0};
    while (!feof(srcFp)) {
        size_t numBytes = fread(buf, 1, sizeof(buf), srcFp);
        if (numBytes >= 1) {
            zipWriteInFileInZip(zipfile, buf, static_cast<unsigned int>(numBytes));
        }
        if (ferror(srcFp)) {
            HIVIEW_LOGE("ferror srcFp");
            break;
        }
    }

    if (fclose(srcFp) != 0) {
        HIVIEW_LOGW("fclose srcFp failed");
    }
    zipCloseFileInZip(zipfile);
    return true;
}

void DfrZipUtil::CloseZipFile(zipFile zipfile)
{
    if (zipfile == nullptr) {
        return;
    }
    zipClose(zipfile, NULL);
}

bool DfrZipUtil::ZipMultiFiles(list<string> &srcFiles, const string &dstZipFile)
{
    zipFile zf = CreateZipFile(dstZipFile);
    if (zf == nullptr) {
        HIVIEW_LOGE("failed to CreateZipFile, dstZipFile: %{public}s", dstZipFile.c_str());
        return false;
    }

    bool result = true;
    for (auto it = srcFiles.begin(); it != srcFiles.end(); ++it) {
        string &temp = *it;
        if (!AddFileInZip(zf, temp, dstZipFile)) {
            HIVIEW_LOGE("failed to AddFileInZip, src: %{public}s", temp.c_str());
            result = false;
        }
    }

    CloseZipFile(zf);
    return result;
}
}
}
