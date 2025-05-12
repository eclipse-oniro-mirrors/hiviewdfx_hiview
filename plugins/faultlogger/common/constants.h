/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_FAULT_LOGGER_CONSTANTS_H
#define HIVIEW_FAULT_LOGGER_CONSTANTS_H

namespace OHOS {
namespace HiviewDFX {
namespace FaultLogger {
const char * const APP_CRASH_TYPE = "APP_CRASH";
const char * const APP_FREEZE_TYPE = "APP_FREEZE";
const char * const APP_HICOLLIE_TYPE = "APP_HICOLLIE";

constexpr int REPORT_HILOG_LINE = 100;
constexpr int DECIMAL_BASE = 10;

const char * const FAULTLOG_BASE_FOLDER = "/data/log/faultlog/";
const char * const FAULTLOG_TEMP_FOLDER = "/data/log/faultlog/temp/";
const char * const FAULTLOG_FAULT_LOGGER_FOLDER = "/data/log/faultlog/faultlogger/";
const char * const FAULTLOG_FAULT_HILOG_FOLDER = "/data/log/faultlog/hilog/";
}

namespace FaultKey {
constexpr const char * const DEVICE_INFO = "DEVICE_INFO";
constexpr const char * const BUILD_INFO = "BUILD_INFO";
constexpr const char * const MODULE_NAME = "MODULE";
constexpr const char * const PROCESS_NAME = "PNAME";
constexpr const char * const MODULE_PID = "PID";
constexpr const char * const MODULE_UID = "UID";
constexpr const char * const MODULE_VERSION = "VERSION";
constexpr const char * const FAULT_TYPE = "FAULT_TYPE";
constexpr const char * const SYS_VM_TYPE = "SYSVMTYPE";
constexpr const char * const APP_VM_TYPE = "APPVMTYPE";
constexpr const char * const FOREGROUND = "FOREGROUND";
constexpr const char * const LIFETIME = "LIFETIME";
constexpr const char * const REASON = "REASON";
constexpr const char * const FAULT_MESSAGE = "FAULT_MESSAGE";
constexpr const char * const STACKTRACE = "TRUSTSTACK";
constexpr const char * const ROOT_CAUSE = "BINDERMAX";
constexpr const char * const MSG_QUEUE_INFO = "MSG_QUEUE_INFO";
constexpr const char * const BINDER_TRANSACTION_INFO = "BINDER_TRANSACTION_INFO";
constexpr const char * const PROCESS_STACKTRACE = "PROCESS_STACKTRACE";
constexpr const char * const OTHER_THREAD_INFO = "OTHER_THREAD_INFO";
constexpr const char * const KEY_THREAD_INFO = "KEY_THREAD_INFO";
constexpr const char * const KEY_THREAD_REGISTERS = "KEY_THREAD_REGISTERS";
constexpr const char * const MEMORY_USAGE = "MEM_USAGE";
constexpr const char * const CPU_USAGE = "FAULTCPU";
constexpr const char * const TRACE_ID = "TRACEID";
constexpr const char * const SUMMARY = "SUMMARY";
constexpr const char * const TIMESTAMP = "TIMESTAMP";
constexpr const char * const MEMORY_NEAR_REGISTERS = "MEMORY_NEAR_REGISTERS";
constexpr const char * const PRE_INSTALL = "PRE_INSTALL";
constexpr const char * const VERSION_CODE = "VERSION_CODE";
constexpr const char * const FINGERPRINT = "FINGERPRINT";
constexpr const char * const APPEND_ORIGIN_LOG = "APPEND_ORIGIN_LOG";
constexpr const char * const PROCESS_RSS_MEMINFO = "PROCESS_RSS_MEMINFO";
constexpr const char * const DEVICE_MEMINFO = "DEVICE_MEMINFO";
const char * const LIFECYCLE_TIMEOUT = "LIFECYCLE_TIMEOUT";
const char * const STACK = "STACK";
const char * const PACKAGE_NAME = "PACKAGE_NAME";
const char * const HILOG = "HILOG";
const char * const LOG_PATH = "LOG_PATH";
const char * const HAPPEN_TIME = "HAPPEN_TIME";
const char * const FIRST_FRAME = "FIRST_FRAME";
const char * const SECOND_FRAME = "SECOND_FRAME";
const char * const LAST_FRAME = "LAST_FRAME";
}
}  // namespace HiviewDFX
}  // namespace OHOS
#endif  // HIVIEW_FAULT_LOGGGER_CONSTANTS_H
