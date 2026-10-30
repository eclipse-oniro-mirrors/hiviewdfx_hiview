/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef TELEMETRY_TYPES_H
#define TELEMETRY_TYPES_H

#include <string>
#include <cinttypes>
#include <cstdint>
#include <map>
#include <sys/types.h>

// Telemetry path and property constants (single source of truth)
inline constexpr const char* const TELEMETRY_QUOTA_PROPERTY = "user.oomdumptelemetry.quota";
inline constexpr const char* const TELEMETRY_FILTER_LIST_PROPERTY = "user.oomdumptelemetry.enable";
inline constexpr const char* const TELEMETRY_QUOTA_PATH = "/data/service/el1/public/hiview/oomdump_telemetry";
inline constexpr const char* const TELEMETRY_FILTER_LIST_KEY = "filterList";

namespace OHOS {
namespace HiviewDFX {

// Telemetry message type (self-contained, mirrors hiview MessageType::TELEMETRY_EVENT)
enum TelemetryMessageType {
    TELEMETRY_MSG_NONE = 0,
    TELEMETRY_MSG_EVENT = 1,
};

// Telemetry source enum
enum TelemetrySource {
    SYSTEM_TELEMETRY = 0,
    APP_TELEMETRY,
    TELEMETRY_SOURCE_MAX
};

// Leak type enum
enum {
    JS_LEAK_TYPE = 0x01,
    NATIVE_LEAK_TYPE = 0x02,
    JS_OOM_TYPE = 0x04,
    FD_LEAK_TYPE = 0x08,
    THREAD_LEAK_TYPE = 0x10,
    GPU_LEAK_TYPE = 0x20,
    ION_LEAK_TYPE = 0x40,
    OTHER_LEAK = 0x80,
    RSS_LEAK_TYPE = 0x100,
    EXT_PSS_LEAK_TYPE = 0x200,
    GPU_RS_LEAK_TYPE = 0x400,
    ASHMEM_LEAK_TYPE = 0x800,
    LITE_PROFILER_LEAK_TYPE = 0x1000,
};

// SubLeakType enum
enum SubLeakType {
    SUB_NATIVE_HEAP_LEAK = 1,
    SUB_ARKTS_LEAK,
    SUB_ARKTS_STA_LEAK,
    SUB_KOTLIN_LEAK,
    SUB_JSVM_LEAK,
    SUB_ARKWEB_V8_LEAK,
    SUB_ASHMEM_LEAK,
    SUB_GPU_LEAK,
    SUB_ION_LEAK,
    SUB_AVCODEC_LEAK,
    SUB_MEDIA_LEAK,
    SUB_LEAK_TYPE_MAX,
};

// TelemetryLeakInfo struct
struct TelemetryLeakInfo {
    uint64_t leakType{EXT_PSS_LEAK_TYPE};
    SubLeakType subLeakType{SUB_LEAK_TYPE_MAX};
    int32_t subleakOverThreshold{2048};
    TelemetrySource telemetrySource{TELEMETRY_SOURCE_MAX};
    std::string bundleName;
    pid_t pid{-1};
    int32_t maxProfilerTime{0};
    int32_t leakThreshold{4096};
    std::string telemetryId{""};
};

// Telemetry error codes for HiSysEvent reporting
enum TelemetryError {
    // errcode for STATISTIC
    NO_ERROR = 0,
    // errcode for FAULT
    DECODE_TELEMTRYID_ERROR,
    DECODE_BUNDLENAME_ERROR,
    DECODE_LEAKTYPE_ERRORI,
    DECODE_LEAKTYPE_ERRORII,
    DECODE_KEYSTR_ERROR,
    REPEATED_EVENT_ERROR,
    BUNDLE_QUOTA_EXHAUSTED,
    POWER_CONNECT_QUOTA_CONSUMED,
    POWER_DISCONNECT_QUOTA_CONSUMED,
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // TELEMETRY_TYPES_H
