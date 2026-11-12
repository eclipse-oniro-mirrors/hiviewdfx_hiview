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

#include "thread_inspector.h"

#include "allocator/map_allocator.h"
#include "thread_snapshot_impl.h"

namespace OHOS {

ThreadInspector::ThreadInspector(pid_t pid, pid_t tid, Allocator<ThreadInspector> allocator)
{
    Allocator<ThreadSnapshotImpl> implAllocator(allocator);
    impl_ = implAllocator.MakeUnique(pid, tid, implAllocator);
}

ThreadInspector::~ThreadInspector() {}

bool ThreadInspector::CaptureThreadInfo(ThreadInfoList &threads, size_t &tidsSize)
{
    return impl_->CaptureThreadInfo(threads, tidsSize);
}

void ThreadInspector::GetLocalThreadInfo()
{
    impl_->GetLocalThreadInfo();
}

void ThreadInspector::SetThreadInfo(ThreadInfoList &threads)
{
    impl_->SetThreadInfo(threads);
}

void ThreadInspector::SetForkDone()
{
    impl_->SetForkDone();
}

}  // namespace OHOS
