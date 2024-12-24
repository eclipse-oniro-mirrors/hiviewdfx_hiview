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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_COLLECT_DEVICE_CLIENT_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_COLLECT_DEVICE_CLIENT_H

#include <memory>
#include <vector>

#include "thread_cpu_data.h"
#include "process_cpu_data.h"

namespace OHOS {
namespace HiviewDFX {
class CollectDeviceClient {
public:
    CollectDeviceClient();
    ~CollectDeviceClient();

public:
    int Open();
    std::shared_ptr<ProcessCpuData> FetchProcessCpuData();
    std::shared_ptr<ProcessCpuData> FetchProcessCpuData(int pid);
    std::shared_ptr<ThreadCpuData> FetchThreadCpuData(int pid);
    std::shared_ptr<ThreadCpuData> FetchSelfThreadCpuData(int pid);
private:
    int GetDeviceFd(bool readOnly);
    unsigned int GetProcessCount();
    unsigned int GetThreadCount(int pid);
    unsigned int GetSelfThreadCount(int pid);
    int fd_;
};
} // HiviewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_COLLECT_DEVICE_CLIENT_H