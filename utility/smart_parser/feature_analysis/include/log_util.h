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
#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace OHOS {
namespace HiviewDFX {
struct IpcItem {
    std::string pidTid{0};
    std::string packageName;
    std::string threadName;
};

struct IpcTrans {
    double waitTime{0};
    std::string code;
    std::string line;
    IpcItem cIpcItem;
    IpcItem sIpcItem;
};

class LogUtil {
public:
    LogUtil(const LogUtil&) = delete;
    LogUtil& operator=(const LogUtil&) = delete;
    static void GetTrace(std::stringstream& buffer, int cursor, const std::string& reg, std::string& result,
        std::string startReg = "");
    static bool ReadFileBuff(const std::string& file, std::stringstream& buffer);
    static bool IsTestModel(const std::string& sourceFile, const std::string& name,
        const std::string& pattern, std::string& desPath);
    static bool FileExist(const std::string& file);

public:
    static constexpr const char *SPLIT_PATTERN = "\n";
    static constexpr const char *SMART_PARSER_TEST_DIR = "/data/test/test_data/SmartParser";
    static constexpr int TOTAL_LINE_NUM = 200;

private:
    static int GetFileFd(const std::string& file);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif /* LOG_UTIL_H */
