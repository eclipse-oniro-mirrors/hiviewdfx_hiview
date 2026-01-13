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
#ifndef OHOS_HIVIEWDFX_UCOLLECTUTIL_COMMON_UTIL_H
#define OHOS_HIVIEWDFX_UCOLLECTUTIL_COMMON_UTIL_H

#include <cinttypes>
#include <sstream>
#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class CommonUtil {
private:
    CommonUtil() = default;
    ~CommonUtil() = default;

public:
    static bool ParseTypeAndValue(const std::string &str, std::string &type, int64_t &value);
    static int32_t ReadNodeWithOnlyNumber(const std::string& fileName);
    template <typename T> static bool StrToNum(const std::string& sString, T &tX)
    {
        std::istringstream iStream(sString);
        return (iStream >> tX) ? true : false;
    }
}; // CommonUtil
} // UCollectUtil
} // HiviewDFX
} // OHOS
#endif // OHOS_HIVIEWDFX_UCOLLECTUTIL_COMMON_UTIL_H
