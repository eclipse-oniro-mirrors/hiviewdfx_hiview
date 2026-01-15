/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef HIVIEWDFX_HIVIEW_TRACE_DB_CALLBACK_H
#define HIVIEWDFX_HIVIEW_TRACE_DB_CALLBACK_H

#include "rdb_helper.h"

namespace OHOS {
namespace HiviewDFX {
// Table trace_flow_control column name
inline constexpr char FLOW_TABLE_NAME[] = "trace_flow_control";
inline constexpr char COLUMN_SYSTEM_TIME[] = "system_time";
inline constexpr char COLUMN_CALLER_NAME[] = "caller_name";
inline constexpr char COLUMN_USED_SIZE[] = "used_size";
inline constexpr char COLUMN_IO_USED_SIZE[] = "io_used_size";
inline constexpr char COLUMN_ZIP_USED_SIZE[] = "zip_used_size";
inline constexpr char COLUMN_DYNAMIC_DECREASE[] = "dynamic_decrease";

// Table unified_collection_task column name
inline constexpr char TABLE_NAME_TASK[] = "unified_collection_task";
inline constexpr char COLUMN_TASK_DATE[] = "task_date";
inline constexpr char COLUMN_TASK_TYPE[] = "task_type";
inline constexpr char COLUMN_UID[] = "uid";
inline constexpr char COLUMN_PID[] = "pid";
inline constexpr char COLUMN_BUNDLE_NAME[] = "bundle_name";
inline constexpr char COLUMN_BUNDLE_VERSION[] = "bundle_version";
inline constexpr char COLUMN_START_TIME[] = "start_time";
inline constexpr char COLUMN_FINISH_TIME[] = "finish_time";
inline constexpr char COLUMN_RESOURCE_PATH[] = "resource_path";
inline constexpr char COLUMN_RESOURCE_SIZE[] = "resource_size";
inline constexpr char COLUMN_COST_CPU[] = "cost_cpu";
inline constexpr char COLUMN_STATE[] = "state";

// Table trace_behavior_db_helper column name
inline constexpr char TABLE_NAME_BEHAVIOR[] = "trace_behavior_db_helper";
inline constexpr char COLUMN_BEHAVIOR_ID[] = "behavior_id ";
inline constexpr char COLUMN_DATE[] = "task_date";
inline constexpr char COLUMN_USED_QUOTA[] = "used_quota";

// Table telemetry_flow_control column name
inline constexpr char TABLE_TELEMETRY_CONTROL[] = "telemetry_control";
inline constexpr char COLUMN_MODULE_NAME[] = "module";
inline constexpr char COLUMN_QUOTA[] = "quota";
inline constexpr char COLUMN_TELEMTRY_ID[] = "telemetry_id";
inline constexpr char COLUMN_RUNNING_TIME[] = "running_time";

namespace TraceDbStoreCallback {
int OnCreate(NativeRdb::RdbStore &rdbStore);
int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion);
}
}
}
#endif // HIVIEWDFX_HIVIEW_TRACE_DB_CALLBACK_H
