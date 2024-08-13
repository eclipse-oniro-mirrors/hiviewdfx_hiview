/*
 * Copyright (c) 2023 Huawei Device Co., Ltd. All rights reserved.
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
#include "gwpasan_collector.h"

#include <cstdint>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <mutex>
#include <securec.h>
#include <sys/time.h>
#include <time_util.h>
#include <unistd.h>
#include <vector>

#include "bundle_mgr_client.h"
#include "event_publish.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "hisysevent.h"
#include "json/json.h"
#include "hilog/log.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D12

#undef LOG_TAG
#define LOG_TAG "Sanitizer"

static std::stringstream g_asanlog;

void WriteGwpAsanLog(char* buf, size_t sz)
{
    if (buf == nullptr || sz == 0) {
        return;
    }

    static std::mutex sMutex;
    std::lock_guard<std::mutex> lock(sMutex);
    // append to buffer
    for (size_t i = 0; i < sz; i++) {
        g_asanlog << buf[i];
    }

    char *gwpOutput = strstr(buf, "End GWP-ASan report");
    char *tsanOutput = strstr(buf, "End Tsan report");
    char *cfiOutput = strstr(buf, "End CFI report");
    char *hwasanOutput = strstr(buf, "End Hwasan report");
    if (gwpOutput) {
        std::string gwpasanlog = g_asanlog.str();
        // parse log
        std::string errType = "GWP-ASAN";
        ReadGwpAsanRecord(gwpasanlog, errType);
        // clear buffer
        g_asanlog.str("");
    } else if (tsanOutput) {
        std::string tsanlog = g_asanlog.str();
        // parse log
        std::string errType = "TSAN";
        ReadGwpAsanRecord(tsanlog, errType);
        // clear buffer
        g_asanlog.str("");
    } else if (cfiOutput) {
        std::string ubsanlog = g_asanlog.str();
        // parse log
        std::string errType = "UBSAN";
        ReadGwpAsanRecord(ubsanlog, errType);
        // clear buffer
        g_asanlog.str("");
    } else if (hwasanOutput) {
        std::string hwasanlog = g_asanlog.str();
        // parse log
        std::string errType = "HWASAN";
        ReadGwpAsanRecord(hwasanlog, errType);
        // clear buffer
        g_asanlog.str("");
    }
}

void ReadGwpAsanRecord(const std::string& gwpAsanBuffer, const std::string& errType)
{
    GwpAsanCurrInfo currInfo;
    currInfo.description = gwpAsanBuffer;
    currInfo.pid = getpid();
    currInfo.uid = getuid();
    currInfo.errType = errType;
    currInfo.procName = GetNameByPid(currInfo.pid);
    currInfo.appVersion = "";
    time_t timeNow = time(nullptr);
    uint64_t timeTmp = timeNow;
    std::string timeStr = OHOS::HiviewDFX::GetFormatedTime(timeTmp);
    currInfo.happenTime = std::stoll(timeStr);

    // Do upload when data ready
    WriteCollectedData(currInfo);
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RELIABILITY, "ADDR_SANITIZER",
                    OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
                    "MODULE", currInfo.procName,
                    "VERSION", currInfo.appVersion,
                    "REASON", currInfo.errType,
                    "PID", currInfo.pid,
                    "UID", currInfo.uid,
                    "SUMMARY", currInfo.description,
                    "HAPPEN_TIME", currInfo.happenTime);
}

bool WriteNewFile(const int32_t fd, const GwpAsanCurrInfo &currInfo)
{
    if (fd < 0) {
        return false;
    }

    OHOS::HiviewDFX::FileUtil::SaveStringToFd(fd, std::string("Generated by HiviewDFX @OpenHarmony") + "\n" +
        std::string("=================================================================\n") +
        "TIMESTAMP:" + std::to_string(currInfo.happenTime) + "\n" +
        "Device Info:" + OHOS::HiviewDFX::Parameter::GetString("const.product.name", "Unknown") + "\n" +
        "Build Info:" + OHOS::HiviewDFX::Parameter::GetString("const.product.software.version", "Unknown") + "\n" +
        "Pid:" + std::to_string(currInfo.pid) + "\n" +
        "Uid:" + std::to_string(currInfo.uid) + "\n" +
        "Process name:" + currInfo.procName + "\n" +
        "Fault thread info:\n" + currInfo.description);

    close(fd);
    return true;
}

std::string CalcCollectedLogName(const GwpAsanCurrInfo &currInfo)
{
    std::string filePath = "/dev/asanlog/";
    std::string prefix = "";
    if (currInfo.errType.compare("GWP-ASAN") == 0) {
        prefix = "gwpasan";
    } else if (currInfo.errType.compare("TSAN") == 0) {
        prefix = "tsan";
    } else if (currInfo.errType.compare("UBSAN") == 0) {
        prefix = "ubsan";
    } else {
        prefix = "unknown-crash";
    }
    std::string name = currInfo.procName;
    if (name.find("/") != std::string::npos) {
        name = currInfo.procName.substr(currInfo.procName.find_last_of("/") + 1);
    }

    std::string fileName = "";
    fileName.append(prefix);
    fileName.append("-");
    fileName.append(name);
    fileName.append("-");
    fileName.append(std::to_string(currInfo.pid));
    fileName.append("-");
    fileName.append(std::to_string(currInfo.happenTime));

    std::string fullName = filePath + fileName;
    return fullName;
}

void WriteCollectedData(const GwpAsanCurrInfo &currInfo)
{
    std::string fullName = CalcCollectedLogName(currInfo);
    if (fullName.size() == 0) {
        return;
    }
    int32_t fd = CreateLogFile(fullName);
    if (fd < 0) {
        return;
    }
    WriteNewFile(fd, currInfo);
    Json::Value eventParams;
    auto timeNow = OHOS::HiviewDFX::TimeUtil::GetMilliseconds();
    eventParams["time"] = timeNow;
    eventParams["type"] = currInfo.errType;
    eventParams["bundle_version"] = currInfo.appVersion;
    eventParams["bundle_name"] = currInfo.procName;
    Json::Value externalLog;
    externalLog.append(fullName);
    eventParams["external_log"] = externalLog;
    eventParams["pid"] = currInfo.pid;
    eventParams["uid"] = currInfo.uid;

    std::string paramsStr = Json::FastWriter().write(eventParams);
    HILOG_INFO(LOG_CORE, "Gwpasan ReportAppEvent: uid:%{public}d, json:%{public}s.",
        currInfo.uid, paramsStr.c_str());
    OHOS::HiviewDFX::EventPublish::GetInstance().PushEvent(currInfo.uid, "ADDRESS_SANITIZER",
        OHOS::HiviewDFX::HiSysEvent::EventType::FAULT, paramsStr);
}

std::mutex g_mutex;
int32_t CreateLogFile(const std::string& name)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    int32_t fd = -1;

    if (access(name.c_str(), F_OK) == 0) {
        fd = TEMP_FAILURE_RETRY(open(name.c_str(), O_WRONLY | O_APPEND));
    } else {
        fd = TEMP_FAILURE_RETRY(open(name.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR));
    }
    if (fd < 0) {
        HILOG_ERROR(LOG_CORE, "Fail to create %{public}s,  err: %{public}s.",
            name.c_str(), strerror(errno));
    }
    return fd;
}

std::string GetNameByPid(int32_t pid)
{
    char path[BUF_SIZE] = { 0 };
    int err = snprintf_s(path, sizeof(path), sizeof(path) - 1, "/proc/%d/cmdline", pid);
    if (err <= 0) {
        return "";
    }
    char cmdline[BUF_SIZE] = { 0 };
    size_t i = 0;
    FILE *fp = fopen(path, "r");
    if (fp == nullptr) {
        return "";
    }
    while (i < (BUF_SIZE - 1)) {
        char c = static_cast<char>(fgetc(fp));
        // 0. don't need args of cmdline
        // 1. ignore unvisible character
        if (!isgraph(c)) {
            break;
        }
        cmdline[i] = c;
        i++;
    }
    (void)fclose(fp);
    return cmdline;
}
