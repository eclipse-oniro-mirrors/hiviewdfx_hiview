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

#include "thread_snapshot_impl.h"

#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <linux/futex.h>
#include <asm/ptrace.h>
#include <ctime>
#include <elf.h>
#include <fcntl.h>
#include <securec.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unique_fd.h>
#include <unistd.h>

#include "dfx_signal_handler.h"
#include "log.h"

namespace {
constexpr size_t PID_BUF_SIZE = 11;
constexpr size_t PROC_PATH_SIZE = 256;
constexpr size_t DIRENT_BUF_SIZE = 4096;
constexpr int LOCALDUMP_TIMEOUT_MS = 500;  // Reduced from 1000ms to limit malloc_disable duration
constexpr long FUTEX_WAIT_TIMEOUT_NS = 500000000;
constexpr long NANOSECONDS_PER_MILLISECOND = 1000000;
constexpr size_t INITIAL_THREAD_INFO_CAPACITY = 10000;

void NanosleepWithRetry(struct timespec &ts)
{
    int ret;
    do {
        ret = nanosleep(&ts, &ts);
    } while (ret == -1 && errno == EINTR);
}
}

namespace OHOS {

ThreadSnapshotImpl::ThreadSnapshotImpl(pid_t pid, pid_t tid, Allocator<ThreadSnapshotImpl> &allocator)
    : allocator_(allocator),
      pid_(pid),
      tid_(tid),
      tidSelf_(0),
      capturedThreadStates_(Allocator<MAP_ALLOCATOR::map<pid_t, unsigned int>::value_type>(allocator)),
      captureContext{0, 0, 0, nullptr, 0, 0, PTHREAD_MUTEX_INITIALIZER}
{
}

ThreadSnapshotImpl::~ThreadSnapshotImpl() {}

void ThreadCaptureSignalHandler(int sig, siginfo_t *si, void *context)
{
    (void)sig;
    if (si == nullptr || context == nullptr) {
        HILOGE("[tid=%{public}d] ThreadCaptureSignalHandler: invalid params", gettid());
        return;
    }
#if defined(__aarch64__)
    ucontext_t *uc = static_cast<ucontext_t *>(context);
    mcontext_t *regs = &uc->uc_mcontext;
    CaptureContext *sigCtx = static_cast<CaptureContext *>(si->si_value.sival_ptr);

    pid_t tid = gettid();
    int pos = atomic_fetch_add(&sigCtx->threadStartCount, 1);
    pthread_mutex_lock(&sigCtx->capturedThreadInfoMtx);
    if (pos >= static_cast<int>(sigCtx->capturedThreadInfo->size())) {
        HILOGE("[tid=%{public}d] ThreadCaptureSignalHandler: pos %{public}d out of bounds size %{public}zu", tid, pos,
               sigCtx->capturedThreadInfo->size());
        pthread_mutex_unlock(&sigCtx->capturedThreadInfoMtx);
        return;
    }
    ThreadInfo info;
    info.tid = tid;
    for (int i = 0; i < IDX_SP; i++) {
        info.regs[i] = regs->regs[i];
    }
    info.stack = std::make_pair(regs->sp, 0);
    (*sigCtx->capturedThreadInfo)[pos] = std::move(info);
    sigCtx->threadCompletedCount++;
    pthread_mutex_unlock(&sigCtx->capturedThreadInfoMtx);
    if (sigCtx->tid != tid) {
        // Block on a futex instead of spinning on sched_yield.
        // The original code used mallocMtx->lock()/unlock() to freeze threads
        // until fork() completes.  However, a thread interrupted while holding
        // the dynamic-linker loader lock (e.g. inside dlopen) would block on
        // mallocMtx inside the signal handler, while the collect thread's
        // fork() blocks on the loader write lock in __ldso_atfork — a classic
        // lock-order-inversion deadlock that causes appfreeze.
        // Spinning on an atomic with sched_yield avoids the deadlock but causes
        // CPU starvation when hundreds of threads spin simultaneously, delaying
        // CaptureThreadInfo and keeping malloc_disable active for 15+ seconds.
        // Futex wait blocks the thread in the kernel without consuming CPU,
        // avoiding both the deadlock and CPU starvation.
        if (sigCtx->forkDone.load(std::memory_order_acquire) == 0) {
            struct timespec ts;
            ts.tv_sec = 1;   // 1s timeout
            ts.tv_nsec = FUTEX_WAIT_TIMEOUT_NS;  // 500ms timeout
            syscall(SYS_futex, &sigCtx->forkDone, FUTEX_WAIT_PRIVATE, 0, &ts, nullptr, 0);
        }
    }
#endif
}

void ThreadSnapshotImpl::InitSignalHandler()
{
    struct sigaction action;
    (void)memset_s(&action, sizeof(action), 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, MUSL_SIGNAL_MEMCHECK);
    action.sa_flags = SA_RESTART | SA_SIGINFO;
    action.sa_sigaction = ThreadCaptureSignalHandler;
    if (sigaction(MUSL_SIGNAL_MEMCHECK, &action, nullptr) != 0) {
        HILOGE("[tid=%{public}d] InitSignalHandler: sigaction failed errno=%{public}d", gettid(), errno);
        return;
    }
    UnblockSignal(MUSL_SIGNAL_MEMCHECK);
}

void ThreadSnapshotImpl::BlockSignal(int signal)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) != 0) {
        HILOGE("[tid=%{public}d] BlockSignal: sigprocmask failed errno=%{public}d", gettid(), errno);
    }
}

void ThreadSnapshotImpl::UnblockSignal(int signal)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    if (sigprocmask(SIG_UNBLOCK, &mask, nullptr) != 0) {
        HILOGE("[tid=%{public}d] UnblockSignal: sigprocmask failed errno=%{public}d", gettid(), errno);
    }
}

int ThreadSnapshotImpl::WaitTimeout(int timeoutMs)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = NANOSECONDS_PER_MILLISECOND;

    while (timeoutMs > 0) {
        if (captureContext.threadCompletedCount >= captureContext.threadSentCount || timeoutMs == 0) {
            break;
        }
        NanosleepWithRetry(ts);
        timeoutMs--;
    }
    return captureContext.threadCompletedCount;
}

bool ThreadSnapshotImpl::ReadThreadIdsFromDir(int fd, TidList &tids)
{
    struct LinuxDirent64 {
        uint64_t dIno;
        int64_t dOff;
        unsigned short dReclen;
        unsigned char dType;
        char dName[];
    } __attribute((packed));
    char direntBuf[DIRENT_BUF_SIZE];
    ssize_t bytesRead;

    do {
        bytesRead = syscall(SYS_getdents64, fd, direntBuf, sizeof(direntBuf));
        if (bytesRead < 0) {
            HILOGE("[tid=%{public}d] ReadThreadIdsFromDir: SYS_getdents64 failed errno=%{public}d", gettid(), errno);
            return false;
        }

        if (bytesRead == 0) {
            break;
        }

        ssize_t offset = 0;
        while (offset < bytesRead) {
            LinuxDirent64 *dirent = reinterpret_cast<LinuxDirent64 *>(direntBuf + offset);
            offset += dirent->dReclen;
            pid_t tid = atoi(dirent->dName);
            if (tid > 0 && tid != tidSelf_ && tid != tid_) {
                tids.push_back(tid);
            }
        }
    } while (bytesRead != 0);

    HILOGI("[tid=%{public}d] ReadThreadIdsFromDir: found %{public}zu threads", gettid(), tids.size());
    return true;
}

bool ThreadSnapshotImpl::ListThreads(TidList &tids)
{
    tids.clear();

    char pidBuf[PID_BUF_SIZE];
    char path[PROC_PATH_SIZE] = "/proc/";

    int snprintfRet = snprintf_s(pidBuf, sizeof(pidBuf), sizeof(pidBuf), "%d", pid_);
    if (snprintfRet < 0) {
        HILOGE("[tid=%{public}d] ListThreads: snprintf_s failed for pid=%{public}d", gettid(), pid_);
        return false;
    }

    size_t ret = strlcat(path, pidBuf, sizeof(path));
    if (ret >= sizeof(path)) {
        HILOGE("[tid=%{public}d] ListThreads: strlcat failed for pid=%{public}d", gettid(), pid_);
        return false;
    }
    ret = strlcat(path, "/task", sizeof(path));
    if (ret >= sizeof(path)) {
        HILOGE("[tid=%{public}d] ListThreads: strlcat failed for pid=%{public}d", gettid(), pid_);
        return false;
    }

    OHOS::UniqueFd fd(open(path, O_CLOEXEC | O_DIRECTORY | O_RDONLY));
    if (fd == -1) {
        HILOGE("[tid=%{public}d] ListThreads: open %{public}s failed errno=%{public}d", gettid(), path, errno);
        return false;
    }

    return ReadThreadIdsFromDir(fd.Get(), tids);
}

bool ThreadSnapshotImpl::ClearCapturedThreads()
{
    for (auto it = capturedThreadStates_.begin(); it != capturedThreadStates_.end();) {
        it = capturedThreadStates_.erase(it);
    }
    return true;
}

int ThreadSnapshotImpl::CaptureThread(pid_t tid)
{
    int ret;
    siginfo_t si;

    (void)memset_s(&si, sizeof(si), 0, sizeof(si));
    si.si_signo = MUSL_SIGNAL_MEMCHECK;
    si.si_code = SI_QUEUE;
    si.si_pid = pid_;
    si.si_uid = getuid();
    si.si_value.sival_ptr = &captureContext;
    ret = syscall(SYS_rt_tgsigqueueinfo, pid_, tid, MUSL_SIGNAL_MEMCHECK, &si);
    if (ret == 0) {
        capturedThreadStates_[tid] = 1;
    } else {
        capturedThreadStates_[tid] = -1;
        HILOGE("[tid=%{public}d] CaptureThread: failed to send signal to tid=%{public}d errno=%{public}d", gettid(),
               tid, errno);
    }
    captureContext.threadSentCount++;
    return 1;
}

bool ThreadSnapshotImpl::CollectThreadInfo(TidList &tids)
{
    bool hasNewThread;
    int loopCount = 0;
    constexpr int maxLoops = 3;  // Limit iterations to bound CaptureThreadInfo time
    do {
        if (!ListThreads(tids)) {
            ClearCapturedThreads();
            return false;
        }

        hasNewThread = false;

        for (auto it = tids.begin(); it != tids.end(); it++) {
            if (*it == 0) {
                continue;
            }
            auto captured = capturedThreadStates_.find(*it);
            if (captured != capturedThreadStates_.end()) {
                continue;
            }
            if (CaptureThread(*it) < 0) {
                ClearCapturedThreads();
                return false;
            }
            hasNewThread = true;
        }
        loopCount++;
    } while (hasNewThread && loopCount < maxLoops);
    return true;
}

bool ThreadSnapshotImpl::CaptureThreadInfo(ThreadInfoList &threads, size_t &tidsSize)
{
    TidList tids{Allocator<pid_t>(allocator_)};
    tidSelf_ = gettid();

    captureContext.capturedThreadInfo = &threads;
    captureContext.forkDone.store(0, std::memory_order_release);

    InitSignalHandler();

    // First, list all threads to know how many we need
    if (!ListThreads(tids)) {
        return false;
    }
    tidsSize = tids.size();

    // Resize the container to hold all threads (including local thread)
    constexpr int resizeAdd = 1000;
    threads.resize(tidsSize + resizeAdd);

    // Now send signals to all threads
    if (!CollectThreadInfo(tids)) {
        return false;
    }

    int completed = WaitTimeout(LOCALDUMP_TIMEOUT_MS);

    HILOGI("[tid=%{public}d] CaptureThreadInfo: completed=%{public}d sent=%{public}d", gettid(), completed,
           captureContext.threadSentCount.load());

    BlockSignal(MUSL_SIGNAL_MEMCHECK);
    BlockSignal(MUSL_SIGNAL_NATIVE_REMOTE);

    return true;
}

void ThreadSnapshotImpl::SetThreadInfo(ThreadInfoList &threads)
{
    captureContext.capturedThreadInfo = &threads;
    // Resize to a reasonable initial size
    captureContext.capturedThreadInfo->resize(INITIAL_THREAD_INFO_CAPACITY);
}

void ThreadSnapshotImpl::SetForkDone()
{
    captureContext.forkDone.store(1, std::memory_order_release);
    // Wake all threads blocked in futex_wait inside ThreadCaptureSignalHandler.
    syscall(SYS_futex, &captureContext.forkDone, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr, nullptr, 0);
}

bool ThreadSnapshotImpl::GetLocalRegs(uint64_t *regs)
{
#if defined(__aarch64__)
    asm volatile("stp x0, x1, [%[base], #0]\n"
                 "stp x2, x3, [%[base], #16]\n"
                 "stp x4, x5, [%[base], #32]\n"
                 "stp x6, x7, [%[base], #48]\n"
                 "stp x8, x9, [%[base], #64]\n"
                 "stp x10, x11, [%[base], #80]\n"
                 "stp x12, x13, [%[base], #96]\n"
                 "stp x14, x15, [%[base], #112]\n"
                 "stp x16, x17, [%[base], #128]\n"
                 "stp x18, x19, [%[base], #144]\n"
                 "stp x20, x21, [%[base], #160]\n"
                 "stp x22, x23, [%[base], #176]\n"
                 "stp x24, x25, [%[base], #192]\n"
                 "stp x26, x27, [%[base], #208]\n"
                 "stp x28, x29, [%[base], #224]\n"
                 "str x30, [%[base], #240]\n"
                 "mov %[sp], sp\n"
                 : [base] "+r"(regs), [sp] "=r"(regs[IDX_SP])
                 :
                 : "memory");
    return true;
#endif
    (void)regs;
    return false;
}

bool ThreadSnapshotImpl::GetLocalThreadInfo()
{
    if (captureContext.capturedThreadInfo == nullptr) {
        HILOGE("[tid=%{public}d] GetLocalThreadInfo: capturedThreadInfo is null", gettid());
        return false;
    }
    int pos = atomic_fetch_add(&captureContext.threadStartCount, 1);
    pthread_mutex_lock(&captureContext.capturedThreadInfoMtx);
    if (pos >= static_cast<int>(captureContext.capturedThreadInfo->size())) {
        HILOGE("[tid=%{public}d] GetLocalThreadInfo: pos %{public}d out of bounds size %{public}zu", gettid(), pos,
               captureContext.capturedThreadInfo->size());
        pthread_mutex_unlock(&captureContext.capturedThreadInfoMtx);
        return false;
    }
    ThreadInfo info;
    info.tid = gettid();
    GetLocalRegs(info.regs);
    info.stack = std::make_pair(info.regs[IDX_SP], 0);
    (*captureContext.capturedThreadInfo)[pos] = std::move(info);
    captureContext.threadCompletedCount++;
    pthread_mutex_unlock(&captureContext.capturedThreadInfoMtx);
    HILOGI("[tid=%{public}d] ThreadSnapshotImpl::GetLocalThreadInfo", gettid());
    return true;
}

}  // namespace OHOS