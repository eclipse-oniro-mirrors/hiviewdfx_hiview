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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_MANAGER_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_MANAGER_H
#include <cinttypes>
#include <string>
#include <vector>

namespace OHOS {
namespace HiviewDFX {
class TraceManager {
public:
    TraceManager() {};
    ~TraceManager() {};

public:
    int32_t OpenSnapshotTrace(const std::vector<std::string> &tagGroups);
    int32_t OpenRecordingTrace(const std::string &args);
    int32_t CloseTrace();
    int32_t RecoverTrace();
    int32_t GetTraceMode();
    static void RecoverTmpTrace();
};
} // HiViewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_MANAGER_H
