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

#include <chrono>
#include <cinttypes>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <dlfcn.h>

#include "../include/orphanmem.h"

using namespace std;

using GetMemLeakFunc = int64_t (*)(int, char **, size_t, int *);
using GetErrorMessageFunc = const char *(*)(int64_t);

namespace {
constexpr const char *const SO_NAME = "liborphanmem.z.so";
constexpr const char *const SYMBOL_NAME_GET_MEMLEAK = "OHOS_GetMemLeak";
constexpr const char *const SYMBOL_NAME_GET_ERROR_MESSAGE = "OHOS_GetErrorMessage";
int g_argc = 0;
char **g_argv = nullptr;
vector<void *> g_allMalloc;
vector<int *> g_allArray;
}

class OrphanMemDetector {
public:
    OrphanMemDetector()
    {
        handle_ = dlopen(SO_NAME, RTLD_LAZY);
        if (!handle_) {
            std::cerr << "Error loading " << SO_NAME << ": " << dlerror() << std::endl;
            return;
        }

        getMemLeakFunc_ = reinterpret_cast<GetMemLeakFunc>(dlsym(handle_, SYMBOL_NAME_GET_MEMLEAK));
        if (!getMemLeakFunc_) {
            std::cerr << "Failed to resolve symbol: " << SYMBOL_NAME_GET_MEMLEAK << ", error: " << dlerror()
                      << std::endl;
        }

        getErrorMessageFunc_ = reinterpret_cast<GetErrorMessageFunc>(dlsym(handle_, SYMBOL_NAME_GET_ERROR_MESSAGE));
        if (!getErrorMessageFunc_) {
            std::cerr << "Failed to resolve symbol: " << SYMBOL_NAME_GET_ERROR_MESSAGE << ", error: " << dlerror()
                      << std::endl;
        }

        std::cout << "dlopen " << SO_NAME << " finished." << std::endl << std::endl;
    }

    ~OrphanMemDetector()
    {
        if (handle_) {
            dlclose(handle_);
        }
    }

    void LeakDetect() const
    {
        if (getMemLeakFunc_ == nullptr) {
            std::cerr << "OHOS_GetMemLeak function is nullptr" << std::endl;
            return;
        }

        std::cout << "===== OHOS_GetMemLeak Examples =====" << std::endl;
        int64_t ret = TestGetMemLeak();
        if (getErrorMessageFunc_ != nullptr && ret < 0) {
            const char *errorMsg = getErrorMessageFunc_(ret);
            if (errorMsg != nullptr) {
                std::cout << "Error message for code 0x" << std::hex << ret << std::dec << ": " << errorMsg
                          << std::endl;
            }
        }
    }

private:
    int64_t TestGetMemLeak() const
    {
        const int formats[] = {TYPE_TXT, TYPE_VECTOR, TYPE_SNAPSHOT};
        const char *formatNames[] = {"TYPE_TXT", "TYPE_VECTOR", "TYPE_SNAPSHOT"};
        int64_t lastRet = 0;

        for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); i++) {
            char *output = nullptr;
            int length = 0;
            int64_t ret = getMemLeakFunc_(formats[i], &output, SIZE_MAX, &length);
            lastRet = ret;
            std::cout << "\nOHOS_GetMemLeak(" << formatNames[i] << ") returned: 0x" << std::hex << ret << std::dec
                      << " length:" << length << std::endl;
            if (ret != 0 || output == nullptr) {
                continue;
            }
            std::string outputStr;
            outputStr.assign(output, length);
            if (i == 0) {
                // only TYEP_TXT, show contents
                std::cout << "Output:\n" << outputStr << std::endl;
            }
            std::string filename = "/data/local/tmp/memleak_output_";
            switch (formats[i]) {
                case TYPE_TXT:
                    filename += "txt";
                    break;
                case TYPE_VECTOR:
                    filename += "vector";
                    break;
                case TYPE_SNAPSHOT:
                    filename += "snapshot";
                    break;
                default:
                    filename += "unknown";
                    break;
            }

            std::ofstream outfile(filename, std::ios::binary);
            if (outfile.is_open()) {
                outfile.write(output, length);
                outfile.close();
                std::cout << "Output saved to: " << filename << std::endl;
            } else {
                std::cerr << "Failed to open file: " << filename << " for writing" << std::endl;
            }
            free(output);
        }

        return lastRet;
    }

private:
    void *handle_ = nullptr;
    GetMemLeakFunc getMemLeakFunc_ = nullptr;
    GetErrorMessageFunc getErrorMessageFunc_ = nullptr;
};

namespace {
int GetCnt(int argvCnt)
{
    if (g_argc <= argvCnt) {
        return 0;
    }
    const char *arg = g_argv[argvCnt];
    if (!arg || arg[0] == '\0') {
        return 0;
    }
    char *endptr;
    long val = strtol(arg, &endptr, 10);
    if (endptr == arg || *endptr != '\0') {
        return 0;
    }
    return val;
}

void CreateLineLeaks(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateLineLeaks\n");

    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]mallocing\n", i, cnt);
        void **o = static_cast<void **>(malloc(sizeof(void *)));
        void **p = static_cast<void **>(malloc(sizeof(void *)));
        void **q = static_cast<void **>(malloc(sizeof(void *)));

        if (o == nullptr || p == nullptr || q == nullptr) {
            free(o);
            free(p);
            free(q);
            continue;
        }

        *o = p;
        *p = q;

        printf("leak o: %zx, *o: %zx\n", reinterpret_cast<size_t>(o), reinterpret_cast<size_t>(*o));
        printf("leak p: %zx, *p: %zx\n", reinterpret_cast<size_t>(p), reinterpret_cast<size_t>(*p));
        printf("leak q: %zx, *q: %zx\n", reinterpret_cast<size_t>(q), reinterpret_cast<size_t>(*q));
    }
}

void CreateSingleLeaks(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateSingleLeaks\n");

    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]\n", i, cnt);
        void **o = static_cast<void **>(malloc(sizeof(void *)));
        void **p = static_cast<void **>(malloc(sizeof(void *)));
        void **q = static_cast<void **>(malloc(sizeof(void *)));

        if (o == nullptr || p == nullptr || q == nullptr) {
            free(o);
            free(p);
            free(q);
            continue;
        }

        printf("leak o: %zx\n", reinterpret_cast<size_t>(o));
        printf("leak p: %zx\n", reinterpret_cast<size_t>(p));
        printf("leak q: %zx\n", reinterpret_cast<size_t>(q));
    }
}

void CreateLoopLeaks(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateLoopLeaks\n");

    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]\n", i, cnt);
        void **o = static_cast<void **>(malloc(sizeof(void *)));
        void **p = static_cast<void **>(malloc(sizeof(void *)));
        void **q = static_cast<void **>(malloc(sizeof(void *)));

        if (o == nullptr || p == nullptr || q == nullptr) {
            free(o);
            free(p);
            free(q);
            continue;
        }

        *o = p;
        *p = q;
        *q = o;

        printf("leak o: %zx, *o: %zx\n", reinterpret_cast<size_t>(o), reinterpret_cast<size_t>(*o));
        printf("leak p: %zx, *p: %zx\n", reinterpret_cast<size_t>(p), reinterpret_cast<size_t>(*p));
        printf("leak q: %zx, *q: %zx\n", reinterpret_cast<size_t>(q), reinterpret_cast<size_t>(*q));
    }
}

void CreateLoopLeaks2(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateLoopLeaks\n");

    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]\n", i, cnt);
        void **o = static_cast<void **>(malloc(sizeof(void *)));
        void **p = static_cast<void **>(malloc(sizeof(void *)));

        if (o == nullptr || p == nullptr) {
            free(o);
            free(p);
            continue;
        }

        *o = p;
        *p = o;

        printf("leak o: %zx, *o: %zx\n", reinterpret_cast<size_t>(o), reinterpret_cast<size_t>(*o));
        printf("leak p: %zx, *p: %zx\n", reinterpret_cast<size_t>(p), reinterpret_cast<size_t>(*p));
    }
}

void CreateArrayLeaks(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateArrayLeaks\n");
    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]\n", i, cnt);
        char *leak1 = new char[1024];
        printf("leak100: %zx\n", reinterpret_cast<size_t>(leak1));

        int *leak2 = new int[1024];
        printf("leak50: %zx\n", reinterpret_cast<size_t>(leak2));
    }
}

void CreateNoLeakArray(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateNoLeakArray\n");
    size_t beforeSize = g_allArray.size();
    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]\n", i, cnt);
        constexpr size_t cap = 1024 * 1024;
        int *leak = new int[cap];
        constexpr int fillValue = 1024;
        for (size_t j = 0; j < cap; j++) {
            *(leak + j) = fillValue;
        }
        g_allArray.push_back(leak);
    }
    size_t afterSize = g_allArray.size();
    printf("CreateNoLeakArray success cnts: %zu\n", afterSize - beforeSize);
}

void CreateNoLeak1M(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateNoLeak1M\n");
    size_t beforeSize = g_allMalloc.size();
    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]\n", i, cnt);
        void *p = malloc(1024 * 1024);
        if (p != nullptr) {
            g_allMalloc.push_back(p);
        }
    }
    size_t afterSize = g_allMalloc.size();
    printf("CreateNoLeak1M success cnts: %zu\n", afterSize - beforeSize);
}

void CreateNoLeak1K(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateNoLeak1K\n");
    size_t beforeSize = g_allMalloc.size();
    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]\n", i, cnt);
        void *p = malloc(1024);
        if (p != nullptr) {
            g_allMalloc.push_back(p);
        }
    }
    size_t afterSize = g_allMalloc.size();
    printf("CreateNoLeak1K success cnts: %zu\n", afterSize - beforeSize);
}

void CreateDequeLeak(int argvCnt)
{
    struct Dequeue {
        Dequeue *next{nullptr};
        Dequeue *prev{nullptr};
        int data{0};
    };
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateDequeLeak\n");
    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]:\n", i, cnt);
        Dequeue *o = static_cast<Dequeue *>(malloc(sizeof(Dequeue)));
        Dequeue *p = static_cast<Dequeue *>(malloc(sizeof(Dequeue)));
        Dequeue *q = static_cast<Dequeue *>(malloc(sizeof(Dequeue)));

        if (o == nullptr || p == nullptr || q == nullptr) {
            continue;
        }

        o->next = p;
        p->next = q;
        q->next = o;
        o->prev = q;
        q->prev = p;
        p->prev = o;
        printf("leak o: %zx, o->next: %zx, o->prev: %zx\n", reinterpret_cast<size_t>(o),
               reinterpret_cast<size_t>(o->next), reinterpret_cast<size_t>(o->prev));
        printf("leak p: %zx, p->next: %zx, p->prev: %zx\n", reinterpret_cast<size_t>(p),
               reinterpret_cast<size_t>(p->next), reinterpret_cast<size_t>(p->prev));
        printf("leak q: %zx, q->next: %zx, q->prev: %zx\n", reinterpret_cast<size_t>(q),
               reinterpret_cast<size_t>(q->next), reinterpret_cast<size_t>(q->prev));
    }
}

void CreateLoopWithHead(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateLoopWithHead\n");
    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]:\n", i, cnt);
        void **a = static_cast<void **>(malloc(sizeof(void *)));
        void **b = static_cast<void **>(malloc(sizeof(void *)));
        void **c = static_cast<void **>(malloc(sizeof(void *)));
        void **d = static_cast<void **>(malloc(sizeof(void *)));

        if (a == nullptr || b == nullptr || c == nullptr || d == nullptr) {
            free(a);
            free(b);
            free(c);
            free(d);
            continue;
        }

        *a = b;
        *b = c;
        *c = d;
        *d = b;
        printf("leak a: %zx, *a: %zx\n", reinterpret_cast<size_t>(a), reinterpret_cast<size_t>(*a));
        printf("leak b: %zx, *b: %zx\n", reinterpret_cast<size_t>(b), reinterpret_cast<size_t>(*b));
        printf("leak c: %zx, *c: %zx\n", reinterpret_cast<size_t>(c), reinterpret_cast<size_t>(*c));
        printf("leak d: %zx, *d: %zx\n", reinterpret_cast<size_t>(d), reinterpret_cast<size_t>(*d));
    }
}

void CreateUniquePtrLeak()
{
    constexpr size_t maxBufferSize = 1024;
    std::unique_ptr<char[]> content = std::make_unique<char[]>(maxBufferSize);
    printf("leak unique_ptr (make_unique): %zx\n", reinterpret_cast<size_t>(content.get()));

    std::unique_ptr<char[]> content2 = std::make_unique<char[]>(maxBufferSize);
    printf("leak unique_ptr (make_unique 2): %zx\n", reinterpret_cast<size_t>(content2.get()));

    if (getchar() == EOF) {
        printf("stdin is closed, continue\n");
    }
}

void CreateTLSLeak(int argvCnt)
{
    int cnt = GetCnt(argvCnt);
    if (cnt == 0) {
        return;
    }
    printf("\n\nCreateTLSLeak\n");

    pthread_key_t key;
    if (pthread_key_create(&key, nullptr) != 0) {
        printf("pthread_key_create failed\n");
        return;
    }

    int i = 0;
    while (i++ < cnt) {
        printf("[%d / %d]\n", i, cnt);
        void *ptr = malloc(1024);
        if (ptr != nullptr) {
            pthread_setspecific(key, ptr);
            printf("leak TLS: %zx\n", reinterpret_cast<size_t>(ptr));
        }
    }
}
}  // namespace

extern "C" int RunOrphanmemExample(int argc, char *argv[])
{
    g_argc = argc;
    g_argv = argv;

    constexpr int lineLeakCnt = 1;
    constexpr int singleLeakCnt = 2;
    constexpr int loopLeakCnt = 3;
    constexpr int loopLeak2Cnt = 4;
    constexpr int arrayLeakCnt = 5;
    constexpr int noLeak1MCnt = 6;
    constexpr int noLeak1KCnt = 7;
    constexpr int dequeLeakCnt = 8;
    constexpr int loopWithHeadCnt = 9;
    constexpr int noLeakArrayCnt = 10;
    constexpr int tlsLeakCnt = 11;
    CreateLineLeaks(lineLeakCnt);
    CreateSingleLeaks(singleLeakCnt);
    CreateLoopLeaks(loopLeakCnt);
    CreateLoopLeaks2(loopLeak2Cnt);
    CreateArrayLeaks(arrayLeakCnt);
    CreateNoLeak1M(noLeak1MCnt);
    CreateNoLeak1K(noLeak1KCnt);
    CreateDequeLeak(dequeLeakCnt);
    CreateLoopWithHead(loopWithHeadCnt);
    CreateNoLeakArray(noLeakArrayCnt);
    CreateTLSLeak(tlsLeakCnt);  // orphanmem_example_main 0 0 0 0 0 0 0 0 0 0 5

    auto uniquePtrLeakTask = []() {
        CreateUniquePtrLeak();
    };
    std::thread uniquePtrLeakThread(uniquePtrLeakTask);
    uniquePtrLeakThread.detach();
    constexpr int uniquePtrLeakWaitMs = 200;
    std::this_thread::sleep_for(std::chrono::milliseconds(uniquePtrLeakWaitMs));

    auto leakDetectionTask = []() {
        auto start = std::chrono::high_resolution_clock::now();

        OrphanMemDetector detector;
        detector.LeakDetect();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\nMemory leak detection took " << duration << " milliseconds.\n";
    };

    std::thread leakDetectThread(leakDetectionTask);
    leakDetectThread.join();

    return 0;
}