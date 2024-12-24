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
#include "process_cpu_data.h"

#include <cstdlib>

#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
ProcessCpuData::ProcessCpuData(int magic, int pid, unsigned int count): entry_(nullptr), current_(0)
{
    Init(magic, count, pid);
}

void ProcessCpuData::Init(int magic, unsigned int totalCount, int pid)
{
    unsigned int totalSize = sizeof(struct ucollection_process_cpu_entry)
        + sizeof(struct ucollection_process_cpu_item) * totalCount;
    entry_ = (struct ucollection_process_cpu_entry *)malloc(totalSize);
    if (entry_ == NULL) {
        return;
    }
    memset_s(entry_, totalSize, 0, totalSize);
    entry_->magic = magic;
    entry_->total_count = totalCount;
    entry_->cur_count = 0;
    entry_->filter.pid = pid;
}

ProcessCpuData::~ProcessCpuData()
{
    if (entry_ == NULL) {
        return;
    }
    free(entry_);
    entry_ = nullptr;
}

struct ucollection_process_cpu_item* ProcessCpuData::GetNextProcess()
{
    if (entry_ == NULL || current_ >= entry_->cur_count) {
        return nullptr;
    }

    struct ucollection_process_cpu_item *item = &entry_->datas[current_];
    current_++;
    return item;
}
} // HiviewDFX
} // OHOS