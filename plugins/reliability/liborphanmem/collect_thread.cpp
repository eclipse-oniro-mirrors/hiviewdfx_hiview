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

#include "collect_thread.h"

#include <cstdint>
#include <cstdio>
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "log.h"

namespace OHOS {

CollectThread::CollectThread(const std::function<int()> &func)
{
    func_ = func;
    joined_ = false;
    sem_init(&sem_, 0, 0);
}

CollectThread::~CollectThread()
{
    if (!joined_ && thread_ != 0) {
        pthread_join(thread_, nullptr);
        joined_ = true;
    }
    sem_destroy(&sem_);
}

bool CollectThread::Start()
{
    pid_t tid = gettid();

    auto proxy = [](void *arg) -> void* {
        HILOGI("liborphanmem collect thread start");
        prctl(PR_SET_NAME, "liborphanmem collect");
        CollectThread *self = reinterpret_cast<CollectThread *>(arg);
        sem_wait(&self->sem_);
        return reinterpret_cast<void *>(static_cast<intptr_t>(self->func_()));
    };

    int ret = pthread_create(&thread_, nullptr, proxy, this);
    if (ret != 0) {
        HILOGE("[tid=%{public}d] pthread_create failed, ret=%{public}d", tid, ret);
        return false;
    }
    HILOGI("[tid=%{public}d] pthread_create success", tid);

    return true;
}

void CollectThread::Signal()
{
    sem_post(&sem_);
}

int CollectThread::Join()
{
    pid_t tid = gettid();

    if (joined_ || thread_ == 0) {
        HILOGW("[tid=%{public}d] already joined or no thread", tid);
        return -1;
    }

    void *retval = nullptr;
    int ret = pthread_join(thread_, &retval);
    if (ret != 0) {
        HILOGE("[tid=%{public}d] pthread_join failed, err=%{public}d", tid, ret);
        return -1;
    }
    joined_ = true;

    HILOGI("[tid=%{public}d] pthread_join success", tid);
    return reinterpret_cast<intptr_t>(retval);
}
}  // namespace OHOS
