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
 * Author: zhouhang
 * Create: 2024-8-21
 */

#include "log_file.h"

#include <list>
#include <stack>
#include <string>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace OHOS {
namespace HiviewDFX {
LogFile::LogFile(const std::string &fullPath) : path_(fullPath)
{
    struct stat sb;
    if ((stat(fullPath.c_str(), &sb) != -1)) {
        mtime_ = sb.st_mtime;
    }
    auto pos = fullPath.find_last_of('/');
    if (pos != std::string::npos) {
        dirPath_ = fullPath.substr(0, pos);
        fileName_ = fullPath.substr(pos + 1);
    }
}
} // namespace HiviewDFX
} // namespace OHOS