/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "common_util.h"

#include <regex>

#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("UCollectUtil-CommonUtil");
}

bool CommonUtil::ParseTypeAndValue(const std::string &str, std::string &type, int64_t &value)
{
    std::string::size_type typePos = str.find(":");
    if (typePos != std::string::npos) {
        type = str.substr(0, typePos);
        std::string valueStr = str.substr(typePos + 1);
        std::string::size_type valuePos = valueStr.find("kB");
        if (valuePos == std::string::npos) {
            valuePos = valueStr.find("KB");
        }
        if (valuePos != std::string::npos) {
            valueStr.resize(valuePos);
        }
        StrToNum(valueStr, value);
        return true;
    }
    return false;
}

int32_t CommonUtil::ReadNodeWithOnlyNumber(const std::string& fileName)
{
    std::string content;
    if (!FileUtil::LoadStringFromFile(fileName, content)) {
        HIVIEW_LOGW("read node failed");
        return 0;
    }
    int32_t parsedVal = 0;
    // this string content might be empty or consist of some special charactors
    // so "std::stoi" and "StringUtil::StrToInt" aren't applicable here.
    std::stringstream ss(content);
    ss >> parsedVal;
    return parsedVal;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
