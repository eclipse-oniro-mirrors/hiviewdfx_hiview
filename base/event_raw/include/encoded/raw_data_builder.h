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

#ifndef BASE_EVENT_RAW_ENCODE_INCLUDE_RAW_DATA_BUILDER_H
#define BASE_EVENT_RAW_ENCODE_INCLUDE_RAW_DATA_BUILDER_H

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "decoded/decoded_param.h"
#include "encoded/encoded_param.h"
#include "base/raw_data_base_def.h"
#include "base/raw_data.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
class RawDataBuilder {
public:
    RawDataBuilder() {};
    RawDataBuilder(std::shared_ptr<EventRaw::RawData> rawData);
    RawDataBuilder(const std::string& domain, const std::string& name, const int eventType);
    ~RawDataBuilder() = default;

public:
    std::shared_ptr<RawData> Build();
    std::shared_ptr<EncodedParam> GetValue(const std::string& key);
    struct HiSysEventHeader& GetHeader();
    struct TraceInfo& GetTraceInfo();
    std::string GetDomain();
    std::string GetName();
    int GetEventType();
    size_t GetParamCnt();
    bool IsBaseInfo(const std::string& key);

public:
    RawDataBuilder& AppendValue(std::shared_ptr<EncodedParam> param);
    RawDataBuilder& AppendDomain(const std::string& domain);
    RawDataBuilder& AppendName(const std::string& name);
    RawDataBuilder& AppendType(const int eventType);
    RawDataBuilder& AppendTimeStamp(const uint64_t timeStamp);
    RawDataBuilder& AppendTimeZone(const std::string& timeZone);
    RawDataBuilder& AppendTimeZone(const uint8_t timeZone);
    RawDataBuilder& AppendUid(const uint32_t uid);
    RawDataBuilder& AppendPid(const uint32_t pid);
    RawDataBuilder& AppendTid(const uint32_t tid);
    RawDataBuilder& AppendLog(const uint8_t log);
    RawDataBuilder& AppendId(const uint64_t id);
    RawDataBuilder& AppendId(const std::string& id);
    RawDataBuilder& AppendTraceId(const uint64_t traceId);
    RawDataBuilder& AppendSpanId(const uint32_t spanId);
    RawDataBuilder& AppendPSpanId(const uint32_t pSpanId);
    RawDataBuilder& AppendTraceFlag(const uint8_t traceFlag);
    RawDataBuilder& AppendTraceInfo(const uint64_t traceId, const uint32_t spanId,
        const uint32_t pSpanId, const uint8_t traceFlag);
    RawDataBuilder& RemoveParam(const std::string& paramName);

    template<typename T>
    bool ParseValueByKey(const std::string& key, T& dest)
    {
        if (IsBaseInfo(key)) {
            return GetBaseInfoValueByKey(key, dest);
        }
        return GetValueByKey(key, dest);
    }

    template<typename T>
    RawDataBuilder& AppendValue(const std::string& key, T val)
    {
        if (IsBaseInfo(key)) {
            return AppendBaseInfoValue(key, val);
        }
        if constexpr (isString<T>) {
            return AppendValue(std::make_shared<StringEncodedParam>(key, val));
        }
        if constexpr (isFloatingPointNum<T>) {
            return AppendValue(std::make_shared<FloatingNumberEncodedParam<double>>(key, static_cast<double>(val)));
        }
        if constexpr (std::is_same_v<std::decay_t<T>, bool> || isSignedNum<T>) {
            return AppendValue(std::make_shared<SignedVarintEncodedParam<int64_t>>(key, static_cast<int64_t>(val)));
        }
        if constexpr (isUnsignedNum<T>) {
            return AppendValue(std::make_shared<UnsignedVarintEncodedParam<uint64_t>>(key,
                static_cast<uint64_t>(val)));
        }
        return AppendArrayValue(key, val);
    }

private:
    template<typename T>
    RawDataBuilder& UpdateType(const T val)
    {
        if constexpr (isUnsignedNum<T>) {
            return AppendType(static_cast<int>(val));
        }
        return *this;
    }

    template<typename T>
    RawDataBuilder& UpdateUid(const T val)
    {
        if constexpr (isUnsignedNum<T>) {
            return AppendUid(static_cast<uint32_t>(val));
        }
        return *this;
    }

    template<typename T>
    RawDataBuilder& UpdateLog(const T val)
    {
        if constexpr (isUnsignedNum<T>) {
            return AppendLog(static_cast<uint8_t>(val));
        }
        return *this;
    }

    template<typename T>
    RawDataBuilder& UpdatePid(const T val)
    {
        if constexpr (isUnsignedNum<T>) {
            return AppendPid(static_cast<uint32_t>(val));
        }
        return *this;
    }

    template<typename T>
    RawDataBuilder& UpdateTid(const T val)
    {
        if constexpr (isUnsignedNum<T>) {
            return AppendTid(static_cast<uint32_t>(val));
        }
        return *this;
    }

    template<typename T>
    RawDataBuilder& UpdateId(const T val)
    {
        if constexpr (isString<T>) {
            return AppendId(val);
        }
        if constexpr (isUnsignedNum<T>) {
            return AppendId(static_cast<uint64_t>(val));
        }
        return *this;
    }

    template<typename T>
    void TransHexStrToNum(const std::string& hexStr, T& num)
    {
        std::stringstream ss;
        ss << std::hex << hexStr;
        ss >> num >> std::dec;
    }

    template<typename T>
    RawDataBuilder& UpdateTraceId(const T val)
    {
        if constexpr (isString<T>) {
            uint64_t traceId = 0;
            TransHexStrToNum(val, traceId);
            return AppendTraceId(traceId);
        }
        if constexpr (isUnsignedNum<T>) {
            return AppendTraceId(static_cast<uint64_t>(val));
        }
        return *this;
    }

    template<typename T>
    RawDataBuilder& UpdateSpanId(const T val)
    {
        if constexpr (isString<T>) {
            uint32_t spanId = 0;
            TransHexStrToNum(val, spanId);
            return AppendSpanId(spanId);
        }
        if constexpr (isUnsignedNum<T>) {
            return AppendSpanId(static_cast<uint32_t>(val));
        }
        return *this;
    }

    template<typename T>
    RawDataBuilder& UpdatePSpanId(const T val)
    {
        if constexpr (isString<T>) {
            uint32_t pSpanId = 0;
            TransHexStrToNum(val, pSpanId);
            return AppendPSpanId(pSpanId);
        }
        if constexpr (isUnsignedNum<T>) {
            return AppendPSpanId(static_cast<uint32_t>(val));
        }
        return *this;
    }

    template<typename T>
    RawDataBuilder& UpdateTraceFlag(const T val)
    {
        if constexpr (isUnsignedNum<T>) {
            return AppendTraceFlag(static_cast<uint8_t>(val));
        }
        return *this;
    }

    template<typename T>
    RawDataBuilder& AppendArrayValue(const std::string& key, T val)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, std::vector<std::string>>) {
            return AppendValue(std::make_shared<StringEncodedArrayParam>(key, val));
        }
        if constexpr (isFloatingPointNumArray<T>) {
            std::vector<double> dVector;
            for (auto item : val) {
                dVector.emplace_back(static_cast<double>(item));
            }
            return AppendValue(std::make_shared<FloatingNumberEncodedArrayParam<double>>(key, dVector));
        }
        if constexpr (std::is_same_v<std::decay_t<T>, std::vector<bool>> || isSignedNumArray<T>) {
            std::vector<int64_t> i64Vector;
            for (auto item : val) {
                i64Vector.emplace_back(static_cast<int64_t>(item));
            }
            return AppendValue(std::make_shared<SignedVarintEncodedArrayParam<int64_t>>(key, i64Vector));
        }
        if constexpr (isUnsignedNumArray<T>) {
            std::vector<uint64_t> i64Vector;
            for (auto item : val) {
                i64Vector.emplace_back(static_cast<uint64_t>(item));
            }
            return AppendValue(std::make_shared<UnsignedVarintEncodedArrayParam<uint64_t>>(key, i64Vector));
        }
        return *this;
    }

    template<typename T>
    void InitBaseAppendHandlers(std::unordered_map<std::string, std::function<RawDataBuilder&(T)>>& handlers)
    {
        handlers.emplace(BASE_INFO_KEY_DOMAIN, [this] (T val) -> decltype(auto) {
            if constexpr (isString<T>) {
                return this->AppendDomain(val);
            }
            return *this;
        });
        handlers.emplace(BASE_INFO_KEY_NAME, [this] (T val) -> decltype(auto) {
            if constexpr (isString<T>) {
                return this->AppendName(val);
            }
            return *this;
        });
        handlers.emplace(BASE_INFO_KEY_TYPE, [this] (T val) -> decltype(auto) {
            return this->UpdateType(val);
        });
        handlers.emplace(BASE_INFO_KEY_TIME_STAMP, [this] (T val) -> decltype(auto) {
            if constexpr (std::is_same_v<std::decay_t<T>, uint64_t>) {
                return this->AppendTimeStamp(val);
            }
            return *this;
        });
        handlers.emplace(BASE_INFO_KEY_TIME_ZONE, [this] (T val) -> decltype(auto) {
            if constexpr (isString<T>) {
                return this->AppendTimeZone(val);
            }
            return *this;
        });
    }

    template<typename T>
    void InitIdInfoAppendHandlers(std::unordered_map<std::string, std::function<RawDataBuilder&(T)>>& handlers)
    {
        handlers.emplace(BASE_INFO_KEY_ID, [this] (T val) -> decltype(auto) {
            return this->UpdateId(val);
        });
        handlers.emplace(BASE_INFO_KEY_PID, [this] (T val) -> decltype(auto) {
            return this->UpdatePid(val);
        });
        handlers.emplace(BASE_INFO_KEY_TID, [this] (T val) -> decltype(auto) {
            return this->UpdateTid(val);
        });
        handlers.emplace(BASE_INFO_KEY_UID, [this] (T val) -> decltype(auto) {
            return this->UpdateUid(val);
        });
        handlers.emplace(BASE_INFO_KEY_LOG, [this] (T val) -> decltype(auto) {
            return this->UpdateLog(val);
        });
        handlers.emplace(BASE_INFO_KEY_TRACE_ID, [this] (T val) -> decltype(auto) {
            return this->UpdateTraceId(val);
        });
        handlers.emplace(BASE_INFO_KEY_SPAN_ID, [this] (T val) -> decltype(auto) {
            return this->UpdateSpanId(val);
        });
        handlers.emplace(BASE_INFO_KEY_PARENT_SPAN_ID, [this] (T val) -> decltype(auto) {
            return this->UpdatePSpanId(val);
        });
        handlers.emplace(BASE_INFO_KEY_TRACE_FLAG, [this] (T val) -> decltype(auto) {
            return this->UpdateTraceFlag(val);
        });
    }

    template<typename T>
    RawDataBuilder& AppendBaseInfoValue(const std::string& key, T val)
    {
        std::unordered_map<std::string, std::function<RawDataBuilder&(T)>> appendHandlers;
        InitBaseAppendHandlers(appendHandlers);
        InitIdInfoAppendHandlers(appendHandlers);
        auto iter = appendHandlers.find(key);
        return (iter == appendHandlers.end()) ? *this : iter->second(val);
    }

    template<typename T>
    bool GetArrayValueByKey(std::shared_ptr<EncodedParam> encodedParam, T& val)
    {
        std::unordered_map<DataCodedType,
            std::function<bool(std::shared_ptr<EncodedParam>, T&)>> getFuncs = {
            {DataCodedType::UNSIGNED_VARINT_ARRAY, [] (std::shared_ptr<EncodedParam> param, T& val) {
                    if constexpr (std::is_same_v<std::decay_t<T>, std::vector<uint64_t>>) {
                        param->AsUint64Vec(val);
                        return true;
                    }
                    return false;
                }
            },
            {DataCodedType::SIGNED_VARINT_ARRAY, [] (std::shared_ptr<EncodedParam> param, T& val) {
                    if constexpr (std::is_same_v<std::decay_t<T>, std::vector<int64_t>>) {
                        param->AsInt64Vec(val);
                        return true;
                    }
                    return false;
                }
            },
            {DataCodedType::FLOATING_ARRAY, [] (std::shared_ptr<EncodedParam> param, T& val) {
                    if constexpr (std::is_same_v<std::decay_t<T>, std::vector<double>>) {
                        param->AsDoubleVec(val);
                        return true;
                    }
                    return false;
                }
            },
            {DataCodedType::DSTRING_ARRAY, [] (std::shared_ptr<EncodedParam> param, T& val) {
                    if constexpr (std::is_same_v<std::decay_t<T>, std::vector<std::string>>) {
                        param->AsStringVec(val);
                        return true;
                    }
                    return false;
                }
            }
        };
        auto iter = getFuncs.find(encodedParam->GetDataCodedType());
        return (iter == getFuncs.end()) ? false : iter->second(encodedParam, val);
    }

    template<typename T>
    bool GetValueByKey(const std::string& key, T& val)
    {
        auto encodedParam = GetValue(key);
        if (encodedParam == nullptr) {
            return false;
        }
        std::unordered_map<DataCodedType, std::function<bool(std::shared_ptr<EncodedParam>, T&)>> getFuncs = {
            {DataCodedType::UNSIGNED_VARINT, [] (std::shared_ptr<EncodedParam> param, T& val) {
                    if constexpr (std::is_same_v<std::decay_t<T>, uint64_t>) {
                        param->AsUint64(val);
                        return true;
                    }
                    return false;
                }
            },
            {DataCodedType::SIGNED_VARINT, [] (std::shared_ptr<EncodedParam> param, T& val) {
                    if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
                        param->AsInt64(val);
                        return true;
                    }
                    return false;
                }
            },
            {DataCodedType::FLOATING, [] (std::shared_ptr<EncodedParam> param, T& val) {
                    if constexpr (std::is_same_v<std::decay_t<T>, double>) {
                        param->AsDouble(val);
                        return true;
                    }
                    return false;
                }
            },
            {DataCodedType::DSTRING, [] (std::shared_ptr<EncodedParam> param, T& val) {
                    if constexpr (isString<T>) {
                        param->AsString(val);
                        return true;
                    }
                    return false;
                }
            }
        };
        auto iter = getFuncs.find(encodedParam->GetDataCodedType());
        return (iter == getFuncs.end()) ? GetArrayValueByKey(encodedParam, val) : iter->second(encodedParam, val);\
    }

    template<typename T, typename V>
    bool ParseAndSetTraceInfo(T& to, V from)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            to = TransNumToHexStr(from);
            return true;
        }
        return ParseValue(to, from);
    }

    template<typename T>
    bool GetBaseInfoValueByKey(const std::string& key, T& val)
    {
        std::unordered_map<std::string, std::function<bool(T&)>> parseFuncs;
        parseFuncs.emplace(BASE_INFO_KEY_DOMAIN, [this] (T& val) -> bool {
            return this->ParseValue(val, std::string(header_.domain));
        });
        parseFuncs.emplace(BASE_INFO_KEY_NAME, [this] (T& val) -> bool {
            return this->ParseValue(val, std::string(header_.name));
        });
        parseFuncs.emplace(BASE_INFO_KEY_TYPE, [this] (T& val) -> bool {
            int type = static_cast<int>(header_.type) + 1;
            return this->ParseValue(val, type);
        });
        parseFuncs.emplace(BASE_INFO_KEY_TIME_STAMP, [this] (T& val) -> bool {
            return this->ParseValue(val, header_.timestamp);
        });
        parseFuncs.emplace(BASE_INFO_KEY_TIME_ZONE, [this] (T& val) -> bool {
            return this->ParseTimeZoneFromHeader(val);
        });
        parseFuncs.emplace(BASE_INFO_KEY_ID, [this] (T& val) -> bool {
            return this->ParseValue(val, header_.id);
        });
        parseFuncs.emplace(BASE_INFO_KEY_PID, [this] (T& val) -> bool {
            return this->ParseValue(val, header_.pid);
        });
        parseFuncs.emplace(BASE_INFO_KEY_TID, [this] (T& val) -> bool {
            return this->ParseValue(val, header_.tid);
        });
        parseFuncs.emplace(BASE_INFO_KEY_UID, [this] (T& val) -> bool {
            return this->ParseValue(val, header_.uid);
        });
        parseFuncs.emplace(BASE_INFO_KEY_LOG, [this] (T& val) -> bool {
            return this->ParseValue(val, header_.log);
        });
        parseFuncs.emplace(BASE_INFO_KEY_TRACE_ID, [this] (T& val) -> bool {
            return this->ParseAndSetTraceInfo(val, traceInfo_.traceId);
        });
        parseFuncs.emplace(BASE_INFO_KEY_SPAN_ID, [this] (T& val) -> bool {
            return this->ParseAndSetTraceInfo(val, traceInfo_.spanId);
        });
        parseFuncs.emplace(BASE_INFO_KEY_PARENT_SPAN_ID, [this] (T& val) -> bool {
            return this->ParseAndSetTraceInfo(val, traceInfo_.pSpanId);
        });
        parseFuncs.emplace(BASE_INFO_KEY_TRACE_FLAG, [this] (T& val) -> bool {
            return this->PareTraceFlagFromHeader(val);
        });
        auto iter = parseFuncs.find(key);
        return (iter == parseFuncs.end()) ? false : iter->second(val);
    }

    template<typename T, typename V>
    bool ParseValue(T& to, V from)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<V>>) {
            to = from;
            return true;
        }
        if constexpr ((isSignedNum<T> && isSignedNum<V>) || (isUnsignedNum<T> && isUnsignedNum<V>)) {
            to = static_cast<std::decay_t<T>>(from);
            return true;
        }
        return false;
    }

    template<typename T>
    bool ParseTimeZoneFromHeader(T& val)
    {
        if constexpr (isUnsignedNum<T>) {
            val = static_cast<std::decay_t<T>>(header_.timeZone);
            return true;
        }
        if constexpr (isString<T>) {
            val = ParseTimeZone(header_.timeZone);
            return true;
        }
        return false;
    }

    template<typename T>
    bool PareTraceFlagFromHeader(T& val)
    {
        if constexpr (isUnsignedNum<T>) {
            val = static_cast<std::decay_t<T>>(traceInfo_.traceFlag);
            return true;
        }
        return false;
    }

    bool BuildHeader(std::shared_ptr<RawData> dest);
    bool BuildCustomizedParams(std::shared_ptr<RawData> dest);
    void InitValueParams(const std::vector<std::shared_ptr<DecodedParam>>& params);
    void InitArrayValueParams(const std::vector<std::shared_ptr<DecodedParam>>& params);

private:
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
    std::list<std::shared_ptr<EncodedParam>> allParams_;
    std::mutex paramsOptMtx_;
};
extern template void RawDataBuilder::InitIdInfoAppendHandlers<char const*>(
    std::unordered_map<std::string, std::function<RawDataBuilder&(char const*)>>&);
extern template void RawDataBuilder::InitIdInfoAppendHandlers<std::string>(
    std::unordered_map<std::string, std::function<RawDataBuilder&(std::string)>>&);
extern template void RawDataBuilder::InitIdInfoAppendHandlers<uint64_t>(
    std::unordered_map<std::string, std::function<RawDataBuilder&(uint64_t)>>&);
extern template RawDataBuilder& RawDataBuilder::AppendValue<double>(const std::string&, double);
extern template RawDataBuilder& RawDataBuilder::AppendValue<std::string>(const std::string&, std::string);
extern template RawDataBuilder& RawDataBuilder::AppendValue<int64_t>(const std::string&, int64_t);
extern template RawDataBuilder& RawDataBuilder::AppendValue<uint64_t>(const std::string&, uint64_t);
} // namespace EventRaw
} // namespace HiviewDFX
} // namespace OHOS

#endif // BASE_EVENT_RAW_ENCODE_INCLUDE_RAW_DATA_BUILDER_H
