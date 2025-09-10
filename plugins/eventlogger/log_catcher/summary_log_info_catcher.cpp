/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "summary_log_info_catcher.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "file_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    #define SUMMARY_LOG_BASE 'S'
    #define GET_SUMMARY_LOG _IOWR(SUMMARY_LOG_BASE, 0x01, int32_t)

    static constexpr int LINE_BASE_SIZE = 120;
    static constexpr int SUMMARY_LOG_INFO_MAX_SIZE = 10;
    static constexpr int SUMMARY_LOG_MAGIC = 0xE5AC01;
    struct summary_log_line_info {
        int64_t sec_timestamp;
        char buffer[LINE_BASE_SIZE];
    };
    struct ringbuff_log_info {
        unsigned int magic;
        int64_t needed_sec_timestamp;
        struct summary_log_line_info line[SUMMARY_LOG_INFO_MAX_SIZE];
        unsigned int magicSize;
    };
}

DEFINE_LOG_LABEL(0xD002D01, "EventLogger-SummaryLogInfoCatcher");
SummaryLogInfoCatcher::SummaryLogInfoCatcher() : EventLogCatcher()
{
    name_ = "SummaryLogInfoCatcher";
}

bool SummaryLogInfoCatcher::Initialize(const std::string& strParam1, int intParam1, int intParam2)
{
    // this catcher do not need parameters, just return true
    description_ = "SummaryLogInfoCatcher --\n";
    return true;
}

void SummaryLogInfoCatcher::SetFaultTime(int64_t faultTime)
{
    faultTime_ = faultTime;
}

int SummaryLogInfoCatcher::Catch(int fd, int jsonFd)
{
    int originSize = GetFdSize(fd);
    
    int sysLoadFd = open("/dev/sysload", O_RDWR);
    if (sysLoadFd < 0) {
        HIVIEW_LOGE("open /dev/sysload failed!");
        return 0;
    }
    ringbuff_log_info info = { 0 };
    info.magic = SUMMARY_LOG_MAGIC;
    info.needed_sec_timestamp = faultTime_;
    info.magicSize = sizeof(struct ringbuff_log_info);
    int res = ioctl(sysLoadFd, GET_SUMMARY_LOG, &info);
    if (res < 0) {
        HIVIEW_LOGE("ioctl failed, errno:%{public}d", errno);
        close(sysLoadFd);
        return 0;
    }
    close(sysLoadFd);
    HIVIEW_LOGI("ioctl res:%{public}d", res);

    std::string summaryLogInfoStr;
    std::string lineStr;
    for (const summary_log_line_info &lineInfo: info.line) {
        if (lineInfo.sec_timestamp == 0) {
            continue;
        }
        lineStr = "timestamp=" + std::to_string(lineInfo.sec_timestamp) +
            ", data=" + CharArrayStr(lineInfo.buffer, static_cast<size_t>(LINE_BASE_SIZE)) + "\n";
        summaryLogInfoStr += lineStr;
    }
    FileUtil::SaveStringToFd(fd, summaryLogInfoStr);

    logSize_ = GetFdSize(fd) - originSize;
    if (logSize_ <= 0) {
        FileUtil::SaveStringToFd(fd, "summary log info is empty or saving it failed!\n");
    }
    return logSize_;
}

std::string SummaryLogInfoCatcher::CharArrayStr(const char* chars, size_t maxSize)
{
    if (!chars) {
        return "";
    }

    size_t length = 0;
    while (length < maxSize && chars[length] != '\0') {
        length++;
    }
    return std::string(chars, length);
}
} // namespace HiviewDFX
} // namespace OHOS