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

#ifndef HIVIEW_BASE_SYS_EVENT_H
#define HIVIEW_BASE_SYS_EVENT_H

#include <atomic>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "encoded/encoded_param.h"
#include "decoded/decoded_event.h"
#include "pipeline.h"
#include "encoded/raw_data_builder.h"
#include "base/raw_data.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
class EventCol {
public:
    static std::string DOMAIN;
    static std::string NAME;
    static std::string TYPE;
    static std::string TS;
    static std::string TZ;
    static std::string PID;
    static std::string TID;
    static std::string UID;
    static std::string INFO;
    static std::string LEVEL;
    static std::string SEQ;
    static std::string TAG;
};
}

struct EventParamInfo {
    std::string allowListFile;
    uint8_t throwType = 0;
    EventParamInfo(const std::string& allowListFile, uint8_t throwType)
        : allowListFile(allowListFile), throwType(throwType) {}
};

struct EventPeriodSeqInfo {
    // formatted time stamp: YYYYMMDDHH
    std::string timeStamp;

    // tag for event export:
    bool isNeedExport = false;

    // the sequence of event in current time range
    uint64_t periodSeq = 0;
};

// param info of one event, like <paramName, EventParamInfo>
using PARAM_INFO_MAP_PTR = std::shared_ptr<std::map<std::string, std::shared_ptr<EventParamInfo>>>;

constexpr uint8_t LOG_ALLOW_PACK = 0 << 5;
constexpr uint8_t LOG_NOT_ALLOW_PACK = 1 << 5;
constexpr uint8_t LOG_PACKED = 1;
constexpr uint8_t LOG_REPEAT = 1;
constexpr uint8_t LOG_THRESHOLD = 2;

constexpr int32_t UNINIT_REPORT_INTERVAL = -1;

class SysEventCreator;
class SysEvent : public PipelineEvent {
public:
    SysEvent(const std::string& sender, PipelineEventProducer* handler,
        std::shared_ptr<EventRaw::RawData> rawData, int64_t seq,
        const std::string& sysVersion, const std::string& patchVersion);
    SysEvent(const std::string& sender, PipelineEventProducer* handler,
        std::shared_ptr<EventRaw::RawData> rawData, int64_t seq);
    SysEvent(const std::string& sender, PipelineEventProducer* handler, std::shared_ptr<EventRaw::RawData> rawData);
    SysEvent(const std::string& sender, PipelineEventProducer* handler, SysEventCreator& sysEventCreator);
    SysEvent(const std::string& sender, PipelineEventProducer* handler, const std::string& jsonStr);
    ~SysEvent();

public:
    void SetTag(const std::string& tag);
    std::string GetTag() const;
    void SetLevel(const std::string& level);
    std::string GetLevel() const;
    int32_t GetPid() const;
    int32_t GetTid() const;
    int32_t GetUid() const;
    int16_t GetTz() const;
    void SetSeq(int64_t seq);
    int64_t GetSeq() const;
    void SetEventSeq(int64_t eventSeq);
    int64_t GetEventSeq() const;
    int GetEventType() const;
    void SetId(uint64_t id);
    void SetLog(uint8_t log);
    void SetPrivacy(uint8_t privacy);
    uint8_t GetPrivacy() const;

    std::string GetEventValue(const std::string& key);
    int64_t GetEventIntValue(const std::string& key);
    uint64_t GetEventUintValue(const std::string& key);
    double GetEventDoubleValue(const std::string& key);
    bool GetEventIntArrayValue(const std::string& key, std::vector<int64_t>& dest);
    bool GetEventUintArrayValue(const std::string& key, std::vector<uint64_t>& dest);
    bool GetEventDoubleArrayValue(const std::string& key, std::vector<double>& dest);
    bool GetEventStringArrayValue(const std::string& key, std::vector<std::string>& dest);
    bool IsParamExist(const std::string& paramName);
    bool RemoveParam(const std::string& paramName);
    void SetInvalidParams(PARAM_INFO_MAP_PTR invalidParams);
    PARAM_INFO_MAP_PTR GetInvalidParams();
    std::string AsJsonStr();
    uint8_t* AsRawData();
    std::string GetSysVersion();
    std::string GetPatchVersion();
    void SetEventPeriodSeqInfo(const EventPeriodSeqInfo& info);
    EventPeriodSeqInfo GetEventPeriodSeqInfo();
    void SetReportInterval(int32_t reportInterval);
    int32_t GetReportInterval();

public:
    template<typename T>
    void SetEventValue(const std::string& key, T value, bool appendValue = false)
    {
        if (!InitBuilder()) {
            return;
        }
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            auto param = builder_->GetValue(key);
            std::string paramValue;
            if (appendValue && (param != nullptr) && param->AsString(paramValue)) {
                paramValue = UnescapeJsonStringValue(paramValue);
                paramValue.append(value);
                value = paramValue;
            }
            value = EscapeJsonStringValue(value);
        }
        builder_->AppendValue(key, value);
        isUpdated_ = true;
    }

public:
    int32_t eventType_;
    bool preserve_;
    uint8_t log_;
    bool collect_ = false;

public:
    static std::atomic<uint32_t> totalCount_;
    static std::atomic<int64_t> totalSize_;

private:
    void InitialMembers();
    bool InitBuilder();
    bool TryToUpdateRawData();
    std::shared_ptr<EventRaw::RawData> TansJsonStrToRawData(const std::string& jsonStr);
    std::string EscapeJsonStringValue(const std::string& src);
    std::string UnescapeJsonStringValue(const std::string& src);

private:
    bool isUpdated_ = false;
    int64_t seq_ = 0;
    int32_t pid_ = 0;
    int32_t tid_ = 0;
    int32_t uid_ = 0;
    int16_t tz_ = 0;
    int64_t eventSeq_ = -1;
    uint8_t privacy_ = 0;
    std::string tag_;
    std::string level_;
    std::shared_ptr<EventRaw::RawDataBuilder> builder_;
    std::string sysVersion_;
    std::string patchVersion_;
    PARAM_INFO_MAP_PTR invalidParams_;
    int32_t reportInterval_ = UNINIT_REPORT_INTERVAL;
};

class SysEventCreator {
public:
    enum EventType {
        FAULT     = 1,    // system fault event
        STATISTIC = 2,    // system statistic event
        SECURITY  = 3,    // system security event
        BEHAVIOR  = 4     // system behavior event
    };

public:
    SysEventCreator(const std::string &domain, const std::string &eventName, EventType type);

public:
    template<typename T>
    void SetKeyValue(const std::string& key, T value)
    {
        if (builder_ == nullptr) {
            return;
        }
        if constexpr (std::is_same_v<std::decay_t<T>, std::vector<std::string>>) {
            std::vector<std::string> transVal;
            for (auto item : value) {
                transVal.emplace_back(EscapeJsonStringValue(item));
            }
            builder_->AppendValue(key, transVal);
            return;
        }
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            value = EscapeJsonStringValue(value);
        }
        builder_->AppendValue(key, value);
    }

public:
    std::shared_ptr<EventRaw::RawData> GetRawData();

private:
    std::string EscapeJsonStringValue(const std::string& src);

private:
    std::shared_ptr<EventRaw::RawDataBuilder> builder_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_SYS_EVENT_H
