/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_CALCULATOR_IO_CALCULATOR_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_CALCULATOR_IO_CALCULATOR_H

#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class IoCalculator {
private:
    IoCalculator() = default;
    ~IoCalculator() = default;

public:
    static int64_t GetEMMCSize(const std::string &path);
    static std::string GetEMMCManfid(const std::string &path);
    static double PercentValue(uint64_t preValue, uint64_t currValue, uint64_t period);
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_CALCULATOR_IO_CALCULATOR_H
