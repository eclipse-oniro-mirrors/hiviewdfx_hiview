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

#include "orphanmem.h"

#include <atomic>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <dlfcn.h>
#include <pthread.h>
#include <sanitizer/memgraph_interface.h>
#include <securec.h>
#include <semaphore.h>
#include <sys/cdefs.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>  // for pid_t, uid_t, gid_t
#include <sys/wait.h>   // for waitpid()
#include <syscall.h>
#include <unistd.h>  // for getpid(), gettid()
#include <cstdlib>

#include "map_allocator.h"
#include "leak_info.h"
#include "log.h"
#include "orphanmem_detector.h"
#include "performance_tracer.h"
#include "process_mappings.h"
#include "process_pipe.h"
#include "thread_inspector.h"
#include "collect_thread.h"

#if defined(__aarch64__)
extern "C" {
    void gwp_asan_disable();
    void gwp_asan_enable();
}
#endif

namespace {
constexpr unsigned int DEFAULT_SNAPSHOT_THREADS = 4;
constexpr unsigned int MAX_SNAPSHOT_THREADS = 128;
constexpr size_t SNAPSHOT_MAGIC_SIZE = sizeof("NSNAP1.0") - 1;
constexpr size_t MAX_LEAK_OUTPUT_LEN = 64 * 1024 * 1024;
}

namespace OHOS {

struct SnapshotWorkContext {
    const MAP_ALLOCATOR::unordered_set<Range, RangeHash> *leaked;
    char *buffer;
    size_t startIndex;
    size_t endIndex;
    size_t headerSize;
    size_t totalSize;
};

void *SnapshotWorkerThread(void *arg)
{
    SnapshotWorkContext *ctx = static_cast<SnapshotWorkContext *>(arg);
    size_t entrySize = sizeof(uintptr_t) + sizeof(size_t);

    auto it = ctx->leaked->begin();
    std::advance(it, ctx->startIndex);

    for (size_t i = ctx->startIndex; i < ctx->endIndex; ++i) {
        const Range &range = *it;

        uintptr_t addr = range.begin;
        size_t size = range.Size();
        size_t writeOffset = ctx->headerSize + i * entrySize;

        errno_t ret = memcpy_s(ctx->buffer + writeOffset, ctx->totalSize - writeOffset, &addr, sizeof(addr));
        if (ret != 0) {
            return nullptr;
        }
        ret = memcpy_s(ctx->buffer + writeOffset + sizeof(uintptr_t), ctx->totalSize - writeOffset - sizeof(uintptr_t),
                       &size, sizeof(size));
        if (ret != 0) {
            return nullptr;
        }

        ++it;
    }
    return nullptr;
}

extern "C" __attribute__((visibility("default"))) void UpdateByMusl(LeakGraphSTL &leakGraph)
{
    void *handler = dlopen("libclang_rt.memgraph.so", RTLD_LAZY);
    if (handler == nullptr) {
        HILOGE("dlopen failed for libclang_rt.memgraph.so");
        return;
    }
    auto getBlockInfo = (int (*)(unsigned long, block_info_t *))dlsym(handler, "get_block_info");
    if (getBlockInfo == nullptr) {
        HILOGE("get_block_info == nullptr");
        dlclose(handler);
        return;
    }

    int failCnt = 0;
    for (auto &[address, node] : leakGraph) {
        block_info_t blockInfo{};
        int ret = getBlockInfo(static_cast<unsigned long>(address), &blockInfo);
        if (ret == 0) {
            failCnt++;
            continue;
        }
        bool success = true;
        if (blockInfo.type_name != nullptr) {
            node.type = blockInfo.type_name;
        } else {
            success = false;
        }
        if (blockInfo.name != nullptr) {
            node.name = blockInfo.name;
        } else {
            success = false;
        }
        node.pc = blockInfo.malloc_pc;
        node.capacity = blockInfo.size;
        if (!success) {
            failCnt++;
        }
    }
    if (failCnt > 0) {
        HILOGW("get_block_info failed %{public}d time(s) in %{public}zu", failCnt, leakGraph.size());
    } else {
        HILOGI("get_block_info all success in %{public}zu time(s)", leakGraph.size());
    }
    dlclose(handler);
}

void ConvertLeakGraphToSTL(const LeakGraphALC &leakGraph, LeakGraphSTL &leakGraphSTL)
{
    for (const auto &[key, node] : leakGraph) {
        NodeSTL stlNode;
        stlNode.capacity = node.capacity;
        for (const auto &[offset, ptr] : node.references) {
            stlNode.references[offset] = ptr;
        }
        leakGraphSTL[key] = stlNode;
    }
}

bool AssembleLeakData(LeakInfo &leakInfo, int format,
                      const std::chrono::time_point<std::chrono::high_resolution_clock> &startTime, std::string &output)
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    MemLeakTimeInfo timeInfo = {startTime, endTime, duration};
    bool assembleOk = false;
    switch (format) {
        case TYPE_TXT:
            assembleOk = leakInfo.ToTxt(output, timeInfo);
            break;
        case TYPE_VECTOR:
            assembleOk = leakInfo.ToVector(output);
            break;
        case TYPE_SNAPSHOT:
            assembleOk = leakInfo.ToHeapSnapshot(output, timeInfo);
            break;
        default:
            return false;
    }
    return assembleOk;
}

struct TxtFormatContext {
    Heap &heap;
    OrphanMemDetector &detector;
    PerformanceTracer &childTracer;
    const std::chrono::time_point<std::chrono::high_resolution_clock> &startTime;
    int format;
    size_t limit;
    std::string &output;
};

void ProcessTxtFormat(const TxtFormatContext &ctx)
{
    LeakGraphALC leakGraph(Allocator<LeakGraphALC::value_type>(ctx.heap));
    size_t leakCnt = 0;
    size_t leakBytes = 0;
    bool ok = ctx.detector.Detect(leakGraph, ctx.limit, &leakCnt, &leakBytes);
    if (!ok) {
        _exit(-1);
    }

    LeakGraphSTL leakGraphSTL;
    ConvertLeakGraphToSTL(leakGraph, leakGraphSTL);

    ctx.childTracer.Mark("UpdateByMusl start");
    UpdateByMusl(leakGraphSTL);
    ctx.childTracer.Mark("UpdateByMusl end");

    LeakInfo leakInfo;
    leakInfo.leakGraph_ = leakGraphSTL;
    ctx.childTracer.Mark("AssembleData start");
    if (!AssembleLeakData(leakInfo, ctx.format, ctx.startTime, ctx.output)) {
        _exit(-1);
    }
}

static void ProcessSnapshotWithThreads(const MAP_ALLOCATOR::unordered_set<Range, RangeHash> &leaked, char *buffer,
                                       size_t leakCnt, size_t headerSize, size_t totalSize)
{
    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) {
        threadCount = DEFAULT_SNAPSHOT_THREADS;
    }
    if (threadCount > MAX_SNAPSHOT_THREADS) {
        threadCount = MAX_SNAPSHOT_THREADS;
    }

    pthread_t *threads = new pthread_t[threadCount];
    SnapshotWorkContext *workCtx = new SnapshotWorkContext[threadCount];
    size_t chunkSize = (leakCnt + threadCount - 1) / threadCount;

    for (unsigned int i = 0; i < threadCount; i++) {
        workCtx[i].leaked = &leaked;
        workCtx[i].buffer = buffer;
        workCtx[i].startIndex = i * chunkSize;
        workCtx[i].endIndex = std::min((i + 1) * chunkSize, leakCnt);
        workCtx[i].headerSize = headerSize;
        workCtx[i].totalSize = totalSize;
        if (workCtx[i].startIndex < leakCnt) {
            pthread_create(&threads[i], nullptr, SnapshotWorkerThread, &workCtx[i]);
        } else {
            threads[i] = 0;
        }
    }

    for (unsigned int i = 0; i < threadCount; i++) {
        if (threads[i] != 0) {
            pthread_join(threads[i], nullptr);
        }
    }

    delete[] threads;
    delete[] workCtx;
}

void ProcessSnapshotFormat(Heap &heap, OrphanMemDetector &detector, std::string &output)
{
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked{Allocator<Range>(heap)};
    size_t leakCnt = 0;
    size_t leakBytes = 0;
    if (!detector.Detect(leaked, SIZE_MAX, &leakCnt, &leakBytes)) {
        _exit(-1);
    }
    size_t headerSize = SNAPSHOT_MAGIC_SIZE + sizeof(leakCnt);
    size_t entrySize = sizeof(uintptr_t) + sizeof(size_t);
    size_t totalSize = headerSize + leakCnt * entrySize;
    char *buffer = new char[totalSize]();
    errno_t ret = memcpy_s(buffer, totalSize, "NSNAP1.0", SNAPSHOT_MAGIC_SIZE);
    if (ret != 0) {
        delete[] buffer;
        _exit(-1);
    }
    ret = memcpy_s(buffer + SNAPSHOT_MAGIC_SIZE, totalSize - SNAPSHOT_MAGIC_SIZE, &leakCnt, sizeof(leakCnt));
    if (ret != 0) {
        delete[] buffer;
        _exit(-1);
    }

    ProcessSnapshotWithThreads(leaked, buffer, leakCnt, headerSize, totalSize);
    output.assign(buffer, totalSize);
    delete[] buffer;
}

void BlockSignal(int signal)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) != 0) {
        HILOGE("[tid=%{public}d] SIG_BLOCK sigprocmask failed errno=%{public}d", gettid(), errno);
    }
}

void UnblockSignal(int signal)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    if (sigprocmask(SIG_UNBLOCK, &mask, nullptr) != 0) {
        HILOGE("[tid=%{public}d] SIG_UNBLOCK sigprocmask failed errno=%{public}d", gettid(), errno);
    }
}

struct ThreadCollectorContext {
    Heap &heap;
    int parentPid;
    int parentTid;
    sem_t &mallocSem;
    int format;
    ProcessPipe &pipe;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    size_t limit;
    size_t &tidsSize;
    ThreadInspector &threadInspector;
    MAP_ALLOCATOR::vector<ThreadInfo> &threadInfo;
};

void RunChildProcess(ThreadCollectorContext &ctx, MAP_ALLOCATOR::vector<ThreadInfo> &threadInfo,
                     MAP_ALLOCATOR::vector<Mapping> &mappings, MAP_ALLOCATOR::vector<uintptr_t> &refs)
{
    PerformanceTracer childTracer("Child");
    OrphanMemDetector detector{Allocator<void>(ctx.heap)};
    childTracer.Mark("CollectAllocations start");
    if (!detector.CollectAllocations(threadInfo, mappings, refs)) {
        _exit(-1);
    }
    childTracer.Mark("CollectAllocations end");

#ifdef ORPHANMEM_FORMAT_COMPILE
    ctx.format = ORPHANMEM_FORMAT_COMPILE;
#endif

    std::string output;
    if (ctx.format == TYPE_TXT) {
        TxtFormatContext txtCtx{ctx.heap, detector, childTracer, ctx.startTime, ctx.format, ctx.limit, output};
        ProcessTxtFormat(txtCtx);
    } else if (ctx.format == TYPE_SNAPSHOT) {
        ProcessSnapshotFormat(ctx.heap, detector, output);
    }
    childTracer.Mark("AssembleData end");

    ProcessPipe pipeMain;
    pipeMain.SetFd(-1, ctx.pipe.GetWriteFd());
    childTracer.Mark("SendString start");
    if (!pipeMain.SendString(output)) {
        _exit(-1);
    }
    childTracer.Mark("SendString end");
    _exit(0);
}

int ThreadCollectorFunc(ThreadCollectorContext &ctx)
{
    MAP_ALLOCATOR::vector<Mapping> mappings(Allocator<Mapping>(ctx.heap));
    MAP_ALLOCATOR::vector<uintptr_t> refs(Allocator<uintptr_t>(ctx.heap));
    HILOGI("[tid=%{public}d] CaptureThreadInfo start", gettid());
    if (!ctx.threadInspector.CaptureThreadInfo(ctx.threadInfo, ctx.tidsSize)) {
        sem_post(&ctx.mallocSem);
        ctx.threadInspector.SetForkDone();
        return -1;
    }

    if (!MappingHelper::ProcessMappings(ctx.parentPid, mappings)) {
        sem_post(&ctx.mallocSem);
        ctx.threadInspector.SetForkDone();
        return -1;
    }
    sem_post(&ctx.mallocSem);

    int ret = fork();
    if (ret < 0) {
        HILOGE("[tid=%{public}d] fork failed", gettid());
        ctx.threadInspector.SetForkDone();
        return -1;
    }
    if (ret == 0) {
        // The parent disables malloc (malloc_disable) before signaling this
        // thread, and re-enables it only after sem_wait returns.  fork() happens
        // right after sem_post, so it races with the parent's malloc_enable():
        // the child often inherits the malloc-disabled state.  Any malloc in the
        // child (std::string, std::thread, std::mutex, malloc_iterate, ...)
        // would then block forever, which is the root cause of the liborphanmem
        // hang.
        //
        // fork() (unlike raw SYS_clone) also runs jemalloc's pthread_atfork
        // child handler, which resets tcache/arena metadata so the child can
        // safely use jemalloc.  Re-enable malloc here in the child to clear the
        // inherited disabled flag deterministically.
        malloc_enable();
        RunChildProcess(ctx, ctx.threadInfo, mappings, refs);
    } else {
        // Release all threads blocked in futex_wait inside
        // ThreadCaptureSignalHandler.  Previously this used mallocMtx.unlock()
        // which could deadlock with fork()'s loader lock when a signalled
        // thread was interrupted inside dlopen (holding the loader read lock).
        // The futex wake is lock-free and breaks the lock-order-inversion.
        ctx.threadInspector.SetForkDone();
        UnblockSignal(MUSL_SIGNAL_NATIVE_REMOTE);
    }
    return 0;
}

void RunMainProcessWithMallocDisabled(CollectThread &thread, sem_t &mallocSem,
                                      ThreadInspector &threadInspector)
{
    BlockSignal(MUSL_SIGNAL_NATIVE_REMOTE);
    thread.Start();
    HILOGI("start malloc_disable");
    malloc_disable();
#if defined(__aarch64__)
    gwp_asan_disable();
#endif
    threadInspector.GetLocalThreadInfo();
    thread.Signal();
    sem_wait(&mallocSem);
#if defined(__aarch64__)
    gwp_asan_enable();
#endif
    malloc_enable();
    UnblockSignal(MUSL_SIGNAL_NATIVE_REMOTE);
}

static bool InitPipeAndSem(sem_t &mallocSem, OHOS::ProcessPipe &pipe, PerformanceTracer &tracer)
{
    sem_init(&mallocSem, 0, 0);
    if (!pipe.IsOpened()) {
        HILOGE("pipe open failed");
        sem_destroy(&mallocSem);
        return false;
    }
    tracer.Mark("finish init");
    return true;
}

static bool ReceiveOutput(OHOS::ProcessPipe &pipe, sem_t &mallocSem, std::string &output)
{
    bool ok = pipe.ReceiveString(output);
    sem_destroy(&mallocSem);
    return ok;
}

int64_t GetUnreachableMemory(LeakInfo &info, size_t limit, int format)
{
    HILOGI("start GetUnreachableMemory");
    const char *ldPreload = getenv("LD_PRELOAD");
    if (ldPreload != nullptr && ldPreload[0] != '\0') {
        HILOGI("LD_PRELOAD is set: %{public}s", ldPreload);
    }
#ifdef ORPHANMEM_LIMIT_COMPILE
    limit = ORPHANMEM_LIMIT_COMPILE;
#endif
    PerformanceTracer tracer("Main");
    int parentPid = getpid();
    int parentTid = gettid();
    prctl(PR_SET_DUMPABLE, 1L, 0L, 0L, 0L);
    prctl(PR_SET_PTRACER, parentPid, 0L, 0L, 0L);
    auto startTime = std::chrono::high_resolution_clock::now();
    sem_t mallocSem;
    Heap heap;
    OHOS::ProcessPipe pipe;

    size_t tidsSize = 0;
    ThreadInspector threadInspector(parentPid, parentTid, Allocator<ThreadInspector>(heap));
    MAP_ALLOCATOR::vector<ThreadInfo> threadInfo((Allocator<ThreadInfo>(heap)));
    threadInspector.SetThreadInfo(threadInfo);
    ThreadCollectorContext ctx{heap, parentPid, parentTid, mallocSem, format, pipe, startTime, limit,
                               tidsSize,  threadInspector, threadInfo};
    CollectThread thread{[&ctx]() mutable -> int {
        return ThreadCollectorFunc(ctx);
    }};
    if (!InitPipeAndSem(mallocSem, pipe, tracer)) {
        return EC_FAILED;
    }
    RunMainProcessWithMallocDisabled(thread, mallocSem, threadInspector);

    int ret = thread.Join();
    tracer.Mark("thread.Join");
    HILOGI("tids size: %{public}zu", tidsSize);
    if (ret != 0) {
        HILOGE("thread exited with error: %{public}d", ret);
        return EC_FAILED;
    }

    tracer.Mark("ReceiveString start");
    if (!ReceiveOutput(pipe, mallocSem, info.leakOutput)) {
        return EC_FAILED;
    }
    tracer.Mark("ReceiveString end");

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    HILOGI("GetUnreachableMemory total cost: %{public}lld ms", duration.count());
    return EC_SUCCESS;
}

int64_t GetMemLeak(int format, std::string &leakStr, size_t limit)
{
    LeakInfo info;
    int64_t errCode = GetUnreachableMemory(info, limit, format);
    if (errCode != EC_SUCCESS) {
        HILOGE("failed to GetUnreachableMemory, errCode: 0x%{public}llx", static_cast<unsigned long long>(errCode));
        return errCode;
    }

    leakStr = info.leakOutput;

    HILOGI("GetMemLeakString success");
    return EC_SUCCESS;
}

bool GetMemLeakString(int format, std::string &leakStr, size_t limit)
{
    return GetMemLeak(format, leakStr, limit) == EC_SUCCESS;
}
}  // OHOS

extern "C" {
const char *OHOS_GetErrorMessage(int64_t errCode)
{
    switch (errCode) {
        case EC_SUCCESS:
            return "Success";
        case EC_LD_PRELOAD:
            return "LD_PRELOAD is set";
        case EC_FAILED:
        default:
            return "Operation failed";
    }
}

int64_t OHOS_GetMemLeak(int format, char **output, size_t limit, int *length)
{
    if (output == nullptr) {
        return EC_FAILED;
    }

    std::string leakStr;
    int64_t errCode = OHOS::GetMemLeak(format, leakStr, limit);
    if (errCode != EC_SUCCESS) {
        return errCode;
    }

    size_t len = leakStr.length();
    if (len == 0 || len > MAX_LEAK_OUTPUT_LEN) {
        return EC_FAILED;
    }
    *output = (char *)malloc(len);  // free by caller
    if (*output == nullptr) {
        return EC_FAILED;
    }
    errno_t ret = memcpy_s(*output, len, leakStr.data(), leakStr.length());
    if (ret != 0) {
        free(*output);
        *output = nullptr;
        return EC_FAILED;
    }
    if (length != nullptr) {
        *length = static_cast<int>(len);
    }
    return EC_SUCCESS;
}

bool OHOS_GetMemLeakString(int format, char **output, size_t limit, int *length)
{
    return OHOS_GetMemLeak(format, output, limit, length) == EC_SUCCESS;
}
}
