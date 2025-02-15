/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "log_util.h"

#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("LogUtil");

namespace {
    const string ARROW = "->";
    const string CODE = "code";
    const string WAIT = "wait";
    const int BUF_LEN_2048 = 2048;
    const int TOTAL_SKIP_NUM = 4;
}

const std::string LogUtil::SPLIT_PATTERN = "\n";
const std::string LogUtil::SMART_PARSER_TEST_DIR = "/data/test/test_data/SmartParser";
const int LogUtil::TOTAL_LINE_NUM = 200;

/* GetTrace function:
 * buffer : log buffer
 * cursor : buffer seekg
 * reg : regex which is used to get trace line
 * result : all trace line will be spliced by "\n"
 * startReg : start place when regex is match, default empty string
 */
void LogUtil::GetTrace(stringstream& buffer, int cursor, const string& reg, string& result, string startReg)
{
    buffer.seekg(cursor, ios::beg);
    string line;
    bool start = false;
    int num = 0;
    int skipNum = 0;
    startReg = startReg.empty() ? reg : startReg;

    while (getline(buffer, line) && num++ < TOTAL_LINE_NUM) {
        if (line.length() > BUF_LEN_2048) {
            continue;
        }
        if (line.size() == 0 || skipNum >= TOTAL_SKIP_NUM) {
            break; // blank line
        }
        if (!start) {
            start = regex_search(line, regex(startReg));
            if (!start) {
                continue;
            }
        }

        smatch matches;
        if (regex_search(line, matches, regex(reg))) {
            skipNum = 0;
            result += matches.str(0) + LogUtil::SPLIT_PATTERN;
            continue;
        }

        if (regex_match(line, matches, regex("^Tid:\\d+, Name:.*$"))) {
            break; // match new thread break
        }

        skipNum++;
    }
}

bool LogUtil::ReadFileBuff(const string& file, stringstream& buffer)
{
    int fd = LogUtil::GetFileFd(file);
    if (fd < 0) {
        HIVIEW_LOGE("%{public}s get fd fail, fd is %{public}d.", file.c_str(), fd);
        return false;
    }

    std::string content;
    if (!FileUtil::LoadStringFromFd(fd, content)) {
        HIVIEW_LOGE("read file: %s failed, fd is %d\n", file.c_str(), fd);
        close(fd);
        return false;
    }
    buffer.str(content);
    close(fd);
    return true;
}

int LogUtil::GetFileFd(const string& file)
{
    if (file.empty() || !FileUtil::IsLegalPath(file)) {
        HIVIEW_LOGE("the system file (%{public}s) is illegal.", file.c_str());
        return -1;
    }
    std::string realFileName;
    if (!FileUtil::PathToRealPath(file, realFileName) || realFileName.empty() ||
        !FileUtil::FileExists(realFileName)) {
        HIVIEW_LOGE("the system file (%{public}s) is not found.", realFileName.c_str());
        return -1;
    }
    return open(realFileName.c_str(), O_RDONLY);
}

bool LogUtil::FileExist(const string& file)
{
    return FileUtil::FileExists(file);
}

bool LogUtil::IsTestModel(const string& sourceFile, const string& name,
    const string& pattern, string& desPath)
{
    if (FileUtil::IsDirectory(LogUtil::SMART_PARSER_TEST_DIR)) {
        HIVIEW_LOGI("test dir exist.");
        std::string sourceFileName = StringUtil::GetRrightSubstr(sourceFile, "/");
        std::string dirOrFileName = StringUtil::GetRrightSubstr(name, "/");
        std::string fileName = pattern.find("/") != std::string::npos ?
            StringUtil::GetRrightSubstr(pattern, "/") : pattern;
        smatch result;
        if (regex_match(sourceFileName, result, regex(dirOrFileName)) ||
            regex_match(sourceFileName, result, regex(fileName))) {
            return LogUtil::FileExist(desPath);
        }
        return false;
    }
    return false;
}
} // namespace HiviewDFX
} // namespace OHOS
