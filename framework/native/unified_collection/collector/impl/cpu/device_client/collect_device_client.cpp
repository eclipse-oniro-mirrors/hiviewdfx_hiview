/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "collect_device_client.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "hiview_logger.h"
#include "securec.h"

#define UNIFIED_COLLECTION_DEVICE  "/dev/ucollection"
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("UDeviceCli");
namespace {
    constexpr int PID_ALL = 0;
    constexpr unsigned int PROCESS_ONLY_ONE_COUNT = 1;
    constexpr unsigned int ADD_COUNT = 30;
}

CollectDeviceClient::CollectDeviceClient(): fd_(-1)
{}

CollectDeviceClient::~CollectDeviceClient()
{
    if (fd_ > 0) {
        close(fd_);
    }
}

int CollectDeviceClient::GetDeviceFd(bool readOnly)
{
    int f = 0;
    if (readOnly) {
        f = open(UNIFIED_COLLECTION_DEVICE, O_RDONLY);
    } else {
        f = open(UNIFIED_COLLECTION_DEVICE, O_WRONLY);
    }

    if (f < 0) {
        HIVIEW_LOGE("Cannot open file=%{public}s, readOnly=%{public}d", UNIFIED_COLLECTION_DEVICE, readOnly);
        return -1;
    }
    HIVIEW_LOGD("open ucollection device readOnly=%{public}d successful", readOnly);
    return f;
}

int CollectDeviceClient::Open()
{
    fd_ = GetDeviceFd(true);
    return (fd_ > 0) ? 0 : -1;
}

unsigned int CollectDeviceClient::GetProcessCount()
{
    HIVIEW_LOGD("send IOCTRL_COLLECT_PROC_COUNT");
    unsigned int processCount = 0;
    int ret = ioctl(fd_, IOCTRL_COLLECT_PROC_COUNT, &processCount);
    if (ret < 0) {
        HIVIEW_LOGE("ioctl IOCTRL_COLLECT_PROC_COUNT, ret=%{public}d", ret);
        return 0;
    }
    return processCount;
}

std::shared_ptr<ProcessCpuData> CollectDeviceClient::FetchProcessCpuData()
{
    unsigned int processCount = GetProcessCount();
    HIVIEW_LOGD("send IOCTRL_COLLECT_ALL_PROC_CPU");
    std::shared_ptr<ProcessCpuData> data = std::make_shared<ProcessCpuData>(IOCTRL_COLLECT_ALL_PROC_CPU, PID_ALL,
        processCount + ADD_COUNT);
    int ret = ioctl(fd_, IOCTRL_COLLECT_ALL_PROC_CPU, data->entry_);
    if (ret < 0) {
        HIVIEW_LOGE("ioctl IOCTRL_COLLECT_ALL_PROC_CPU ret=%{public}d", ret);
        return data;
    }
    return data;
}

std::shared_ptr<ProcessCpuData> CollectDeviceClient::FetchProcessCpuData(int pid)
{
    HIVIEW_LOGD("send IOCTRL_COLLECT_THE_PROC_CPU");
    std::shared_ptr<ProcessCpuData> data = std::make_shared<ProcessCpuData>(IOCTRL_COLLECT_THE_PROC_CPU, pid,
        PROCESS_ONLY_ONE_COUNT);
    int ret = ioctl(fd_, IOCTRL_COLLECT_THE_PROC_CPU, data->entry_);
    if (ret < 0) {
        HIVIEW_LOGE("ioctl IOCTRL_COLLECT_THE_PROC_CPU ret=%{public}d", ret);
        return data;
    }
    return data;
}

unsigned int CollectDeviceClient::GetThreadCount(int pid)
{
    HIVIEW_LOGD("send IOCTRL_COLLECT_THREAD_COUNT");
    struct ucollection_process_thread_count threadCount {pid, 0};
    int ret = ioctl(fd_, IOCTRL_COLLECT_THREAD_COUNT, &threadCount);
    if (ret < 0) {
        HIVIEW_LOGE("ioctl IOCTRL_COLLECT_PROC_THREAD_COUNT ret=%{public}d", ret);
        return 0;
    }
    return threadCount.thread_count;
}

unsigned int CollectDeviceClient::GetSelfThreadCount(int pid)
{
    HIVIEW_LOGD("send IOCTRL_COLLECT_APP_THREAD_COUNT");
    struct ucollection_process_thread_count threadCount {pid, 0};
    int ret = ioctl(fd_, IOCTRL_COLLECT_APP_THREAD_COUNT, &threadCount);
    if (ret < 0) {
        HIVIEW_LOGE("ioctl IOCTRL_COLLECT_APP_THREAD_COUNT ret=%{public}d", ret);
        return 0;
    }
    return threadCount.thread_count;
}

std::shared_ptr<ThreadCpuData> CollectDeviceClient::FetchThreadCpuData(int pid)
{
    HIVIEW_LOGD("send IOCTRL_COLLECT_THE_THREAD");
    unsigned int threadCount = GetThreadCount(pid);
    if (threadCount <= 0) {
        HIVIEW_LOGE("ioctl GetThreadCount error");
        return nullptr;
    }
    auto data = std::make_shared<ThreadCpuData>(IOCTRL_COLLECT_THE_THREAD, pid, threadCount);
    int ret = ioctl(fd_, IOCTRL_COLLECT_THE_THREAD, data->entry_);
    if (ret < 0) {
        HIVIEW_LOGE("ioctl FetchThreadData, ret=%{public}d", ret);
        return data;
    }
    return data;
}

std::shared_ptr<ThreadCpuData> CollectDeviceClient::FetchSelfThreadCpuData(int pid)
{
    HIVIEW_LOGD("send IOCTRL_COLLECT_APP_THREAD, cmd=%{public}zu", IOCTRL_COLLECT_APP_THREAD);
    unsigned int threadCount = GetSelfThreadCount(pid);
    if (threadCount <= 0) {
        HIVIEW_LOGE("ioctl GetSelfThreadCount error");
        return nullptr;
    }
    auto data = std::make_shared<ThreadCpuData>(IOCTRL_COLLECT_APP_THREAD, pid, threadCount);
    int ret = ioctl(fd_, IOCTRL_COLLECT_APP_THREAD, data->entry_);
    if (ret < 0) {
        HIVIEW_LOGE("ioctl FetchSelfThreadCpuData, ret=%{public}d", ret);
        return data;
    }
    return data;
}
} // HiviewDFX
} // OHOS
