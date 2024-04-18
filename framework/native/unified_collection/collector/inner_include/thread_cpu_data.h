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
#ifndef HIVIEW_THREAD_CPU_DATA_H
#define HIVIEW_THREAD_CPU_DATA_H

#include "unified_collection_data.h"

namespace OHOS {
namespace HiviewDFX {
class ThreadCpuData {
public:
    ThreadCpuData(int magic, int pid, uint32_t thread_count);
    ~ThreadCpuData();
    struct ucollection_thread_cpu_item* GetNextThread();

private:
    void Init(int magic, uint32_t totalCount, int pid);

    friend class CollectDeviceClient;
    struct ucollection_thread_cpu_entry *entry_;
    unsigned int current_;
};
} // HiviewDFX
} // OHOS
#endif //HIVIEW_THREAD_CPU_DATA_H
