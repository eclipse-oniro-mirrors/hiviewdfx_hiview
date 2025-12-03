/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_COLLECT_EVENT_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_COLLECT_EVENT_H

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
constexpr char UCOLLECTOR_PLUGIN[] = "UnifiedCollector";

// define unified collection event
constexpr char MAIN_THREAD_JANK[] = "MAIN_THREAD_JANK";

constexpr char START_APP_TRACE[] = "START_APP_TRACE";
constexpr char STOP_APP_TRACE[] = "STOP_APP_TRACE";
constexpr char DUMP_APP_TRACE[] = "DUMP_APP_TRACE";

constexpr char EVENT_PARAM_TRACE_FILE[] = "tracefile";
constexpr char EVENT_PARAM_SYS_EVENT[] = "sysevent";
constexpr char EVENT_PARAM_COST_TIME[] = "costtime";

constexpr char APP_EVENT_PARAM_UID[] = "uid";
constexpr char APP_EVENT_PARAM_PID[] = "pid";
constexpr char APP_EVENT_PARAM_TIME[] = "time";
constexpr char APP_EVENT_PARAM_BUNDLE_NAME[] = "bundle_name";
constexpr char APP_EVENT_PARAM_BUNDLE_VERSION[] = "bundle_version";
constexpr char APP_EVENT_PARAM_BEGIN_TIME[] = "begin_time";
constexpr char APP_EVENT_PARAM_END_TIME[] = "end_time";
constexpr char APP_EVENT_PARAM_ISBUSINESSJANK[] = "is_business_jank";
constexpr char APP_EVENT_PARAM_EXTERNAL_LOG[] = "external_log";
constexpr char APP_EVENT_PARAM_APP_START_JIFFIES_TIME[] = "app_start_jiffies_time";
constexpr char APP_EVENT_PARAM_HEAVIEST_STACK[] = "heaviest_stack";

constexpr char SYS_EVENT_PARAM_BUNDLE_NAME[] = "BUNDLE_NAME";
constexpr char SYS_EVENT_PARAM_BUNDLE_VERSION[] = "BUNDLE_VERSION";
constexpr char SYS_EVENT_PARAM_BEGIN_TIME[] = "BEGIN_TIME";
constexpr char SYS_EVENT_PARAM_END_TIME[] = "END_TIME";
constexpr char SYS_EVENT_PARAM_JANK_LEVEL[] = "JANK_LEVEL";
constexpr char SYS_EVENT_TRACE_DUMP_CODE[] = "TRACE_DUMP_CODE";
constexpr char SYS_EVENT_PARAM_EXTERNAL_LOG[] = "EXTERNAL_LOG";
constexpr char SYS_EVENT_PARAM_THREAD_NAME[] = "THREAD_NAME";
constexpr char SYS_EVENT_PARAM_FOREGROUND[] = "FOREGROUND";
constexpr char SYS_EVENT_PARAM_LOG_TIME[] = "LOG_TIME";
constexpr char SYS_EVENT_PARAM_APP_START_JIFFIES_TIME[] = "APP_START_JIFFIES_TIME";
constexpr char SYS_EVENT_PARAM_HEAVIEST_STACK[] = "HEAVIEST_STACK";
constexpr int32_t SYS_EVENT_JANK_LEVEL_VALUE_TRACE = 1;
} // UCollectUtil
} // HiviewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_COLLECT_EVENT_H