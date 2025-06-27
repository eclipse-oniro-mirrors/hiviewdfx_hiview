/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "sys_event.h"

#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <vector>

#include "encoded/raw_data_builder_json_parser.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
std::string EventCol::DOMAIN = "domain_";
std::string EventCol::NAME = "name_";
std::string EventCol::TYPE = "type_";
std::string EventCol::TS = "time_";
std::string EventCol::TZ = "tz_";
std::string EventCol::PID = "pid_";
std::string EventCol::TID = "tid_";
std::string EventCol::UID = "uid_";
std::string EventCol::INFO = "info_";
std::string EventCol::LEVEL = "level_";
std::string EventCol::SEQ = "seq_";
std::string EventCol::TAG = "tag_";
}
using EventRaw::HiSysEventHeader;
namespace {
constexpr int64_t DEFAULT_INT_VALUE = 0;
constexpr uint64_t DEFAULT_UINT_VALUE = 0;
constexpr double DEFAULT_DOUBLE_VALUE = 0.0;
constexpr size_t BLOCK_SIZE_OFFSET = sizeof(int32_t);
constexpr size_t PERIOD_SEQ_INFO_ITEM_CNT = 3;
constexpr char REPORT_INTERVAL_KEY[] = "reportInterval_";

template<typename T>
void AppendJsonValue(std::string& eventJson, const std::string& key, T val)
{
    if (eventJson.empty()) {
        return;
    }
    std::string findKey = "\"" + key + "\":";
    if (eventJson.find(findKey) != std::string::npos) {
        return;
    }
    std::string appendStr;
    appendStr.append(",\"").append(key).append("\":");
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        appendStr.append("\"").append(val).append("\"");
    } else {
        appendStr.append(std::to_string(val));
    }
    eventJson.insert(eventJson.size() - 1, appendStr); // 1 for '}'
}

template<typename T>
bool ParseArrayValue(std::shared_ptr<EventRaw::RawDataBuilder> builder, const std::string& key,
    std::function<bool(T&)> itemHandler)
{
    if (builder == nullptr) {
        return false;
    }
    if (std::vector<T> arr; builder->ParseValueByKey(key, arr)) {
        if (arr.empty()) {
            return true;
        }
        return all_of(arr.begin(), arr.end(), [&itemHandler] (T& item) {
            return itemHandler(item);
        });
    }
    return false;
}
}
using EventRaw::UnsignedVarintEncodedParam;
using EventRaw::SignedVarintEncodedParam;
using EventRaw::FloatingNumberEncodedParam;
using EventRaw::StringEncodedParam;
using EventRaw::UnsignedVarintEncodedArrayParam;
using EventRaw::SignedVarintEncodedArrayParam;
using EventRaw::FloatingNumberEncodedArrayParam;
using EventRaw::StringEncodedArrayParam;
std::atomic<uint32_t> SysEvent::totalCount_(0);
std::atomic<int64_t> SysEvent::totalSize_(0);

SysEvent::SysEvent(const std::string& sender, PipelineEventProducer* handler,
    std::shared_ptr<EventRaw::RawData> rawData, int64_t seq,
    const std::string& sysVersion, const std::string& patchVersion)
    : PipelineEvent(sender, handler), eventType_(0), preserve_(true), log_(0),  seq_(seq)
{
    messageType_ = Event::MessageType::SYS_EVENT;
    if (rawData == nullptr || rawData->GetData() == nullptr ||
        rawData->GetDataLength() < EventRaw::GetValidDataMinimumByteCount()) {
        return;
    }
    sysVersion_ = sysVersion;
    patchVersion_ = patchVersion;
    rawData_ = rawData;
    totalCount_.fetch_add(1);
    totalSize_.fetch_add(*(reinterpret_cast<int32_t*>(rawData_->GetData())));
    InitialMembers();
}

SysEvent::SysEvent(const std::string& sender, PipelineEventProducer* handler,
    std::shared_ptr<EventRaw::RawData> rawData, int64_t seq)
    : SysEvent(sender, handler, rawData, seq, "", "")
{
}

SysEvent::SysEvent(const std::string& sender, PipelineEventProducer* handler,
    std::shared_ptr<EventRaw::RawData> rawData)
    : SysEvent(sender, handler, rawData, 0)
{}

SysEvent::SysEvent(const std::string& sender, PipelineEventProducer* handler, SysEventCreator& sysEventCreator)
    : SysEvent(sender, handler, sysEventCreator.GetRawData())
{}

SysEvent::SysEvent(const std::string& sender, PipelineEventProducer* handler, const std::string& jsonStr)
    : SysEvent(sender, handler, TansJsonStrToRawData(jsonStr))
{}

SysEvent::~SysEvent()
{
    if (totalCount_ > 0) {
        totalCount_.fetch_sub(1);
    }
    if (rawData_ != nullptr && rawData_->GetData() != nullptr) {
        totalSize_.fetch_sub(*(reinterpret_cast<int32_t*>(rawData_->GetData())));
    }
    if (totalSize_ < 0) {
        totalSize_.store(0);
    }
}

void SysEvent::InitialMembers()
{
    HiSysEventHeader header = *(reinterpret_cast<struct HiSysEventHeader*>(rawData_->GetData() + BLOCK_SIZE_OFFSET));
    domain_ = header.domain;
    eventName_ = header.name;
    eventType_ = static_cast<int32_t>(header.type) + 1;
    happenTime_ = header.timestamp;
    if (happenTime_ == 0) {
        auto currentTimeStamp = TimeUtil::GetMilliseconds();
        happenTime_ = currentTimeStamp;
    }
    tz_ = static_cast<int16_t>(header.timeZone);
    pid_ = static_cast<int32_t>(header.pid);
    tid_ = static_cast<int32_t>(header.tid);
    uid_ = static_cast<int32_t>(header.uid);
    log_ = header.log;
}

bool SysEvent::InitBuilder()
{
    if (builder_ != nullptr) {
        return true;
    }
    builder_ = std::make_shared<EventRaw::RawDataBuilder>(rawData_);
    return builder_ != nullptr;
}

std::shared_ptr<EventRaw::RawData> SysEvent::TansJsonStrToRawData(const std::string& jsonStr)
{
    auto parser = std::make_unique<EventRaw::RawDataBuilderJsonParser>(jsonStr);
    if (parser == nullptr) {
        return nullptr;
    }
    auto rawDataBuilder = parser->Parse();
    if (rawDataBuilder == nullptr) {
        return nullptr;
    }
    return rawDataBuilder->Build();
}

void SysEvent::SetTag(const std::string& tag)
{
    tag_ = tag;
}

std::string SysEvent::GetTag() const
{
    return tag_;
}

void SysEvent::SetLevel(const std::string& level)
{
    level_ = level;
}

std::string SysEvent::GetLevel() const
{
    return level_;
}

void SysEvent::SetSeq(int64_t seq)
{
    seq_ = seq;
}

int64_t SysEvent::GetSeq() const
{
    return seq_;
}

void SysEvent::SetEventSeq(int64_t eventSeq)
{
    eventSeq_ = eventSeq;
}

int64_t SysEvent::GetEventSeq() const
{
    return eventSeq_;
}

int32_t SysEvent::GetPid() const
{
    return pid_;
}

int32_t SysEvent::GetTid() const
{
    return tid_;
}

int32_t SysEvent::GetUid() const
{
    return uid_;
}

int16_t SysEvent::GetTz() const
{
    return tz_;
}

int SysEvent::GetEventType() const
{
    return eventType_;
}

void SysEvent::SetId(uint64_t id)
{
    if (rawData_ != nullptr && rawData_->GetData() != nullptr) {
        rawData_->Update(reinterpret_cast<uint8_t*>(&id), sizeof(uint64_t),
            BLOCK_SIZE_OFFSET + EventRaw::POS_OF_ID_IN_HEADER);
    }
    if (builder_ != nullptr) {
        builder_->AppendId(id);
    }
}

void SysEvent::SetLog(uint8_t log)
{
    log_ = log;
    if (rawData_ != nullptr && rawData_->GetData() != nullptr) {
        reinterpret_cast<struct HiSysEventHeader*>(
            rawData_->GetData() + BLOCK_SIZE_OFFSET)->log = log;
    }
    if (builder_ != nullptr) {
        builder_->AppendLog(log);
    }
}

void SysEvent::SetPrivacy(uint8_t privacy)
{
    privacy_ = privacy;
}

uint8_t SysEvent::GetPrivacy() const
{
    return privacy_;
}

void SysEvent::SetInvalidParams(PARAM_INFO_MAP_PTR invalidParams)
{
    invalidParams_ = invalidParams;
}

PARAM_INFO_MAP_PTR SysEvent::GetInvalidParams()
{
    return invalidParams_;
}

bool SysEvent::RemoveParam(const std::string& paramName)
{
    if (!InitBuilder()) {
        return false;
    }
    builder_->RemoveParam(paramName);
    isUpdated_ = true;
    return true;
}

bool SysEvent::IsParamExist(const std::string& paramName)
{
    if (!InitBuilder()) {
        return false;
    }
    return builder_->GetValue(paramName) != nullptr;
}

std::string SysEvent::GetEventValue(const std::string& key)
{
    if (!InitBuilder()) {
        return "";
    }
    std::string dest;
    builder_->ParseValueByKey(key, dest);
    return dest;
}

int64_t SysEvent::GetEventIntValue(const std::string& key)
{
    if (!InitBuilder()) {
        return DEFAULT_INT_VALUE;
    }
    if (int64_t intDest = DEFAULT_INT_VALUE; builder_->ParseValueByKey(key, intDest)) {
        return intDest;
    }
    if (uint64_t uIntDest = DEFAULT_UINT_VALUE; builder_->ParseValueByKey(key, uIntDest) &&
        (uIntDest <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))) {
        return static_cast<int64_t>(uIntDest);
    }
    if (double dDest = DEFAULT_DOUBLE_VALUE; builder_->ParseValueByKey(key, dDest) &&
        (dDest >= static_cast<double>(std::numeric_limits<int64_t>::min())) &&
        (dDest <= static_cast<double>(std::numeric_limits<int64_t>::max()))) {
        return static_cast<int64_t>(dDest);
    }
    return DEFAULT_INT_VALUE;
}

uint64_t SysEvent::GetEventUintValue(const std::string& key)
{
    if (!InitBuilder()) {
        return DEFAULT_UINT_VALUE;
    }
    if (uint64_t uIntDest = DEFAULT_UINT_VALUE; builder_->ParseValueByKey(key, uIntDest)) {
        return uIntDest;
    }
    if (int64_t intDest = DEFAULT_INT_VALUE; builder_->ParseValueByKey(key, intDest) &&
        (intDest >= DEFAULT_INT_VALUE)) {
        return static_cast<uint64_t>(intDest);
    }
    if (double dDest = DEFAULT_DOUBLE_VALUE; builder_->ParseValueByKey(key, dDest) &&
        (dDest >= static_cast<double>(std::numeric_limits<uint64_t>::min())) &&
        (dDest <= static_cast<double>(std::numeric_limits<uint64_t>::max()))) {
        return static_cast<uint64_t>(dDest);
    }
    return DEFAULT_UINT_VALUE;
}

double SysEvent::GetEventDoubleValue(const std::string& key)
{
    if (!InitBuilder()) {
        return DEFAULT_DOUBLE_VALUE;
    }
    if (double dDest = DEFAULT_DOUBLE_VALUE; builder_->ParseValueByKey(key, dDest)) {
        return dDest;
    }
    if (int64_t intDest = DEFAULT_INT_VALUE; builder_->ParseValueByKey(key, intDest)) {
        return static_cast<double>(intDest);
    }
    if (uint64_t uIntDest = DEFAULT_UINT_VALUE; builder_->ParseValueByKey(key, uIntDest)) {
        return static_cast<double>(uIntDest);
    }
    return DEFAULT_DOUBLE_VALUE;
}

bool SysEvent::GetEventIntArrayValue(const std::string& key, std::vector<int64_t>& dest)
{
    dest.clear();
    if (!InitBuilder()) {
        return false;
    }
    auto intArrayItemHandler = [&dest] (int64_t& item) {
        dest.emplace_back(item);
        return true;
    };
    auto uIntArrayItemHandler = [&dest] (uint64_t& item) {
        if (item <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
            dest.emplace_back(static_cast<int64_t>(item));
            return true;
        }
        return false;
    };
    auto dArrayItemHandler = [&dest] (double& item) {
        if ((item >= static_cast<double>(std::numeric_limits<int64_t>::min())) &&
            (item <= static_cast<double>(std::numeric_limits<int64_t>::max()))) {
            dest.emplace_back(static_cast<int64_t>(item));
            return true;
        }
        return false;
    };
    auto strArrayItemHandler = [&dest] (std::string& item) {
        return false;
    };
    if (ParseArrayValue<int64_t>(builder_, key, intArrayItemHandler) ||
        ParseArrayValue<uint64_t>(builder_, key, uIntArrayItemHandler) ||
        ParseArrayValue<double>(builder_, key, dArrayItemHandler) ||
        ParseArrayValue<std::string>(builder_, key, strArrayItemHandler)) {
        return true;
    }
    dest.clear();
    return false;
}

bool SysEvent::GetEventUintArrayValue(const std::string& key, std::vector<uint64_t>& dest)
{
    dest.clear();
    if (!InitBuilder()) {
        return false;
    }
    auto uIntArrayItemHandler = [&dest] (uint64_t& item) {
        dest.emplace_back(item);
        return true;
    };
    auto intArrayItemHandler = [&dest] (int64_t& item) {
        if (item >= DEFAULT_INT_VALUE) {
            dest.emplace_back(static_cast<uint64_t>(item));
            return true;
        }
        return false;
    };
    auto dArrayItemHandler = [&dest] (double& item) {
        if ((item >= static_cast<double>(std::numeric_limits<uint64_t>::min())) &&
            (item <= static_cast<double>(std::numeric_limits<uint64_t>::max()))) {
            dest.emplace_back(static_cast<uint64_t>(item));
            return true;
        }
        return false;
    };
    auto strArrayItemHandler = [&dest] (std::string& item) {
        return false;
    };
    if (ParseArrayValue<uint64_t>(builder_, key, uIntArrayItemHandler) ||
        ParseArrayValue<int64_t>(builder_, key, intArrayItemHandler) ||
        ParseArrayValue<double>(builder_, key, dArrayItemHandler) ||
        ParseArrayValue<std::string>(builder_, key, strArrayItemHandler)) {
        return true;
    }
    dest.clear();
    return false;
}

bool SysEvent::GetEventDoubleArrayValue(const std::string& key, std::vector<double>& dest)
{
    dest.clear();
    if (!InitBuilder()) {
        return false;
    }
    auto dArrayItemHandler = [&dest] (double& item) {
        dest.emplace_back(item);
        return true;
    };
    auto intArrayItemHandler = [&dest] (int64_t& item) {
        dest.emplace_back(static_cast<double>(item));
        return true;
    };
    auto uIntArrayItemHandler = [&dest] (uint64_t& item) {
        dest.emplace_back(static_cast<double>(item));
        return true;
    };
    auto strArrayItemHandler = [&dest] (std::string& item) {
        return false;
    };
    if (ParseArrayValue<double>(builder_, key, dArrayItemHandler) ||
        ParseArrayValue<int64_t>(builder_, key, intArrayItemHandler) ||
        ParseArrayValue<uint64_t>(builder_, key, uIntArrayItemHandler) ||
        ParseArrayValue<std::string>(builder_, key, strArrayItemHandler)) {
        return true;
    }
    dest.clear();
    return false;
}

bool SysEvent::GetEventStringArrayValue(const std::string& key, std::vector<std::string>& dest)
{
    dest.clear();
    if (!InitBuilder()) {
        return false;
    }
    auto strArrayItemHandler = [&dest] (std::string& item) {
        dest.emplace_back(item);
        return true;
    };
    auto dArrayItemHandler = [&dest] (double& item) {
        return false;
    };
    auto intArrayItemHandler = [&dest] (int64_t& item) {
        return false;
    };
    auto uIntArrayItemHandler = [&dest] (uint64_t& item) {
        return false;
    };
    if (ParseArrayValue<std::string>(builder_, key, strArrayItemHandler) ||
        ParseArrayValue<uint64_t>(builder_, key, uIntArrayItemHandler) ||
        ParseArrayValue<int64_t>(builder_, key, intArrayItemHandler) ||
        ParseArrayValue<double>(builder_, key, dArrayItemHandler)) {
        return true;
    }
    dest.clear();
    return false;
}

bool SysEvent::TryToUpdateRawData()
{
    if (rawData_ == nullptr) {
        return false;
    }

    // raw data does not need to be re-initialized
    if (!isUpdated_) {
        return true;
    }

    // raw data needs to be re-initialized
    if (!InitBuilder()) {
        return false;
    }
    builder_->AppendLog(log_);
    rawData_ = builder_->Build();
    if (rawData_ == nullptr) {
        return false;
    }
    isUpdated_ = false;
    return true;
}

std::string SysEvent::AsJsonStr()
{
    if (!TryToUpdateRawData()) {
        return "";
    }

    std::string jsonStr;
    {
        EventRaw::DecodedEvent event(rawData_->GetData(), rawData_->GetDataLength());
        jsonStr = event.AsJsonStr();
    }
    if (!tag_.empty()) {
        AppendJsonValue(jsonStr, EventStore::EventCol::TAG, tag_);
    }
    if (!level_.empty()) {
        AppendJsonValue(jsonStr, EventStore::EventCol::LEVEL, level_);
    }
    if (eventSeq_ >= 0) {
        AppendJsonValue(jsonStr, EventStore::EventCol::SEQ, eventSeq_);
    }
    if (reportInterval_ != UNINIT_REPORT_INTERVAL) {
        AppendJsonValue(jsonStr, REPORT_INTERVAL_KEY, reportInterval_);
    }
    return jsonStr;
}

uint8_t* SysEvent::AsRawData()
{
    if (!TryToUpdateRawData()) {
        return nullptr;
    }
    return rawData_->GetData();
}

std::string SysEvent::EscapeJsonStringValue(const std::string& src)
{
    return StringUtil::EscapeJsonStringValue(src);
}

std::string SysEvent::UnescapeJsonStringValue(const std::string& src)
{
    return StringUtil::UnescapeJsonStringValue(src);
}

std::string SysEvent::GetSysVersion()
{
    return sysVersion_;
}

std::string SysEvent::GetPatchVersion()
{
    return patchVersion_;
}

void SysEvent::SetEventPeriodSeqInfo(const EventPeriodSeqInfo& info)
{
    std::string periodSeqStr = info.timeStamp;
    // append export tag
    periodSeqStr.append(" ").append(std::to_string(info.isNeedExport ? 1 : 0));
    // append period sequence
    periodSeqStr.append(" ").append(std::to_string(info.periodSeq));
    SetValue("period_seq_", periodSeqStr);
}

EventPeriodSeqInfo SysEvent::GetEventPeriodSeqInfo()
{
    std::string periodSeqStr = GetValue("period_seq_");
    if (periodSeqStr.empty()) {
        periodSeqStr = GetEventValue("period_seq_");
    }
    std::vector<std::string> allInfo;
    StringUtil::SplitStr(periodSeqStr, " ", allInfo);
    EventPeriodSeqInfo eventPeriodSeqInfo;
    if (allInfo.size() != PERIOD_SEQ_INFO_ITEM_CNT) {
        return eventPeriodSeqInfo;
    }
    eventPeriodSeqInfo.timeStamp = allInfo[0]; // 0 is the index of time stamp
    int isNeedExported = 0;
    StringUtil::ConvertStringTo(allInfo[1], isNeedExported); // 1 is the index of tag whether to export
    eventPeriodSeqInfo.isNeedExport = static_cast<bool>(isNeedExported);
    uint64_t periodSeq = 0;
    StringUtil::ConvertStringTo(allInfo[2], periodSeq); // 2 is the index of period sequence
    eventPeriodSeqInfo.periodSeq = periodSeq;
    return eventPeriodSeqInfo;
}

void SysEvent::SetReportInterval(int32_t reportInterval)
{
    if (reportInterval_ == reportInterval) {
        return;
    }
    reportInterval_ = reportInterval;
    SetEventValue(REPORT_INTERVAL_KEY, reportInterval_);
}

int32_t SysEvent::GetReportInterval()
{
    if (reportInterval_ == UNINIT_REPORT_INTERVAL) {
        reportInterval_ = static_cast<int32_t>(GetEventIntValue(REPORT_INTERVAL_KEY));
    }
    return reportInterval_;
}

SysEventCreator::SysEventCreator(const std::string& domain, const std::string& eventName,
    SysEventCreator::EventType type)
{
    builder_ = std::make_shared<EventRaw::RawDataBuilder>();
    if (builder_ != nullptr) {
        builder_->AppendDomain(domain).AppendName(eventName).AppendType(static_cast<int>(type)).
            AppendTimeStamp(TimeUtil::GetMilliseconds()).AppendTimeZone(TimeUtil::GetTimeZone()).
            AppendPid(getpid()).AppendTid(gettid()).AppendUid(getuid());
    }
}

std::shared_ptr<EventRaw::RawData> SysEventCreator::GetRawData()
{
    if (builder_ == nullptr) {
        return nullptr;
    }
    return builder_->Build();
}

std::string SysEventCreator::EscapeJsonStringValue(const std::string& src)
{
    return StringUtil::EscapeJsonStringValue(src);
}
} // namespace HiviewDFX
} // namespace OHOS
