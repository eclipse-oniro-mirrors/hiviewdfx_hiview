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

#ifndef LIBORPHANMEM_PROCESS_PIPE
#define LIBORPHANMEM_PROCESS_PIPE

#include <cerrno>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

#include "map_allocator.h"
#include "leak_info.h"
#include "log.h"

namespace OHOS {
enum PIPE_IDX {
    READ = 0,
    WRITE,
    CNT,
};

class ProcessPipe {
public:
    ProcessPipe();
    ~ProcessPipe();
    bool IsOpened();

    template <typename T>
    bool SendData(const T &data)
    {
        ssize_t ret = TEMP_FAILURE_RETRY(write(pipeFd_[PIPE_IDX::WRITE], &data, sizeof(T)));
        if (ret < 0) {
            HILOGE("Failed to send data, errno: %{public}d, err: %{public}s", errno, strerror(errno));
            return false;
        }
        if (static_cast<size_t>(ret) != sizeof(T)) {
            HILOGE("Incomplete data write");
            return false;
        }
        return true;
    }

    template <typename T>
    bool ReceiveData(T *data)
    {
        ssize_t ret = TEMP_FAILURE_RETRY(read(pipeFd_[PIPE_IDX::READ], reinterpret_cast<void *>(data), sizeof(T)));
        if (ret < 0) {
            HILOGE("Failed to receive data, errno: %{public}d, err: %{public}s", errno, strerror(errno));
            return false;
        }
        if (static_cast<size_t>(ret) != sizeof(T)) {
            HILOGE("Incomplete data read");
            return false;
        }
        return true;
    }

    bool SendLeakGraph(LeakGraphALC &leakGraph);
    bool ReceiveLeakGraph(LeakGraphSTL &leakGraph);
    bool SendString(const std::string &str);
    bool ReceiveString(std::string &str);
    void SetFd(int readFd, int writeFd);
    int GetWriteFd() const;

private:
    int pipeFd_[PIPE_IDX::CNT] = {-1, -1};
};

}  // namespace OHOS

#endif  // LIBORPHANMEM_PROCESS_PIPE