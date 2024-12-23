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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_DEVICE_CLIENT_PROCESS_CPU_DATA_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_DEVICE_CLIENT_PROCESS_CPU_DATA_H
#include "unified_collection_data.h"

namespace OHOS {
namespace HiviewDFX {
class ProcessCpuData {
public:
    ProcessCpuData(int magic, int pid, unsigned int count);
    ~ProcessCpuData();
    struct ucollection_process_cpu_item* GetNextProcess();

private:
    void Init(int magic, unsigned int totalCount, int pid);

    friend class CollectDeviceClient;
    struct ucollection_process_cpu_entry *entry_;
    unsigned int current_;
};
} // HiviewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_DEVICE_CLIENT_PROCESS_CPU_DATA_H