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

#ifndef OHOS_HIVIEWDFX_LOG_FILE_WRITER_H
#define OHOS_HIVIEWDFX_LOG_FILE_WRITER_H

#include <fstream>
#include <list>
#include <mutex>
#include <string>

namespace OHOS {
namespace HiviewDFX {
struct LogStrategy {
    // the name prefix of the log file
    std::string fileNamePrefix;

    // the max count of the log file with same type
    size_t fileMaxCnt = 0;

    // the size limit of a single log file
    uint64_t singleFileMaxSize = 0;
};

class LogFileWriter {
public:
    LogFileWriter(const LogStrategy& strategy);
    ~LogFileWriter();

public:
    void Write(const std::string& content);

private:
    void DeleteOutNumberLogFiles();
    void InitByStrategy(const LogStrategy& strategy);
    void ResetLogFileStreamByFileIndex(size_t fileIndex);

private:
    std::mutex writeMutex_;
    LogStrategy logStrategy_;
    std::ofstream logFileStream_;
    size_t curFileIndex_ = 0;
    int64_t curFileSize_ = 0;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_LOG_FILE_WRITER_H