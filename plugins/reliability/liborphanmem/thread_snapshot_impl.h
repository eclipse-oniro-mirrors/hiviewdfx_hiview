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

#ifndef LIBORPHANMEM_THREAD_SNAPSHOT_IMPL_H
#define LIBORPHANMEM_THREAD_SNAPSHOT_IMPL_H

#include <atomic>
#include <mutex>
#include <semaphore.h>
#include <pthread.h>

#include "allocator/map_allocator.h"

namespace OHOS {
inline constexpr int REGS_MAX = 32;
inline constexpr int IDX_SP = 31;
using TidList = MAP_ALLOCATOR::vector<pid_t>;
struct ThreadInfo {
    pid_t tid;
    uintptr_t regs[REGS_MAX];
    std::pair<uintptr_t, uintptr_t> stack;
};
using ThreadInfoList = MAP_ALLOCATOR::vector<ThreadInfo>;

struct CaptureContext {
    std::atomic<int> threadSentCount;
    std::atomic<int> threadStartCount;
    std::atomic<int> threadCompletedCount;
    ThreadInfoList *capturedThreadInfo;
    std::atomic<int> forkDone;  // futex word: 0 = not done, 1 = done
    pid_t tid;
    pthread_mutex_t capturedThreadInfoMtx;
};

class ThreadSnapshotImpl {
public:
    ThreadSnapshotImpl(pid_t pid, pid_t tid, Allocator<ThreadSnapshotImpl> &allocator);
    ~ThreadSnapshotImpl();
    bool CaptureThreadInfo(ThreadInfoList &threads, size_t &tidsSize);
    bool ListThreads(TidList &tids);
    bool CaptureThreads();
    bool GetLocalThreadInfo();
    void SetThreadInfo(ThreadInfoList &threads);
    void SetForkDone();

private:
    bool GetLocalRegs(uintptr_t *regs);
    int WaitTimeout(int timeoutMs);
    int CaptureThread(pid_t tid);
    bool ClearCapturedThreads();
    int CaptureAttach(pid_t tid);
    void CaptureDetach(pid_t tid, unsigned int signal);
    bool CaptureSingleThreadInfo(pid_t tid, ThreadInfo &threadInfo);
    bool ReadThreadIdsFromDir(int fd, TidList &tids);
    void InitSignalHandler();
    void BlockSignal(int signal);
    void UnblockSignal(int signal);
    bool CollectThreadInfo(TidList &tids);
    Allocator<ThreadSnapshotImpl> allocator_;
    pid_t pid_;
    pid_t tid_;
    pid_t tidSelf_;
    MAP_ALLOCATOR::map<pid_t, unsigned int> capturedThreadStates_;
    CaptureContext captureContext;
};

}  // namespace OHOS

#endif