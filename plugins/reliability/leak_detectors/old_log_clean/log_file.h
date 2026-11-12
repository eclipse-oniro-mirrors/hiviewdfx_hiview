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

#ifndef DFR_LOG_FILE_H
#define DFR_LOG_FILE_H

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace OHOS {
namespace HiviewDFX {
class LogFile {
public:
    // base path should be end with "/", and if name contains slash which means
    // the name has sub directories
    explicit LogFile(const std::string &fullPath);
    ~LogFile() {};
    bool operator < (const LogFile &other) const
    {
        return (mtime_ < other.mtime_);
    }
    time_t mtime_ = 0;
    std::string path_ = "";
    std::string dirPath_ = "";
    std::string fileName_ = "";
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_LOG_FILE_H
