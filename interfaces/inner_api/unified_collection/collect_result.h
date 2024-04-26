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
#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_COLLECT_RESULT_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_COLLECT_RESULT_H

namespace OHOS {
namespace HiviewDFX {
namespace UCollect {
enum UcError {
    SUCCESS = 0,
    UNSUPPORT = 1,
    READ_FAILED = 2,
    WRITE_FAILED = 3,
    PERMISSION_CHECK_FAILED = 4,
    SYSTEM_ERROR = 5,

    // for trace call
    TRACE_IS_OCCUPIED = 1002,
    TRACE_TAG_ERROR = 1003,
    TRACE_FILE_ERROR = 1004,
    TRACE_WRITE_ERROR = 1005,
    TRACE_CALL_ERROR = 1006,

    // control policy
    TRACE_OVER_FLOW = 1100,

    // app trace all
    EXISTS_CAPTURE_TASK = 1201,
    UNEXISTS_CAPTURE_TASK = 1201,
    HAD_CAPTURED_TRACE = 1203,
    INVALID_TRACE_STATE = 1204,
    INCONSISTENT_PROCESS = 1205,
    INVALID_ACTION_ID = 1206,

    // for perf call
    USAGE_EXCEED_LIMIT = 2000,
    PERF_COLLECT_FAILED = 2001,
};
} // UCollect

template<typename T> class CollectResult {
public:
    CollectResult()
    {
        retCode = UCollect::UcError::UNSUPPORT;
        data = {};
    }

    UCollect::UcError retCode;
    T data;
};
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_COLLECT_RESULT_H