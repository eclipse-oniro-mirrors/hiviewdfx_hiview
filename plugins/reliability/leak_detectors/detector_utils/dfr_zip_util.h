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

#ifndef DFR_ZIP_UTIL_H
#define DFR_ZIP_UTIL_H

#include <zlib.h>
#include <zip.h>
#include <string>
#include <list>

namespace OHOS {
namespace HiviewDFX {
class DfrZipUtil {
private:
    static zipFile CreateZipFile(const std::string &zipPath);
    static std::string GetSrcFileName(const std::string &srcFile, const std::string &dstFileName);
    static const FILE* GetFileHanlderDefault(const std::string &file);
    static bool AddFileInZip(zipFile zipfile, std::string file, const std::string &dstFileName);
    static void CloseZipFile(zipFile zipfile);

public:
    static bool ZipMultiFiles(std::list<std::string> &srcFiles, const std::string &dstZipFile);
};
}
}
#endif
