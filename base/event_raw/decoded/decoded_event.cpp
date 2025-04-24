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

#include "decoded/decoded_event.h"

#include <functional>
#include <securec.h>
#include <sstream>
#include <unordered_map>

#include "base/raw_data_base_def.h"
#include "decoded/raw_data_decoder.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
DEFINE_LOG_TAG("HiView-DecodedEvent");
namespace {
constexpr size_t MAX_BLOCK_SIZE = 384 * 1024; // 384K
constexpr size_t MAX_PARAM_CNT = 128 + 10; // 128 for Write, 10 for hiview

std::string TransUInt64ToFixedLengthStr(uint64_t src)
{
    const size_t maxIdLen = 20;
    std::string uint64Str = std::to_string(src);
    if (uint64Str.size() >= maxIdLen) {
        return uint64Str;
    }
    std::string dest(maxIdLen, '0');
    dest.replace(maxIdLen - uint64Str.size(), uint64Str.size(), uint64Str);
    return dest;
}
}

DecodedEvent::DecodedEvent(uint8_t* data, size_t len)
{
    if (data == nullptr || len < sizeof(int32_t)) {
        HIVIEW_LOGE("raw data is is invalid.");
        return;
    }
    size_t blockSize = static_cast<size_t>(*(reinterpret_cast<int32_t*>(data)));
    if (blockSize != len || blockSize < GetValidDataMinimumByteCount() || blockSize > MAX_BLOCK_SIZE) {
        HIVIEW_LOGE("size of raw data is %{public}zu, which is invalid.", blockSize);
        return;
    }
    rawData_ = new(std::nothrow) uint8_t[blockSize];
    if (rawData_ == nullptr) {
        return;
    }
    auto ret = memcpy_s(rawData_, blockSize, data, blockSize);
    if (ret != EOK) {
        HIVIEW_LOGE("Decode memory copy failed, ret is %{public}d.", ret);
        delete[] rawData_;
        return;
    }
    Parse();
}

DecodedEvent::~DecodedEvent()
{
    if (rawData_ != nullptr) {
        delete[] rawData_;
        rawData_ = nullptr;
    }
}

void DecodedEvent::AppendBaseInfo(std::stringstream& ss)
{
    size_t domainArraySize = MAX_DOMAIN_LENGTH + 1;
    char* domain = new(std::nothrow) char[domainArraySize];
    if (domain == nullptr) {
        return;
    }
    if (memcpy_s(domain, domainArraySize, header_.domain, MAX_DOMAIN_LENGTH) != EOK) {
        delete[] domain;
        return;
    }
    domain[MAX_DOMAIN_LENGTH] = '\0';
    auto eventDomain = std::string(domain);
    AppendValue(ss, BASE_INFO_KEY_DOMAIN, eventDomain);
    delete[] domain;
    size_t nameArraySize = MAX_EVENT_NAME_LENGTH + 1;
    char* name = new(std::nothrow) char[nameArraySize];
    if (name == nullptr) {
        return;
    }
    if (memcpy_s(name, nameArraySize, header_.name, MAX_EVENT_NAME_LENGTH) != EOK) {
        delete[] name;
        return;
    }
    name[MAX_EVENT_NAME_LENGTH] = '\0';
    auto eventName = std::string(name);
    AppendValue(ss, BASE_INFO_KEY_NAME, eventName);
    delete[] name;
    AppendValue(ss, BASE_INFO_KEY_TYPE, (static_cast<int>(header_.type) + 1)); // header_.type is only 2 bits which has
                                                                          // been subtracted 1 before wrote.
    AppendValue(ss, BASE_INFO_KEY_TIME_STAMP, header_.timestamp);
    auto timeZone = ParseTimeZone(static_cast<size_t>(header_.timeZone));
    AppendValue(ss, BASE_INFO_KEY_TIME_ZONE, timeZone);
    AppendValue(ss, BASE_INFO_KEY_PID, header_.pid);
    AppendValue(ss, BASE_INFO_KEY_TID, header_.tid);
    AppendValue(ss, BASE_INFO_KEY_UID, header_.uid);
    AppendValue(ss, BASE_INFO_KEY_LOG, static_cast<uint32_t>(header_.log));
    AppendValue(ss, BASE_INFO_KEY_ID, TransUInt64ToFixedLengthStr(header_.id));
    if (header_.isTraceOpened == 1) {
        AppendValue(ss, BASE_INFO_KEY_TRACE_FLAG, static_cast<int>(traceInfo_.traceFlag));
        AppendValue(ss, BASE_INFO_KEY_TRACE_ID, TransNumToHexStr(traceInfo_.traceId));
        AppendValue(ss, BASE_INFO_KEY_SPAN_ID, TransNumToHexStr(traceInfo_.spanId));
        AppendValue(ss, BASE_INFO_KEY_PARENT_SPAN_ID, TransNumToHexStr(traceInfo_.pSpanId));
    }
}

void DecodedEvent::AppendCustomizedArrayParam(std::stringstream& ss, std::shared_ptr<DecodedParam> param)
{
    std::unordered_map<DataCodedType, std::function<void(std::shared_ptr<DecodedParam>)>> allFuncs = {
        {DataCodedType::UNSIGNED_VARINT_ARRAY, [this, &ss](std::shared_ptr<DecodedParam> param) {
                std::vector<uint64_t> u64Vec;
                if (param->AsUint64Vec(u64Vec)) {
                    this->AppendValue(ss, param->GetKey(), u64Vec);
                }
            }
        },
        {DataCodedType::SIGNED_VARINT_ARRAY, [this, &ss](std::shared_ptr<DecodedParam> param) {
                std::vector<int64_t> i64Vec;
                if (param->AsInt64Vec(i64Vec)) {
                    this->AppendValue(ss, param->GetKey(), i64Vec);
                }
            }
        },
        {DataCodedType::FLOATING_ARRAY, [this, &ss](std::shared_ptr<DecodedParam> param) {
                std::vector<double> dVec;
                if (param->AsDoubleVec(dVec)) {
                    this->AppendValue(ss, param->GetKey(), dVec);
                }
            }
        },
        {DataCodedType::DSTRING_ARRAY, [this, &ss](std::shared_ptr<DecodedParam> param) {
                std::vector<std::string> strVec;
                if (param->AsStringVec(strVec)) {
                    this->AppendValue(ss, param->GetKey(), strVec);
                }
            }
        }
    };
    auto iter = allFuncs.find(param->GetDataCodedType());
    if (iter == allFuncs.end()) {
        return;
    }
    iter->second(param);
}

void DecodedEvent::AppendCustomizedParam(std::stringstream& ss, std::shared_ptr<DecodedParam> param)
{
    std::unordered_map<DataCodedType, std::function<void(std::shared_ptr<DecodedParam>)>> allFuncs = {
        {DataCodedType::UNSIGNED_VARINT, [this, &ss](std::shared_ptr<DecodedParam> param) {
                uint64_t uint64DecodedVal;
                if (param->AsUint64(uint64DecodedVal)) {
                    this->AppendValue(ss, param->GetKey(), uint64DecodedVal);
                }
            }
        },
        {DataCodedType::SIGNED_VARINT, [this, &ss](std::shared_ptr<DecodedParam> param) {
                int64_t int64DecodedVal;
                if (param->AsInt64(int64DecodedVal)) {
                    this->AppendValue(ss, param->GetKey(), int64DecodedVal);
                }
            }
        },
        {DataCodedType::FLOATING, [this, &ss](std::shared_ptr<DecodedParam> param) {
                double dDecodedVal;
                if (param->AsDouble(dDecodedVal)) {
                    this->AppendValue(ss, param->GetKey(), dDecodedVal);
                }
            }
        },
        {DataCodedType::DSTRING, [this, &ss](std::shared_ptr<DecodedParam> param) {
                std::string strDecodedVal;
                if (param->AsString(strDecodedVal)) {
                    this->AppendValue(ss, param->GetKey(), strDecodedVal);
                }
            }
        }
    };
    auto iter = allFuncs.find(param->GetDataCodedType());
    if (iter == allFuncs.end()) {
        return;
    }
    iter->second(param);
}

void DecodedEvent::AppendCustomizedParams(std::stringstream& ss)
{
    for (auto param: allParams_) {
        if (param == nullptr) {
            continue;
        }
        std::vector<DataCodedType> noArrayEncodedTypes = {
            DataCodedType::UNSIGNED_VARINT,
            DataCodedType::SIGNED_VARINT,
            DataCodedType::FLOATING,
            DataCodedType::DSTRING,
        };
        if (find(noArrayEncodedTypes.begin(), noArrayEncodedTypes.end(), param->GetDataCodedType()) !=
            noArrayEncodedTypes.end()) {
            AppendCustomizedParam(ss, param);
            continue;
        }
        AppendCustomizedArrayParam(ss, param);
    }
}

std::string DecodedEvent::AsJsonStr()
{
    std::stringstream jsonStream;
    jsonStream << "{";
    AppendBaseInfo(jsonStream);
    AppendCustomizedParams(jsonStream);
    if (jsonStream.tellp() != 0) {
        jsonStream.seekp(-1, std::ios_base::end);
    }
    jsonStream << "}";
    return jsonStream.str();
}

std::shared_ptr<RawData> DecodedEvent::GetRawData()
{
    return std::make_shared<RawData>(rawData_, pos_);
}

bool DecodedEvent::IsValid()
{
    return isValid_;
}

const struct HiSysEventHeader& DecodedEvent::GetHeader()
{
    return header_;
}

const struct TraceInfo& DecodedEvent::GetTraceInfo()
{
    return traceInfo_;
}

const std::vector<std::shared_ptr<DecodedParam>>& DecodedEvent::GetAllCustomizedValues()
{
    return allParams_;
}

void DecodedEvent::Parse()
{
    isValid_ = true;
    if (rawData_ == nullptr) {
        isValid_ = false;
        return;
    }
    pos_ = 0; // reset to 0
    // decode block size
    size_t blockSize = static_cast<size_t>(*(reinterpret_cast<int32_t*>(rawData_)));
    pos_ += sizeof(int32_t);
    ParseHeader(blockSize);
    ParseCustomizedParams(blockSize);
}

void DecodedEvent::ParseHeader(const size_t maxLen)
{
    // decode event header
    if ((pos_ + sizeof(struct HiSysEventHeader)) > maxLen) {
        isValid_ = false;
        return;
    }
    header_ = *(reinterpret_cast<struct HiSysEventHeader*>(rawData_ + pos_));
    pos_ += sizeof(struct HiSysEventHeader);
    // decode trace info
    if (header_.isTraceOpened == 1) { // 1: include trace info, 0: exclude trace info
        if (((pos_ + sizeof(struct TraceInfo)) > maxLen)) {
            isValid_ = false;
            return;
        }
        traceInfo_ = *(reinterpret_cast<struct TraceInfo*>(rawData_ + pos_));
        pos_ += sizeof(struct TraceInfo);
    }
}

void DecodedEvent::ParseCustomizedParams(const size_t maxLen)
{
    if ((pos_ + sizeof(int32_t)) > maxLen) {
        isValid_ = false;
        return;
    }
    auto paramCnt = static_cast<size_t>(*(reinterpret_cast<int32_t*>(rawData_ + pos_)));
    if (paramCnt > MAX_PARAM_CNT) {
        HIVIEW_LOGW("invalid param cnt=%{public}zu.", paramCnt);
        isValid_ = false;
        return;
    }
    pos_ += sizeof(int32_t);
    while (paramCnt > 0) {
        auto decodedParam = ParseCustomizedParam(maxLen);
        if (decodedParam == nullptr || !(decodedParam->DecodeValue())) {
            HIVIEW_LOGE("Value of customized parameter is decoded failed.");
            isValid_ = false;
            return;
        }
        pos_ = decodedParam->GetPosition();
        allParams_.emplace_back(decodedParam);
        --paramCnt;
    }
}

std::shared_ptr<DecodedParam> DecodedEvent::CreateFloatingNumTypeDecodedParam(const size_t maxLen,
    const std::string& key, bool isArray)
{
    if (isArray) {
        return std::make_shared<FloatingNumberDecodedArrayParam>(rawData_, maxLen, pos_, key);
    }
    return std::make_shared<FloatingNumberDecodedParam>(rawData_, maxLen, pos_, key);
}

std::shared_ptr<DecodedParam> DecodedEvent::CreateSignedVarintTypeDecodedParam(const size_t maxLen,
    const std::string& key, bool isArray)
{
    if (isArray) {
        return std::make_shared<SignedVarintDecodedArrayParam>(rawData_, maxLen, pos_, key);
    }
    return std::make_shared<SignedVarintDecodedParam>(rawData_, maxLen, pos_, key);
}

std::shared_ptr<DecodedParam> DecodedEvent::CreateStringTypeDecodedParam(const size_t maxLen,
    const std::string& key, bool isArray)
{
    if (isArray) {
        return std::make_shared<StringDecodedArrayParam>(rawData_, maxLen, pos_, key);
    }
    return std::make_shared<StringDecodedParam>(rawData_, maxLen, pos_, key);
}

std::shared_ptr<DecodedParam> DecodedEvent::CreateUnsignedVarintTypeDecodedParam(const size_t maxLen,
    const std::string& key, bool isArray)
{
    if (isArray) {
        return std::make_shared<UnsignedVarintDecodedArrayParam>(rawData_, maxLen, pos_, key);
    }
    return std::make_shared<UnsignedVarintDecodedParam>(rawData_, maxLen, pos_, key);
}

std::shared_ptr<DecodedParam> DecodedEvent::ParseCustomizedParam(const size_t maxLen)
{
    std::string key;
    if (!RawDataDecoder::StringValueDecoded(rawData_, maxLen, pos_, key)) {
        isValid_ = false;
        return nullptr;
    }
    struct ParamValueType valueType {
        .isArray = 0,
        .valueType = static_cast<uint8_t>(ValueType::UNKNOWN),
        .valueByteCnt = 0,
    };
    if (!RawDataDecoder::ValueTypeDecoded(rawData_, maxLen, pos_, valueType)) {
        isValid_ = false;
        return nullptr;
    }
    std::shared_ptr<DecodedParam> ret = nullptr;
    switch (ValueType(valueType.valueType)) {
        case ValueType::STRING: {
            ret = CreateStringTypeDecodedParam(maxLen, key, valueType.isArray == 1);
            break;
        }
        case ValueType::FLOAT:
        case ValueType::DOUBLE: {
            ret = CreateFloatingNumTypeDecodedParam(maxLen, key, valueType.isArray == 1);
            break;
        }
        case ValueType::UINT8:
        case ValueType::UINT16:
        case ValueType::UINT32:
        case ValueType::UINT64: {
            ret = CreateUnsignedVarintTypeDecodedParam(maxLen, key, valueType.isArray == 1);
            break;
        }
        case ValueType::BOOL:
        case ValueType::INT8:
        case ValueType::INT16:
        case ValueType::INT32:
        case ValueType::INT64: {
            ret = CreateSignedVarintTypeDecodedParam(maxLen, key, valueType.isArray == 1);
            break;
        }
        default:
            break;
    }
    return ret;
}
} // namespace EventRaw
} // namespace HiviewDFX
} // namespace OHOS