/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "event_server.h"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <securec.h>

#include "base/raw_data_base_def.h"
#include "decoded/decoded_event.h"
#include "device_node.h"
#include "init_socket.h"
#include "hiview_logger.h"
#include "socket_util.h"

#define SOCKET_FILE_DIR "/dev/unix/socket/hisysevent"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventServer");
namespace {
constexpr int BUFFER_SIZE = 384 * 1024;
#ifndef KERNEL_DEVICE_BUFFER
constexpr int EVENT_READ_BUFFER = 2048;
#else
constexpr int EVENT_READ_BUFFER = KERNEL_DEVICE_BUFFER;
#endif

struct Header {
    unsigned short len;
    unsigned short headerSize;
    char msg[0];
};

void InitSocketBuf(int socketId, int optName)
{
    int bufferSizeOld = 0;
    socklen_t sizeOfInt = static_cast<socklen_t>(sizeof(int));
    if (getsockopt(socketId, SOL_SOCKET, optName, static_cast<void *>(&bufferSizeOld), &sizeOfInt) < 0) {
        HIVIEW_LOGE("get socket buffer error=%{public}d, msg=%{public}s", errno, strerror(errno));
    }

    int bufferSizeSet = BUFFER_SIZE;
    if (setsockopt(socketId, SOL_SOCKET, optName, static_cast<void *>(&bufferSizeSet), sizeof(int)) < 0) {
        HIVIEW_LOGE("set socket buffer error=%{public}d, msg=%{public}s", errno, strerror(errno));
    }

    int bufferSizeNew = 0;
    sizeOfInt = static_cast<socklen_t>(sizeof(int));
    if (getsockopt(socketId, SOL_SOCKET, optName, static_cast<void *>(&bufferSizeNew), &sizeOfInt) < 0) {
        HIVIEW_LOGE("get new socket buffer error=%{public}d, msg=%{public}s", errno, strerror(errno));
    }
    HIVIEW_LOGI("reset buffer size old=%{public}d, new=%{public}d", bufferSizeOld, bufferSizeNew);
}

void InitRecvBuffer(int socketId)
{
    InitSocketBuf(socketId, SO_RCVBUF);
}

std::shared_ptr<EventRaw::RawData> ConverRawData(char* source)
{
    if (source == nullptr) {
        HIVIEW_LOGE("invalid source.");
        return nullptr;
    }
    uint32_t sourceLen = *(reinterpret_cast<uint32_t*>(source));
    uint32_t desLen = sourceLen + sizeof(uint8_t);
    uint8_t* des = reinterpret_cast<uint8_t*>(malloc(desLen));
    if (des == nullptr) {
        HIVIEW_LOGE("malloc failed.");
        return nullptr;
    }
    uint32_t sourceHeaderLen = sizeof(int32_t) + sizeof(EventRaw::HiSysEventHeader) - sizeof(uint8_t);
    if (memcpy_s(des, desLen, source, sourceHeaderLen) != EOK) {
        HIVIEW_LOGE("copy failed.");
        free(des);
        return nullptr;
    }
    *(reinterpret_cast<uint8_t*>(des + sourceHeaderLen)) = 0; // init header.log flag
    uint32_t desPos = sourceHeaderLen + sizeof(uint8_t);
    if (memcpy_s(des + desPos, desLen - desPos, source + sourceHeaderLen, sourceLen - sourceHeaderLen) != EOK) {
        HIVIEW_LOGE("copy failed.");
        free(des);
        return nullptr;
    }
    *(reinterpret_cast<int32_t*>(des)) = desLen;
    auto rawData = std::make_shared<EventRaw::RawData>(des, desLen);
    free(des);
    return rawData;
}

void InitMsgh(char* buffer, int bufferLen, std::array<char, CMSG_SPACE(sizeof(struct ucred))>& control,
    struct msghdr& msgh, struct iovec& iov)
{
    iov.iov_base = buffer;
    iov.iov_len = static_cast<uint32_t>(bufferLen);
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1; // 1 is length of io vector

    msgh.msg_control = control.data();
    msgh.msg_controllen = control.size();

    msgh.msg_name = nullptr;
    msgh.msg_namelen = 0;
    msgh.msg_flags = 0;
}

pid_t ReadPidFromMsgh(struct msghdr& msgh)
{
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msgh);
    if (cmsg == nullptr) {
        return UN_INIT_INT_TYPE_VAL;
    }
    struct ucred* uCredRecv = reinterpret_cast<struct ucred*>(CMSG_DATA(cmsg));
    if (uCredRecv == nullptr) {
        return UN_INIT_INT_TYPE_VAL;
    }
    return uCredRecv->pid;
}
}

void SocketDevice::InitSocket(int &socketId)
{
    struct sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    if (strcpy_s(serverAddr.sun_path, sizeof(serverAddr.sun_path), SOCKET_FILE_DIR) != EOK) {
        socketId = -1;
        HIVIEW_LOGE("copy hisysevent dev path failed, error=%{public}d, msg=%{public}s", errno, strerror(errno));
        return;
    }
    serverAddr.sun_path[sizeof(serverAddr.sun_path) - 1] = '\0';
    socketId = TEMP_FAILURE_RETRY(socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
    if (socketId < 0) {
        HIVIEW_LOGE("create hisysevent socket failed, error=%{public}d, msg=%{public}s", errno, strerror(errno));
        return;
    }
    InitRecvBuffer(socketId_);
    unlink(serverAddr.sun_path);
    fdsan_exchange_owner_tag(socketId, 0, logLabelDomain);
    if (TEMP_FAILURE_RETRY(bind(socketId, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr))) < 0) {
        fdsan_close_with_tag(socketId, logLabelDomain);
        socketId = -1;
        HIVIEW_LOGE("bind hisysevent socket failed, error=%{public}d, msg=%{public}s", errno, strerror(errno));
        return;
    }
}

int SocketDevice::Open()
{
    socketId_ = GetControlSocket("hisysevent");
    if (socketId_ < 0) {
        HIVIEW_LOGI("create hisysevent socket");
        InitSocket(socketId_);
    } else {
        InitRecvBuffer(socketId_);
        HIVIEW_LOGI("use hisysevent exist socket");
    }

    if (socketId_ < 0) {
        HIVIEW_LOGE("hisysevent create socket failed");
        return -1;
    }
    return socketId_;
}

int SocketDevice::Close()
{
    if (socketId_ > 0) {
        fdsan_close_with_tag(socketId_, logLabelDomain);
        socketId_ = -1;
    }
    return 0;
}

uint32_t SocketDevice::GetEvents()
{
    return EPOLLIN | EPOLLET;
}

std::string SocketDevice::GetName()
{
    return "SysEventSocket";
}

bool SocketDevice::IsValidMsg(char* msg, int32_t len)
{
    if (len < static_cast<int32_t>(EventRaw::GetValidDataMinimumByteCount())) {
        HIVIEW_LOGD("the data length=%{public}d is invalid", len);
        return false;
    }
    int32_t dataByteCnt = *(reinterpret_cast<int32_t*>(msg));
    if (dataByteCnt != len) {
        HIVIEW_LOGW("the data byte count=%{public}d are not equal to read length %{public}d", dataByteCnt, len);
        return false;
    }
    int32_t pid = *(reinterpret_cast<int32_t*>(msg + sizeof(int32_t) + EventRaw::POS_OF_PID_IN_HEADER));
    if (uCredPid_ > 0 && pid != uCredPid_) {
        HIVIEW_LOGW("failed to verify the consistensy of process id: [%{public}" PRId32
            ", %{public}" PRId32 "]", pid, uCredPid_);
        return false;
    }
    msg[len] = '\0';
    return true;
}

void SocketDevice::SetUCredPid(const pid_t pid)
{
    uCredPid_ = pid;
}

int SocketDevice::ReceiveMsg(std::vector<std::shared_ptr<EventReceiver>> &receivers)
{
    char* buffer = new char[BUFFER_SIZE + 1]();
    std::array<char, CMSG_SPACE(sizeof(struct ucred))> control = {0};
    struct msghdr msgh = {0};
    struct iovec iov = {
        .iov_base = nullptr,
        .iov_len = 0
    };
    InitMsgh(buffer, BUFFER_SIZE, control, msgh, iov);
    while (true) {
        int ret = recvmsg(socketId_, &msgh, 0);
        if (ret <= 0) {
            HIVIEW_LOGD("failed to recv msg from socket");
            break;
        }
        pid_t uCredPid = ReadPidFromMsgh(msgh);
        SetUCredPid(uCredPid);
        if (!IsValidMsg(buffer, ret)) {
            break;
        }
        for (auto receiver = receivers.begin(); receiver != receivers.end(); receiver++) {
            (*receiver)->HandlerEvent(ConverRawData(buffer));
        }
    }
    delete[] buffer;
    return 0;
}

int BBoxDevice::Close()
{
    if (fd_ > 0) {
        fdsan_close_with_tag(fd_, logLabelDomain);
        fd_ = -1;
    }
    return 0;
}

int BBoxDevice::Open()
{
    fd_ = open("/dev/sysevent", O_RDONLY | O_NONBLOCK, 0);
    if (fd_ < 0) {
        fd_ = open("/dev/bbox", O_RDONLY | O_NONBLOCK, 0);
    } else {
        hasBbox_ = true;
    }

    if (fd_ < 0) {
        HIVIEW_LOGE("open bbox failed, error=%{public}d, msg=%{public}s", errno, strerror(errno));
        return -1;
    }
    fdsan_exchange_owner_tag(fd_, 0, logLabelDomain);
    return fd_;
}

uint32_t BBoxDevice::GetEvents()
{
    return EPOLLIN;
}

std::string BBoxDevice::GetName()
{
    return "BBox";
}

bool BBoxDevice::IsValidMsg(char* msg, int32_t len)
{
    if (len < static_cast<int32_t>(EventRaw::GetValidDataMinimumByteCount())) {
        HIVIEW_LOGW("the data length=%{public}d is invalid", len);
        return false;
    }
    int32_t dataByteCnt = *(reinterpret_cast<int32_t*>(msg));
    if ((hasBbox_ && dataByteCnt != len) ||
        (!hasBbox_ && dataByteCnt != (len - sizeof(struct Header) - 1))) { // extra bytes in kernel write
        HIVIEW_LOGW("the data lengths=%{public}d are not equal", len);
        return false;
    }
    msg[EVENT_READ_BUFFER - 1] = '\0';
    return true;
}

int BBoxDevice::ReceiveMsg(std::vector<std::shared_ptr<EventReceiver>> &receivers)
{
    char buffer[EVENT_READ_BUFFER];
    (void)memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    int ret = read(fd_, buffer, EVENT_READ_BUFFER);
    if (!IsValidMsg(buffer, ret)) {
        return -1;
    }
    for (auto receiver = receivers.begin(); receiver != receivers.end(); receiver++) {
        (*receiver)->HandlerEvent(ConverRawData(buffer));
    }
    return 0;
}

void EventServer::AddDev(std::shared_ptr<DeviceNode> dev)
{
    int fd = dev->Open();
    if (fd < 0) {
        HIVIEW_LOGI("open device %{public}s failed", dev->GetName().c_str());
        return;
    }
    devs_[fd] = dev;
}

int EventServer::OpenDevs()
{
    AddDev(std::make_shared<SocketDevice>());
    AddDev(std::make_shared<BBoxDevice>());
    if (devs_.empty()) {
        HIVIEW_LOGE("can not open any device");
        return -1;
    }
    HIVIEW_LOGI("has open %{public}zu devices", devs_.size());
    return 0;
}

int EventServer::AddToMonitor(int pollFd, struct epoll_event pollEvents[])
{
    int index = 0;
    auto it = devs_.begin();
    while (it != devs_.end()) {
        HIVIEW_LOGI("add to poll device %{public}s, fd=%{public}d", it->second->GetName().c_str(), it->first);
        pollEvents[index].data.fd = it->first;
        pollEvents[index].events = it->second->GetEvents();
        int ret = epoll_ctl(pollFd, EPOLL_CTL_ADD, it->first, &pollEvents[index]);
        if (ret < 0) {
            HIVIEW_LOGE("add to poll fail device %{public}s error=%{public}d, msg=%{public}s",
                it->second->GetName().c_str(), errno, strerror(errno));
            it->second->Close();
            it = devs_.erase(it);
        } else {
            it++;
        }
        index++;
    }

    if (devs_.empty()) {
        HIVIEW_LOGE("can not monitor any device");
        return -1;
    }
    HIVIEW_LOGI("monitor devices %{public}zu", devs_.size());
    return 0;
}

void EventServer::Start()
{
    HIVIEW_LOGI("start event server");
    if (OpenDevs() < 0) {
        return;
    }

    int pollFd = epoll_create1(EPOLL_CLOEXEC);
    if (pollFd < 0) {
        HIVIEW_LOGE("create poll failed, error=%{public}d, msg=%{public}s", errno, strerror(errno));
        return;
    }

    struct epoll_event pollEvents[devs_.size()];
    if (AddToMonitor(pollFd, pollEvents) < 0) {
        return;
    }

    if (setpriority(PRIO_PROCESS, 0, -20) != 0) { // nice:-20
        HIVIEW_LOGW("failed to setpriority -20");
    }

    HIVIEW_LOGI("go into event loop");
    isStart_ = true;
    while (isStart_) {
        struct epoll_event chkPollEvents[devs_.size()];
        int eventCount = epoll_wait(pollFd, chkPollEvents, devs_.size(), -1); // -1: Wait indefinitely
        if (eventCount <= 0) {
            HIVIEW_LOGD("read event timeout");
            continue;
        }
        for (int ii = 0; ii < eventCount; ii++) {
            auto it = devs_.find(chkPollEvents[ii].data.fd);
            it->second->ReceiveMsg(receivers_);
        }
    }
    CloseDevs();
}

void EventServer::CloseDevs()
{
    for (auto devItem : devs_) {
        devItem.second->Close();
    }
}

void EventServer::Stop()
{
    isStart_ = false;
}

void EventServer::AddReceiver(std::shared_ptr<EventReceiver> receiver)
{
    receivers_.emplace_back(receiver);
}
} // namespace HiviewDFX
} // namespace OHOS
