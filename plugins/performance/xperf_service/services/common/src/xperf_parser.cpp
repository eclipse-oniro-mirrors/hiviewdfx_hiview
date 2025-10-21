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
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

bool ExtractSubTag(const std::string& msg, std::string &value, const std::string& preTag,
    const std::string& nextTag)
{
    if (msg.empty() || preTag.empty() || (msg.length() == preTag.length())) {
        return false;
    }

    size_t tagBegin = msg.find(preTag);
    if (tagBegin == std::string::npos) {
        return false;
    }

    tagBegin += preTag.length();
    if (nextTag.empty()) {
        value = msg.substr(tagBegin);
        return true;
    }

    std::size_t tagEnd = msg.find(nextTag, tagBegin);
    if (tagEnd == std::string::npos || tagEnd == tagBegin) {
        return false;
    }

    value = msg.substr(tagBegin, tagEnd - tagBegin);
    return true;
}

void ExtractStrToLong(const std::string &msg, int64_t &result, const std::string &preTag,
    const std::string &nextTag, int64_t defaultValue)
{
    std::string value;
    if (ExtractSubTag(msg, value, preTag, nextTag)) {
        result = atoll(value.c_str());
    } else {
        LOGD("ExtractStrToLong error, preTag:%{public}s, nextTag:%{public}s", preTag.c_str(), nextTag.c_str());
        result = defaultValue;
    }
}
 
void ExtractStrToInt(const std::string &msg, int32_t &result, const std::string &preTag,
    const std::string &nextTag, int32_t defaultValue)
{
    std::string value;
    if (ExtractSubTag(msg, value, preTag, nextTag)) {
        result = atoi(value.c_str());
    } else {
        LOGD("ExtractStrToInt error, preTag:%{public}s, nextTag:%{public}s", preTag.c_str(), nextTag.c_str());
        result = defaultValue;
    }
}

void ExtractStrToInt16(const std::string &msg, int16_t &result, const std::string &preTag,
    const std::string &nextTag, int16_t defaultValue)
{
    std::string value;
    if (ExtractSubTag(msg, value, preTag, nextTag)) {
        result = static_cast<int16_t>(atoi(value.c_str()));
    } else {
        LOGD("ExtractStrToInt16 error, preTag:%{public}s, nextTag:%{public}s", preTag.c_str(), nextTag.c_str());
        result = defaultValue;
    }
}
 
void ExtractStrToStr(const std::string &msg, std::string &result,
    const std::string &preTag, const std::string &nextTag, const std::string& defaultValue)
{
    if (!ExtractSubTag(msg, result, preTag, nextTag)) {
        LOGD("ExtractStrToStr error, preTag:%{public}s, nextTag:%{public}s", preTag.c_str(), nextTag.c_str());
        result = defaultValue;
    }
}
} // namespace HiviewDFX
} // namespace OHOS