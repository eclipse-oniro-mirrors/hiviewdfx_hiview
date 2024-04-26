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
#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_PROCESS_PROCESS_STATUS_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_PROCESS_PROCESS_STATUS_H

#include <mutex>
#include <unordered_map>

#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
enum ProcessState {
    INVALID = -1,
    BACKGROUND = 0,
    FOREGROUND,
    CREATED,
    DIED,
};

struct ProcessInfo {
    std::string name;
    ProcessState state;
    uint64_t lastForegroundTime;
};

class ProcessStatus : public OHOS::DelayedRefSingleton<ProcessStatus> {
public:
    std::string GetProcessName(int32_t pid);
    ProcessState GetProcessState(int32_t pid);
    uint64_t GetProcessLastForegroundTime(int32_t pid);
    void NotifyProcessState(int32_t pid, ProcessState procState, const std::string& name = "");

private:
    bool UpdateProcessName(int32_t pid, const std::string& procName);
    void UpdateProcessState(int32_t pid, ProcessState procState, const std::string& name);
    void UpdateProcessForegroundState(int32_t pid);
    void UpdateProcessBackgroundState(int32_t pid);
    void UpdateProcessCreatedState(int32_t pid, const std::string& name);
    bool NeedClearProcessInfos();
    void ClearProcessInfos();
    void ClearProcessInfo(int32_t pid);

private:
    std::mutex mutex_;
    /* map<pid, ProcessInfo> */
    std::unordered_map<int32_t, ProcessInfo> processInfos_;
    uint32_t capacity_ = 0; // 0 for dynamic resizing based on actual size
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_PROCESS_PROCESS_STATUS_H
