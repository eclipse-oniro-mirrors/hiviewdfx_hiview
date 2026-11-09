/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef LIBORPHANMEM_THREAD_INSPECTOR_H
#define LIBORPHANMEM_THREAD_INSPECTOR_H

#include <utility>

#include "allocator/map_allocator.h"
#include "thread_snapshot_impl.h"

namespace OHOS {

class ThreadInspector {
public:
    ThreadInspector(pid_t pid, pid_t tid, Allocator<ThreadInspector> allocator);
    ~ThreadInspector();

    bool CaptureThreadInfo(ThreadInfoList &threads, size_t &tidsSize);
    void GetLocalThreadInfo();
    void SetThreadInfo(ThreadInfoList &threads);
    void SetForkDone();

private:
    ThreadInspector(const ThreadInspector &) = delete;
    void operator=(const ThreadInspector &) = delete;

    Allocator<ThreadSnapshotImpl>::UniquePtr impl_;
};

}  // namespace OHOS

#endif
