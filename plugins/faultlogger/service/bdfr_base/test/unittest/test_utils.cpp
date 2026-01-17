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
#include "test_utils.h"

#include <fstream>
#include <iostream>

#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
void CheckSumarryParseResult(std::string& info, int& matchCount)
{
    Json::Reader reader;
    Json::Value appEvent;
    if (!(reader.parse(info, appEvent))) {
        matchCount--;
    }
    auto exception = appEvent["exception"];
    if (exception["name"] == "" || exception["name"] == "none") {
        matchCount--;
    }
    if (exception["message"] == "" || exception["message"] == "none") {
        matchCount--;
    }
    if (exception["stack"] == "" || exception["stack"] == "none") {
        matchCount--;
    }
}

int CheckKeyWordsInFile(const std::string& filePath, std::string *keywords, int length, bool isJsError)
{
    std::ifstream file;
    file.open(filePath.c_str(), std::ios::in);
    std::ostringstream infoStream;
    infoStream << file.rdbuf();
    std::string info = infoStream.str();
    if (info.length() == 0) {
        std::cout << "file is empty, file:" << filePath << std::endl;
        return 0;
    }
    int matchCount = 0;
    for (int index = 0; index < length; index++) {
        if (info.find(keywords[index]) != std::string::npos) {
            matchCount++;
        } else {
            std::cout << "can not find keyword:" << keywords[index] << std::endl;
        }
    }
    if (isJsError) {
        CheckSumarryParseResult(info, matchCount);
    }
    file.close();
    return matchCount;
}
} // namespace HiviewDFX
} // namespace OHOS
