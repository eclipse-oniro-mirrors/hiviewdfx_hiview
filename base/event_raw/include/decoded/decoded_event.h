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

#ifndef BASE_EVENT_RAW_DECODE_INCLUDE_DECODED_EVENT_H
#define BASE_EVENT_RAW_DECODE_INCLUDE_DECODED_EVENT_H

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "base/raw_data_base_def.h"
#include "base/raw_data.h"
#include "decoded_param.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
class DecodedEvent {
public:
    DecodedEvent(uint8_t* data, size_t len);
    ~DecodedEvent();

public:
    std::string AsJsonStr();
    std::shared_ptr<RawData> GetRawData();
    bool IsValid();
    const struct HiSysEventHeader& GetHeader();
    const struct TraceInfo& GetTraceInfo();
    const std::vector<std::shared_ptr<DecodedParam>>& GetAllCustomizedValues();

private:
    void Parse();
    void ParseHeader(const size_t maxLen);
    void ParseCustomizedParams(const size_t maxLen);
    std::shared_ptr<DecodedParam> ParseCustomizedParam(const size_t maxLen);

private:
    template<typename T>
    void AppendValue(std::stringstream& ss, const std::string& key, T val)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            ss << "\"" << key << "\":\"" << val << "\",";
            return;
        }
        if constexpr (std::is_same_v<std::decay_t<T>, bool> ||
            std::is_same_v<std::decay_t<T>, int8_t> ||
            std::is_same_v<std::decay_t<T>, uint8_t> ||
            std::is_same_v<std::decay_t<T>, int16_t> ||
            std::is_same_v<std::decay_t<T>, uint16_t> ||
            std::is_same_v<std::decay_t<T>, int32_t> ||
            std::is_same_v<std::decay_t<T>, uint32_t> ||
            std::is_same_v<std::decay_t<T>, int64_t> ||
            std::is_same_v<std::decay_t<T>, uint64_t> ||
            std::is_same_v<std::decay_t<T>, float> ||
            std::is_same_v<std::decay_t<T>, double>) {
            ss << "\"" << key << "\":" << val << ",";
            return;
        }
        AppendArrayValue(ss, key, val);
    }

    template<typename T>
    void AppendArrayValue(std::stringstream& ss, const std::string& key, T vals)
    {
        ss << "\"" << key << "\":";
        ss << "[";
        bool arrayIsNotEmpty = false;
        if constexpr (std::is_same_v<std::decay_t<T>, std::vector<std::string>>) {
            arrayIsNotEmpty = (vals.size() > 0);
            for (auto item : vals) {
                ss << "\"" << item << "\",";
            }
        }
        if constexpr (std::is_same_v<std::decay_t<T>, std::vector<bool>> ||
            std::is_same_v<std::decay_t<T>, std::vector<int8_t>> ||
            std::is_same_v<std::decay_t<T>, std::vector<uint8_t>> ||
            std::is_same_v<std::decay_t<T>, std::vector<int16_t>> ||
            std::is_same_v<std::decay_t<T>, std::vector<uint16_t>> ||
            std::is_same_v<std::decay_t<T>, std::vector<int32_t>> ||
            std::is_same_v<std::decay_t<T>, std::vector<uint32_t>> ||
            std::is_same_v<std::decay_t<T>, std::vector<int64_t>> ||
            std::is_same_v<std::decay_t<T>, std::vector<uint64_t>> ||
            std::is_same_v<std::decay_t<T>, std::vector<float>> ||
            std::is_same_v<std::decay_t<T>, std::vector<double>>) {
            arrayIsNotEmpty = (vals.size() > 0);
            for (auto item : vals) {
                ss << "" << item << ",";
            }
        }
        if (ss.tellp() != 0 && arrayIsNotEmpty) {
            ss.seekp(-1, std::ios_base::end);
        }
        ss << "],";
    }

    void AppendBaseInfo(std::stringstream& ss);
    void AppendCustomizedArrayParam(std::stringstream& ss, std::shared_ptr<DecodedParam> param);
    void AppendCustomizedParam(std::stringstream& ss, std::shared_ptr<DecodedParam> param);
    void AppendCustomizedParams(std::stringstream& ss);

private:
    std::shared_ptr<DecodedParam> CreateFloatingNumTypeDecodedParam(const size_t maxLen, const std::string& key,
        bool isArray);
    std::shared_ptr<DecodedParam> CreateSignedVarintTypeDecodedParam(const size_t maxLen, const std::string& key,
        bool isArray);
    std::shared_ptr<DecodedParam> CreateStringTypeDecodedParam(const size_t maxLen, const std::string& key,
        bool isArray);
    std::shared_ptr<DecodedParam> CreateUnsignedVarintTypeDecodedParam(const size_t maxLen, const std::string& key,
        bool isArray);

private:
    uint8_t* rawData_ = nullptr;
    size_t pos_ = 0;
    struct HiSysEventHeader header_ = {
        .domain = {0},
        .name = {0},
        .timestamp = 0,
        .timeZone = 0,
        .uid = 0,
        .pid = 0,
        .tid = 0,
        .id = 0,
        .type = 0,
        .isTraceOpened = 0,
        .log = 0,
    };
    struct TraceInfo traceInfo_ {
        .traceFlag = 0,
        .traceId = 0,
        .spanId = 0,
        .pSpanId = 0,
    };
    bool isValid_ = false;
    std::vector<std::shared_ptr<DecodedParam>> allParams_;
};
} // namespace EventRaw
} // namespace HiviewDFX
} // namespace OHOS

#endif // BASE_EVENT_RAW_DECODE_INCLUDE_DECODED_EVENT_H
