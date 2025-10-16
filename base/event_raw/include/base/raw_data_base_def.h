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

#ifndef BASE_EVENT_RAW_INCLUDE_RAW_BASE_DEF_H
#define BASE_EVENT_RAW_INCLUDE_RAW_BASE_DEF_H

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
constexpr unsigned int MAX_DOMAIN_LENGTH = 16;
constexpr unsigned int MAX_EVENT_NAME_LENGTH = 32;
constexpr unsigned int MAX_ARRAY_SIZE = 100;

constexpr char BASE_INFO_KEY_DOMAIN[] = "domain_";
constexpr char BASE_INFO_KEY_NAME[] = "name_";
constexpr char BASE_INFO_KEY_TYPE[] = "type_";
constexpr char BASE_INFO_KEY_TIME_STAMP[] = "time_";
constexpr char BASE_INFO_KEY_TIME_ZONE[] = "tz_";
constexpr char BASE_INFO_KEY_ID[] = "id_";
constexpr char BASE_INFO_KEY_PID[] = "pid_";
constexpr char BASE_INFO_KEY_TID[] = "tid_";
constexpr char BASE_INFO_KEY_UID[] = "uid_";
constexpr char BASE_INFO_KEY_LOG[] = "log_";
constexpr char BASE_INFO_KEY_TRACE_ID[] = "traceid_";
constexpr char BASE_INFO_KEY_SPAN_ID[] = "spanid_";
constexpr char BASE_INFO_KEY_PARENT_SPAN_ID[] = "pspanid_";
constexpr char BASE_INFO_KEY_TRACE_FLAG[] = "trace_flag_";

constexpr unsigned int TAG_BYTE_OFFSET = 5;
constexpr unsigned int TAG_BYTE_BOUND  = (1 << TAG_BYTE_OFFSET);
constexpr unsigned int TAG_BYTE_MASK = (TAG_BYTE_BOUND - 1);

constexpr unsigned int NON_TAG_BYTE_OFFSET = 7;
constexpr unsigned int NON_TAG_BYTE_BOUND = (1 << NON_TAG_BYTE_OFFSET);
constexpr unsigned int NON_TAG_BYTE_MASK = (NON_TAG_BYTE_BOUND - 1);

template<typename T>
inline static constexpr bool isUnsignedNum = std::is_same_v<std::decay_t<T>, std::uint8_t> ||
    std::is_same_v<std::decay_t<T>, std::uint16_t> || std::is_same_v<std::decay_t<T>, std::uint32_t> ||
    std::is_same_v<std::decay_t<T>, uint64_t>;

template<typename T>
inline static constexpr bool isSignedNum = std::is_same_v<std::decay_t<T>, std::int8_t> ||
    std::is_same_v<std::decay_t<T>, std::int16_t> || std::is_same_v<std::decay_t<T>, std::int32_t> ||
    std::is_same_v<std::decay_t<T>, int64_t>;

template<typename T>
inline static constexpr bool isFloatingPointNum = std::is_same_v<std::decay_t<T>, float> ||
    std::is_same_v<std::decay_t<T>, double>;

template<typename T>
inline static constexpr bool isString = std::is_same_v<std::decay_t<T>, std::string> ||
    std::is_same_v<std::decay_t<T>, const char*>;

template<typename T>
inline static constexpr bool isUnsignedNumArray = std::is_same_v<std::decay_t<T>, std::vector<uint8_t>> ||
    std::is_same_v<std::decay_t<T>, std::vector<uint16_t>> ||
    std::is_same_v<std::decay_t<T>, std::vector<uint32_t>> || std::is_same_v<std::decay_t<T>, std::vector<uint64_t>>;

template<typename T>
inline static constexpr bool isSignedNumArray = std::is_same_v<std::decay_t<T>, std::vector<int8_t>> ||
    std::is_same_v<std::decay_t<T>, std::vector<int16_t>> ||
    std::is_same_v<std::decay_t<T>, std::vector<int32_t>> || std::is_same_v<std::decay_t<T>, std::vector<int64_t>>;

template<typename T>
inline static constexpr bool isFloatingPointNumArray = std::is_same_v<std::decay_t<T>, std::vector<float>> ||
    std::is_same_v<std::decay_t<T>, std::vector<double>>;

#pragma pack(1)
struct HiSysEventHeader {
    /* Event domain */
    char domain[MAX_DOMAIN_LENGTH + 1];

    /* Event name */
    char name[MAX_EVENT_NAME_LENGTH + 1];

    /* Event timestamp */
    uint64_t timestamp;

    /* Time zone */
    uint8_t timeZone;

    /* User id */
    uint32_t uid;

    /* Process id */
    uint32_t pid;

    /* Thread id */
    uint32_t tid;

    /* Event hash code*/
    uint64_t id;

    /* Event type */
    uint8_t type : 2;

    /* Trace info flag*/
    uint8_t isTraceOpened : 1;

    /* Log packing flag */
    uint8_t log;
};

struct TraceInfo {
    /* Hitrace flag */
    uint8_t traceFlag;

    /* Hitrace id */
    uint64_t traceId;

    /* Hitrace span id */
    uint32_t spanId;

    /* Hitrace parent span id */
    uint32_t pSpanId;
};

struct ParamValueType {
    /* Array flag */
    uint8_t isArray : 1;

    /* Type of parameter value */
    uint8_t valueType : 4;

    /* Byte count of parameter value */
    uint8_t valueByteCnt : 3;
};
#pragma pack()

enum ValueType: uint8_t {
    // Unknown value
    UNKNOWN = 0,

    // Bool value
    BOOL,

    // Int8_t value
    INT8,

    // Uint8_t value
    UINT8,

    // Int16_t value
    INT16,

    // Uint16_t value
    UINT16,

    // Int32_t value
    INT32,

    // Uint32_t value
    UINT32,

    // Int64_t value
    INT64,

    // Uint64_t value
    UINT64,

    // Float value
    FLOAT,

    // Double value
    DOUBLE,

    // String value
    STRING,
};

enum EncodeType: int8_t {
    // Varint encoding
    VARINT = 0,

    // Length delimited encoding
    LENGTH_DELIMITED = 1,

    // Reserved
    INVALID = 4,
};

constexpr uint32_t POS_OF_PID_IN_HEADER = sizeof(HiSysEventHeader::domain) + sizeof(HiSysEventHeader::name)
    + sizeof(HiSysEventHeader::timestamp) + sizeof(HiSysEventHeader::timeZone) + sizeof(HiSysEventHeader::uid);

constexpr uint32_t POS_OF_ID_IN_HEADER = POS_OF_PID_IN_HEADER + sizeof(HiSysEventHeader::pid)
    + sizeof(HiSysEventHeader::tid);

int ParseTimeZone(const std::string& tzStr);
std::string ParseTimeZone(const uint8_t tzVal);
size_t GetValidDataMinimumByteCount();

template<typename T>
std::string TransNumToHexStr(T num)
{
    std::stringstream ss;
    ss << std::hex << num;
    return ss.str();
}
} // namespace EventRaw
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_RAW_INCLUDE_RAW_BASE_DEF_H