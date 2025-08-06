/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_EVENT_REPORT_PARAM_VALUE_H
#define HIVIEW_BASE_EVENT_REPORT_PARAM_VALUE_H

#include <string>
#include <variant>
#include <vector>

namespace OHOS {
namespace HiviewDFX {
constexpr uint8_t DEFAULT_UINT8 = 0;
constexpr uint16_t DEFAULT_UINT16 = 0;
constexpr uint32_t DEFAULT_UINT32 = 0;
constexpr uint64_t DEFAULT_UINT64 = 0;
inline constexpr char DEFAULT_STRING[] = "";
inline const std::vector<uint32_t> DEFAULT_UINT32_VEC = {};
inline const std::vector<std::string> DEFAULT_STRING_VEC = {};

class ParamValue {
public:
    ParamValue();
    ParamValue(uint8_t value);
    ParamValue(uint16_t value);
    ParamValue(uint32_t value);
    ParamValue(uint64_t value);
    ParamValue(const std::string& value);
    ParamValue(const std::vector<uint32_t>& value);
    ParamValue(const std::vector<std::string>& value);

    size_t GetType() const;

    uint8_t GetUint8() const;
    uint16_t GetUint16() const;
    uint32_t GetUint32() const;
    uint64_t GetUint64() const;
    std::string GetString() const;
    std::vector<uint32_t> GetUint32Vec() const;
    std::vector<std::string> GetStringVec() const;

    bool IsUint8() const;
    bool IsUint16() const;
    bool IsUint32() const;
    bool IsUint64() const;
    bool IsString() const;
    bool IsUint32Vec() const;
    bool IsStringVec() const;

private:
    std::variant<uint8_t, uint16_t, uint32_t, uint64_t, std::string,
        std::vector<uint32_t>, std::vector<std::string>> value_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_EVENT_REPORT_PARAM_VALUE_H
