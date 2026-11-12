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

#include "process_pipe.h"

#include <fcntl.h>

#include "map_allocator.h"

#define FDSAN_TAG_PIPE 4
#define PIPE_DOMAIN 0xD002DA0

constexpr uint64_t FDSAN_TAG = (static_cast<uint64_t>(PIPE_DOMAIN) << 32) | FDSAN_TAG_PIPE;

namespace OHOS {
ProcessPipe::ProcessPipe()
{
    int ret = pipe2(pipeFd_, O_CLOEXEC);
    if (ret < 0) {
        HILOGE("failed to pipe2, errno: %{public}d, err: %{public}s", errno, strerror(errno));
        return;
    }
    fdsan_exchange_owner_tag(pipeFd_[PIPE_IDX::READ], 0, FDSAN_TAG);
    fdsan_exchange_owner_tag(pipeFd_[PIPE_IDX::WRITE], 0, FDSAN_TAG);
}

ProcessPipe::~ProcessPipe()
{
    if (pipeFd_[PIPE_IDX::READ] >= 0) {
        fdsan_close_with_tag(pipeFd_[PIPE_IDX::READ], FDSAN_TAG);
        pipeFd_[PIPE_IDX::READ] = -1;
    }
    if (pipeFd_[PIPE_IDX::WRITE] >= 0) {
        fdsan_close_with_tag(pipeFd_[PIPE_IDX::WRITE], FDSAN_TAG);
        pipeFd_[PIPE_IDX::WRITE] = -1;
    }
}

bool ProcessPipe::IsOpened()
{
    return pipeFd_[PIPE_IDX::READ] >= 0 && pipeFd_[PIPE_IDX::WRITE] >= 0;
}

bool ProcessPipe::SendLeakGraph(LeakGraphALC &leakGraph)
{
    size_t size = leakGraph.size();
    if (!SendData(size)) {
        return false;
    }

    for (const auto &[key, node] : leakGraph) {
        if (!SendData(key)) {
            return false;
        }

        if (!SendData(node.capacity)) {
            return false;
        }
        size_t size = node.references.size();
        if (!SendData(size)) {
            return false;
        }
        for (const auto &[offset, uintptr] : node.references) {
            if (!SendData(offset)) {
                return false;
            }
            if (!SendData(uintptr)) {
                return false;
            }
        }
    }
    HILOGI("success to SendLeakGraph, size: %{public}zu", size);
    return true;
}

bool ProcessPipe::ReceiveLeakGraph(LeakGraphSTL &leakGraph)
{
    size_t size = 0;
    if (!ReceiveData(&size)) {
        return false;
    }

    leakGraph.clear();

    for (size_t i = 0; i < size; ++i) {
        uintptr_t key = 0;
        if (!ReceiveData(&key)) {
            return false;
        }
        NodeSTL node;
        if (!ReceiveData(&node.capacity)) {
            return false;
        }
        size_t refCount = 0;
        if (!ReceiveData(&refCount)) {
            return false;
        }
        for (size_t j = 0; j < refCount; j++) {
            int offset = 0;
            if (!ReceiveData(&offset)) {
                return false;
            }
            uintptr_t uintptr = 0;
            if (!ReceiveData(&uintptr)) {
                return false;
            }
            node.references[offset] = uintptr;
        }
        leakGraph[key] = node;
    }
    HILOGI("success to ReceiveLeakGraph, size: %{public}zu", leakGraph.size());
    return true;
}

bool ProcessPipe::SendString(const std::string &str)
{
    size_t size = str.size();
    if (!SendData(size)) {
        return false;
    }
    if (size > 0) {
        ssize_t ret = TEMP_FAILURE_RETRY(write(pipeFd_[PIPE_IDX::WRITE], str.data(), size));
        if (ret < 0) {
            HILOGE("Failed to send string data, errno: %{public}d, err: %{public}s", errno, strerror(errno));
            return false;
        }
        if (static_cast<size_t>(ret) != size) {
            HILOGE("Incomplete string data write");
            return false;
        }
    }
    HILOGI("success to SendString, size: %{public}zu", size);
    return true;
}

bool ProcessPipe::ReceiveString(std::string &str)
{
    size_t size = 0;
    if (!ReceiveData(&size)) {
        return false;
    }
    str.resize(size);
    if (size > 0) {
        ssize_t ret = TEMP_FAILURE_RETRY(read(pipeFd_[PIPE_IDX::READ], str.data(), size));
        if (ret < 0) {
            HILOGE("Failed to receive string data, errno: %{public}d, err: %{public}s", errno, strerror(errno));
            return false;
        }
        if (static_cast<size_t>(ret) != size) {
            HILOGE("Incomplete string data read");
            return false;
        }
    }
    HILOGI("success to ReceiveString, size: %{public}zu", size);
    return true;
}

void ProcessPipe::SetFd(int readFd, int writeFd)
{
    pipeFd_[PIPE_IDX::READ] = readFd;
    pipeFd_[PIPE_IDX::WRITE] = writeFd;
}

int ProcessPipe::GetWriteFd() const
{
    return pipeFd_[PIPE_IDX::WRITE];
}
}  // namespace OHOS
