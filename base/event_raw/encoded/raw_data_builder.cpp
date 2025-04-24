/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "encoded/raw_data_builder.h"

#include <cinttypes>
#include <securec.h>
#include <sstream>
#include <vector>

#include "decoded/decoded_event.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
DEFINE_LOG_TAG("HiView-RawDataBuilder");
RawDataBuilder::RawDataBuilder(std::shared_ptr<EventRaw::RawData> rawData)
{
    if (rawData == nullptr) {
        return;
    }
    EventRaw::DecodedEvent event(rawData->GetData(), rawData->GetDataLength());
    auto header = event.GetHeader();
    auto traceInfo = event.GetTraceInfo();
    AppendDomain(header.domain).AppendName(header.name).AppendType(static_cast<int>(header.type) + 1).
        AppendTimeStamp(header.timestamp).AppendTimeZone(header.timeZone).AppendLog(header.log).
        AppendUid(header.uid).AppendPid(header.pid).AppendTid(header.tid).AppendId(header.id);
    if (header.isTraceOpened == 1) {
        AppendTraceInfo(traceInfo.traceId, traceInfo.spanId, traceInfo.pSpanId, traceInfo.traceFlag);
    }
    InitValueParams(event.GetAllCustomizedValues());
}

RawDataBuilder::RawDataBuilder(const std::string& domain, const std::string& name, const int eventType)
{
    (void)AppendDomain(domain);
    (void)AppendName(name);
    (void)AppendType(eventType);
}

bool RawDataBuilder::BuildHeader(std::shared_ptr<RawData> dest)
{
    if (!dest->Append(reinterpret_cast<uint8_t*>(&header_), sizeof(struct HiSysEventHeader))) {
        HIVIEW_LOGE("Event header copy failed.");
        return false;
    }
    // append trace info
    if (header_.isTraceOpened == 1 &&
        !dest->Append(reinterpret_cast<uint8_t*>(&traceInfo_), sizeof(struct TraceInfo))) {
        HIVIEW_LOGE("Trace info copy failed.");
        return false;
    }
    return true;
}

bool RawDataBuilder::BuildCustomizedParams(std::shared_ptr<RawData> dest)
{
    std::lock_guard<std::mutex> lock(paramsOptMtx_);
    return !any_of(allParams_.begin(), allParams_.end(), [&dest] (auto& param) {
        auto rawData = param->GetRawData();
        return !dest->Append(rawData.GetData(), rawData.GetDataLength());
    });
}

std::shared_ptr<RawData> RawDataBuilder::Build()
{
    // placehold block size
    int32_t blockSize = 0;
    auto rawData = std::make_shared<RawData>();
    if (!rawData->Append(reinterpret_cast<uint8_t*>(&blockSize), sizeof(int32_t))) {
        HIVIEW_LOGE("Block size copy failed.");
        return nullptr;
    }
    if (!BuildHeader(rawData)) {
        HIVIEW_LOGE("Header of sysevent build failed.");
        return nullptr;
    }
    // append parameter count
    int32_t paramCnt = static_cast<int32_t>(GetParamCnt());
    if (!rawData->Append(reinterpret_cast<uint8_t*>(&paramCnt), sizeof(int32_t))) {
        HIVIEW_LOGE("Parameter count copy failed.");
        return rawData;
    }
    if (!BuildCustomizedParams(rawData)) {
        HIVIEW_LOGE("Customized paramters of sysevent build failed.");
        return rawData;
    }
    // update block size
    blockSize = static_cast<int32_t>(rawData->GetDataLength());
    if (!rawData->Update(reinterpret_cast<uint8_t*>(&blockSize), sizeof(int32_t), 0)) {
        HIVIEW_LOGE("Failed to update block size.");
    }
    return rawData;
}

bool RawDataBuilder::IsBaseInfo(const std::string& key)
{
    std::vector<const std::string> allBaseInfoKeys = {
        BASE_INFO_KEY_DOMAIN, BASE_INFO_KEY_NAME, BASE_INFO_KEY_TYPE, BASE_INFO_KEY_TIME_STAMP, BASE_INFO_KEY_LOG,
        BASE_INFO_KEY_TIME_ZONE, BASE_INFO_KEY_ID, BASE_INFO_KEY_PID, BASE_INFO_KEY_TID, BASE_INFO_KEY_UID,
        BASE_INFO_KEY_TRACE_ID, BASE_INFO_KEY_SPAN_ID, BASE_INFO_KEY_PARENT_SPAN_ID, BASE_INFO_KEY_TRACE_FLAG
    };
    return find(allBaseInfoKeys.begin(), allBaseInfoKeys.end(), key) != allBaseInfoKeys.end();
}

RawDataBuilder& RawDataBuilder::AppendDomain(const std::string& domain)
{
    auto ret = memcpy_s(header_.domain, MAX_DOMAIN_LENGTH + 1, domain.c_str(), domain.length());
    if (ret != EOK) {
        HIVIEW_LOGE("Failed to copy event domain, ret is %{public}d.", ret);
    }
    auto resetPos = std::min(domain.length(), static_cast<size_t>(MAX_DOMAIN_LENGTH));
    header_.domain[resetPos] = '\0';
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendName(const std::string& name)
{
    auto ret = memcpy_s(header_.name, MAX_EVENT_NAME_LENGTH + 1, name.c_str(), name.length());
    if (ret != EOK) {
        HIVIEW_LOGE("Failed to copy event name, ret is %{public}d.", ret);
    }
    auto resetPos = std::min(name.length(), static_cast<size_t>(MAX_EVENT_NAME_LENGTH));
    header_.name[resetPos] = '\0';
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendType(const int eventType)
{
    header_.type = static_cast<uint8_t>(eventType - 1); // header_.type is only 2 bits which must be
                                                       // subtracted 1 in order to avoid data overrflow.
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendTimeStamp(const uint64_t timestamp)
{
    header_.timestamp = timestamp;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendTimeZone(const std::string& timeZone)
{
    header_.timeZone = static_cast<uint8_t>(ParseTimeZone(timeZone));
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendTimeZone(const uint8_t timeZone)
{
    header_.timeZone = timeZone;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendUid(const uint32_t uid)
{
    header_.uid = uid;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendPid(const uint32_t pid)
{
    header_.pid = pid;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendTid(const uint32_t tid)
{
    header_.tid = tid;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendLog(const uint8_t log)
{
    header_.log = log;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendId(const uint64_t id)
{
    header_.id = id;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendId(const std::string& id)
{
    uint64_t u64Id = 0;
    std::stringstream ss(id);
    ss >> u64Id;
    AppendId(u64Id);
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendTraceId(const uint64_t traceId)
{
    header_.isTraceOpened = 1;
    traceInfo_.traceId = traceId;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendSpanId(const uint32_t spanId)
{
    header_.isTraceOpened = 1;
    traceInfo_.spanId = spanId;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendPSpanId(const uint32_t pSpanId)
{
    header_.isTraceOpened = 1;
    traceInfo_.pSpanId = pSpanId;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendTraceFlag(const uint8_t traceFlag)
{
    header_.isTraceOpened = 1;
    traceInfo_.traceFlag = traceFlag;
    return *this;
}

RawDataBuilder& RawDataBuilder::AppendTraceInfo(const uint64_t traceId, const uint32_t spanId,
    const uint32_t pSpanId, const uint8_t traceFlag)
{
    header_.isTraceOpened = 1; // 1: include trace info, 0: exclude trace info.

    traceInfo_.traceId = traceId;
    traceInfo_.spanId = spanId;
    traceInfo_.pSpanId = pSpanId;
    traceInfo_.traceFlag = traceFlag;

    return *this;
}

RawDataBuilder& RawDataBuilder::AppendValue(std::shared_ptr<EncodedParam> param)
{
    if (param == nullptr || !param->Encode()) {
        return *this;
    }
    auto paramKey = param->GetKey();
    std::lock_guard<std::mutex> lock(paramsOptMtx_);
    for (auto iter = allParams_.begin(); iter != allParams_.end(); ++iter) {
        if ((*iter) == nullptr) {
            continue;
        }
        if ((*iter)->GetKey() == paramKey) {
            *iter = param;
            return *this;
        }
    }
    allParams_.emplace_back(param);
    return *this;
}

RawDataBuilder& RawDataBuilder::RemoveParam(const std::string& paramName)
{
    std::lock_guard<std::mutex> lock(paramsOptMtx_);
    for (auto iter = allParams_.begin(); iter != allParams_.end(); ++iter) {
        if ((*iter) == nullptr) {
            continue;
        }
        if ((*iter)->GetKey() == paramName) {
            allParams_.erase(iter);
            break;
        }
    }
    return *this;
}

std::shared_ptr<EncodedParam> RawDataBuilder::GetValue(const std::string& key)
{
    std::lock_guard<std::mutex> lock(paramsOptMtx_);
    for (auto iter = allParams_.begin(); iter != allParams_.end(); ++iter) {
        if ((*iter) == nullptr) {
            continue;
        }
        if ((*iter)->GetKey() == key) {
            return *iter;
        }
    }
    return nullptr;
}

std::string RawDataBuilder::GetDomain()
{
    return std::string(header_.domain);
}

std::string RawDataBuilder::GetName()
{
    return std::string(header_.name);
}

int RawDataBuilder::GetEventType()
{
    return static_cast<int>(header_.type) + 1; // only 2 bits
}

size_t RawDataBuilder::GetParamCnt()
{
    std::lock_guard<std::mutex> lock(paramsOptMtx_);
    return allParams_.size();
}

struct HiSysEventHeader& RawDataBuilder::GetHeader()
{
    return header_;
}

struct TraceInfo& RawDataBuilder::GetTraceInfo()
{
    return traceInfo_;
}

void RawDataBuilder::InitValueParams(const std::vector<std::shared_ptr<DecodedParam>>& params)
{
    std::unordered_map<EventRaw::DataCodedType,
        std::function<void(std::shared_ptr<DecodedParam>)>> paramFuncs = {
        {EventRaw::DataCodedType::UNSIGNED_VARINT, [this] (std::shared_ptr<DecodedParam> param) {
                if (uint64_t val = 0; param->AsUint64(val)) {
                    this->AppendValue(std::make_shared<UnsignedVarintEncodedParam<uint64_t>>(param->GetKey(),
                        val));
                }
            }
        },
        {EventRaw::DataCodedType::SIGNED_VARINT, [this] (std::shared_ptr<DecodedParam> param) {
                if (int64_t val = 0; param->AsInt64(val)) {
                    this->AppendValue(std::make_shared<SignedVarintEncodedParam<int64_t>>(param->GetKey(),
                        val));
                }
            }
        },
        {EventRaw::DataCodedType::FLOATING, [this] (std::shared_ptr<DecodedParam> param) {
                if (double val = 0.0; param->AsDouble(val)) {
                    this->AppendValue(std::make_shared<FloatingNumberEncodedParam<double>>(param->GetKey(),
                        val));
                }
            }
        },
        {EventRaw::DataCodedType::DSTRING, [this] (std::shared_ptr<DecodedParam> param) {
                if (std::string val; param->AsString(val)) {
                    this->AppendValue(std::make_shared<StringEncodedParam>(param->GetKey(),
                        val));
                }
            }
        }
    };
    auto iter = paramFuncs.begin();
    for (const auto& param : params) {
        if (param == nullptr) {
            continue;
        }
        iter = paramFuncs.find(param->GetDataCodedType());
        if (iter == paramFuncs.end()) {
            continue;
        }
        iter->second(param);
    }
    InitArrayValueParams(params);
}

void RawDataBuilder::InitArrayValueParams(const std::vector<std::shared_ptr<DecodedParam>>& params)
{
    std::unordered_map<EventRaw::DataCodedType,
        std::function<void(std::shared_ptr<DecodedParam>)>> paramFuncs = {
        {EventRaw::DataCodedType::UNSIGNED_VARINT_ARRAY, [this] (std::shared_ptr<DecodedParam> param) {
                if (std::vector<uint64_t> vals; param->AsUint64Vec(vals)) {
                    this->AppendValue(std::make_shared<UnsignedVarintEncodedArrayParam<uint64_t>>(param->GetKey(),
                        vals));
                }
            }
        },
        {EventRaw::DataCodedType::SIGNED_VARINT_ARRAY, [this] (std::shared_ptr<DecodedParam> param) {
                if (std::vector<int64_t> vals; param->AsInt64Vec(vals)) {
                    this->AppendValue(std::make_shared<SignedVarintEncodedArrayParam<int64_t>>(param->GetKey(),
                        vals));
                }
            }
        },
        {EventRaw::DataCodedType::FLOATING_ARRAY, [this] (std::shared_ptr<DecodedParam> param) {
                if (std::vector<double> vals; param->AsDoubleVec(vals)) {
                    this->AppendValue(std::make_shared<FloatingNumberEncodedArrayParam<double>>(param->GetKey(),
                        vals));
                }
            }
        },
        {EventRaw::DataCodedType::DSTRING_ARRAY, [this] (std::shared_ptr<DecodedParam> param) {
                if (std::vector<std::string> vals; param->AsStringVec(vals)) {
                    this->AppendValue(std::make_shared<StringEncodedArrayParam>(param->GetKey(),
                        vals));
                }
            }
        }
    };
    auto iter = paramFuncs.begin();
    for (const auto& param : params) {
        if (param == nullptr) {
            continue;
        }
        iter = paramFuncs.find(param->GetDataCodedType());
        if (iter == paramFuncs.end()) {
            continue;
        }
        iter->second(param);
    }
}
} // namespace EventRaw
} // namespace HiviewDFX
} // namespace OHOS