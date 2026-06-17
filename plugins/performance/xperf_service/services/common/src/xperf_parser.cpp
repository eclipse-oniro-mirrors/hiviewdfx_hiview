/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#include <string>
#include <cstring>
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

bool ExtractSubTag(const std::string& msg, std::string &value, const char* preTag,
    const char* nextTag)
{
    if (msg.empty() || preTag == nullptr || preTag[0] == '\0' || (msg.length() == strlen(preTag))) {
        return false;
    }

    const char* tagBegin = strstr(msg.c_str(), preTag);
    if (tagBegin == nullptr) {
        return false;
    }

    tagBegin += strlen(preTag);
    if (nextTag == nullptr || nextTag[0] == '\0') {
        value = tagBegin;
        return true;
    }

    const char* tagEnd = strstr(tagBegin, nextTag);
    if (tagEnd == nullptr || tagEnd == tagBegin) {
        return false;
    }

    value = std::string(tagBegin, tagEnd - tagBegin);
    return true;
}

void ExtractStrToLong(const std::string &msg, int64_t &result, const char* preTag,
    const char* nextTag, int64_t defaultValue)
{
    std::string value;
    if (ExtractSubTag(msg, value, preTag, nextTag)) {
        result = atoll(value.c_str());
    } else {
        LOGD("ExtractStrToLong error, preTag:%{public}s, nextTag:%{public}s", preTag, nextTag);
        result = defaultValue;
    }
}
 
void ExtractStrToInt(const std::string &msg, int32_t &result, const char* preTag,
    const char* nextTag, int32_t defaultValue)
{
    std::string value;
    if (ExtractSubTag(msg, value, preTag, nextTag)) {
        result = atoi(value.c_str());
    } else {
        LOGD("ExtractStrToInt error, preTag:%{public}s, nextTag:%{public}s", preTag, nextTag);
        result = defaultValue;
    }
}

void ExtractStrToInt16(const std::string &msg, int16_t &result, const char* preTag,
    const char* nextTag, int16_t defaultValue)
{
    std::string value;
    if (ExtractSubTag(msg, value, preTag, nextTag)) {
        result = static_cast<int16_t>(atoi(value.c_str()));
    } else {
        LOGD("ExtractStrToInt16 error, preTag:%{public}s, nextTag:%{public}s", preTag, nextTag);
        result = defaultValue;
    }
}
 
void ExtractStrToStr(const std::string &msg, std::string &result,
    const char* preTag, const char* nextTag, const std::string& defaultValue)
{
    if (!ExtractSubTag(msg, result, preTag, nextTag)) {
        LOGD("ExtractStrToStr error, preTag:%{public}s, nextTag:%{public}s", preTag, nextTag);
        result = defaultValue;
    }
}
} // namespace HiviewDFX
} // namespace OHOS