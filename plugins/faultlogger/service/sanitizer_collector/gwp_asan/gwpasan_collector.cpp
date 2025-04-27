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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <mutex>
#include <securec.h>
#include <sys/time.h>
#include <time_util.h>
#include <unistd.h>
#include <parameters.h>

#include "bundle_mgr_client.h"
#include "event_publish.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "hisysevent.h"
#include "json/json.h"
#include "hilog/log.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "faultlog_formatter.h"
#include "faultloggerd_client.h"
#include "tbox.h"
#include "common_defines.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D12

#undef LOG_TAG
#define LOG_TAG "Sanitizer"

namespace {
constexpr unsigned SUMMARY_LOG_SIZE = 350 * 1024;
constexpr unsigned BUF_SIZE = 128;
constexpr unsigned HWASAN_ERRTYPE_FIELD = 2;
constexpr unsigned ASAN_ERRTYPE_FIELD = 2;
constexpr mode_t DEFAULT_SANITIZER_LOG_MODE = 0644;
constexpr char ADDR_SANITIZER_EVENT[] = "ADDR_SANITIZER";
static std::stringstream g_asanlog;
}

void WriteSanitizerLog(char* buf, size_t sz, char* path)
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
    char *ubsanOutput = strstr(buf, "End Ubsan report");
    char *hwasanOutput = strstr(buf, "End Hwasan report");
    char *asanOutput = strstr(buf, "End Asan report");
    if (gwpOutput) {
        std::string gwpasanlog = g_asanlog.str();
        ReadGwpAsanRecord(gwpasanlog, "GWP-ASAN", path);
        // clear buffer
        g_asanlog.str("");
    } else if (tsanOutput) {
        std::string tsanlog = g_asanlog.str();
        ReadGwpAsanRecord(tsanlog, "TSAN", path);
        // clear buffer
        g_asanlog.str("");
    } else if (cfiOutput || ubsanOutput) {
        std::string ubsanlog = g_asanlog.str();
        ReadGwpAsanRecord(ubsanlog, "UBSAN", path);
        // clear buffer
        g_asanlog.str("");
    } else if (hwasanOutput) {
        std::string hwasanlog = g_asanlog.str();
        ReadGwpAsanRecord(hwasanlog, "HWASAN", path);
        // clear buffer
        g_asanlog.str("");
    } else if (asanOutput) {
        std::string asanlog = g_asanlog.str();
        ReadGwpAsanRecord(asanlog, "ASAN", path);
        // clear buffer
        g_asanlog.str("");
    }
}

void ReadGwpAsanRecord(const std::string& gwpAsanBuffer, const std::string& faultType, char* logPath)
{
    const std::unordered_set<std::string> setAsanOptionTypeList = {"ASAN", "HWASAN"};
    GwpAsanCurrInfo currInfo;
    currInfo.description = gwpAsanBuffer;
    currInfo.summary = ((gwpAsanBuffer.size() > SUMMARY_LOG_SIZE) ? (gwpAsanBuffer.substr(0, SUMMARY_LOG_SIZE) +
                                                                     "\nEnd Summary report") : gwpAsanBuffer);
    if (logPath == nullptr || logPath[0] == '\0'||
        setAsanOptionTypeList.find(faultType) == setAsanOptionTypeList.end()) {
        currInfo.logPath = "faultlogger";
        HILOG_INFO(LOG_CORE, "Logpath is null, set as default path: faultlogger");
    } else {
        currInfo.logPath = std::string(logPath);
    }
    currInfo.pid = getpid();
    currInfo.uid = getuid();
    currInfo.faultType = faultType;
    currInfo.errType = GetErrorTypeFromBuffer(gwpAsanBuffer, faultType);
    currInfo.moduleName = GetNameByPid(currInfo.pid);
    currInfo.appVersion = "";
    time_t timeNow = time(nullptr);
    uint64_t timeTmp = timeNow;
    constexpr int decimalBase = 10;
    std::string timeStr = OHOS::HiviewDFX::GetFormatedTime(timeTmp);
    currInfo.happenTime = static_cast<uint64_t>(strtoull(timeStr.c_str(), nullptr, decimalBase));
    currInfo.topStack = GetTopStackWithoutCommonLib(currInfo.description);
    currInfo.hash = OHOS::HiviewDFX::Tbox::CalcFingerPrint(
        currInfo.topStack + currInfo.errType + currInfo.moduleName, 0, OHOS::HiviewDFX::FingerPrintMode::FP_BUFFER);
    currInfo.telemetryId = OHOS::system::GetParameter("persist.hiviewdfx.priv.diagnosis.time.taskId", "");
    HILOG_INFO(LOG_CORE, "ReportSanitizerAppEvent: uid: %{public}d, logPath: %{public}s, telemetryId: %{public}s",
        currInfo.uid, currInfo.logPath.c_str(), currInfo.telemetryId.c_str());
    // Do upload when data ready
    WriteCollectedData(currInfo);
    if (currInfo.logPath == "faultlogger") {
        SendSanitizerHisysevent(currInfo);
    }
}

void SendSanitizerHisysevent(const GwpAsanCurrInfo& currInfo)
{
    std::vector<HiSysEventParam> params = {
        { .name = "MODULE", .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char*>(currInfo.moduleName.c_str()) }, .arraySize = 0 },
        { .name = "VERSION", .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char*>(currInfo.appVersion.c_str()) }, .arraySize = 0 },
        { .name = "FAULT_TYPE", .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char*>(currInfo.faultType.c_str()) }, .arraySize = 0 },
        { .name = "REASON", .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char*>(currInfo.errType.c_str()) }, .arraySize = 0 },
        { .name = "PID", .t = HISYSEVENT_INT32,
            .v = { .i32 = currInfo.pid }, .arraySize = 0 },
        { .name = "UID", .t = HISYSEVENT_INT32,
            .v = { .i32 = currInfo.uid }, .arraySize = 0 },
        { .name = "SUMMARY", .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char*>(currInfo.summary.c_str()) }, .arraySize = 0 },
        { .name = "HAPPEN_TIME", .t = HISYSEVENT_UINT64,
            .v = { .ui64 = currInfo.happenTime }, .arraySize = 0 },
        { .name = "FINGERPRINT", .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char*>(currInfo.hash.c_str()) }, .arraySize = 0 },
        { .name = "FIRST_FRAME", .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char*>(currInfo.errType.c_str()) }, .arraySize = 0 },
        { .name = "SECOND_FRAME", .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char*>(currInfo.topStack.c_str()) }, .arraySize = 0 }
    };
    if (!currInfo.telemetryId.empty()) {
        params.push_back({
            .name = "TELEMETRY_ID", .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char*>(currInfo.telemetryId.c_str()) }, .arraySize = 0 }
        );
    }
    int ret = OH_HiSysEvent_Write(
        OHOS::HiviewDFX::HiSysEvent::Domain::RELIABILITY,
        ADDR_SANITIZER_EVENT,
        HISYSEVENT_FAULT,
        params.data(),
        params.size()
    );
    if (ret < 0) {
        HILOG_ERROR(LOG_CORE, "Sanitizer send hisysevent failed, ret = %{public}d", ret);
    }
}

std::string GetErrorTypeFromBuffer(const std::string& buffer, const std::string& faultType)
{
    const std::regex asanRegex("SUMMARY: (AddressSanitizer|LeakSanitizer): (\\S+)");
    const std::regex hwasanRegex("(Potential Cause|Cause): ([\\w -]+)");

    std::smatch match;
    if (std::regex_search(buffer, match, asanRegex)) {
        return match[ASAN_ERRTYPE_FIELD].str();
    }
    if (std::regex_search(buffer, match, hwasanRegex)) {
        return match[HWASAN_ERRTYPE_FIELD].str();
    }
    HILOG_INFO(LOG_CORE, "%{public}s Regex not match, set default type.", faultType.c_str());
    return faultType;
}

void WriteCollectedData(const GwpAsanCurrInfo& currInfo)
{
    if (currInfo.logPath.size() == 0) {
        return;
    }
    int32_t fd = GetSanitizerFd(currInfo);
    if (fd < 0) {
        HILOG_ERROR(LOG_CORE, "Fail to create %{public}s, err: %{public}s.",
            currInfo.logPath.c_str(), strerror(errno));
        return;
    }
    WriteNewFile(fd, currInfo);
    close(fd);
}

int32_t GetSanitizerFd(const GwpAsanCurrInfo& currInfo)
{
    int32_t fd = -1;
    if (currInfo.logPath == "faultlogger") {
        struct FaultLoggerdRequest request;
        (void)memset_s(&request, sizeof(request), 0, sizeof(request));
        request.type = FaultLoggerType::ADDR_SANITIZER;
        request.pid = currInfo.pid;
        request.time = currInfo.happenTime;
        fd = RequestFileDescriptorEx(&request);
    } else {
        const std::string sandboxBase = "/data/storage/el2";
        if (currInfo.logPath.compare(0, sandboxBase.length(), sandboxBase) != 0) {
            HILOG_ERROR(LOG_CORE, "Invalid log path %{public}s, must in sandbox or set as faultlogger.",
                currInfo.logPath.c_str());
            return -1;
        }
        std::string logPath = currInfo.logPath + "." +
            std::to_string(currInfo.pid) + "." +
            std::to_string(currInfo.happenTime);
        fd = open(logPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_SANITIZER_LOG_MODE);
    }
    return fd;
}

bool WriteNewFile(const int32_t fd, const GwpAsanCurrInfo& currInfo)
{
    std::ostringstream content;
    if (currInfo.logPath == "faultlogger") {
        content << currInfo.description;
    } else {
        content << "Generated by HiviewDFX @OpenHarmony\n"
                << "===============================================================\n"
                << "Device info:" <<
                OHOS::HiviewDFX::Parameter::GetString("const.product.name", "Unknown") << "\n"
                << "Build info:" <<
                OHOS::HiviewDFX::Parameter::GetString("const.product.software.version", "Unknown") << "\n"
                << "Timestamp:" << currInfo.happenTime << "\n"
                << "Module name:" << currInfo.moduleName << "\n"
                << "Pid:" << std::to_string(currInfo.pid) << "\n"
                << "Uid:" << std::to_string(currInfo.uid) << "\n"
                << "Reason:" << currInfo.errType << "\n"
                << currInfo.description;
    }
    OHOS::HiviewDFX::FileUtil::SaveStringToFd(fd, content.str());
    return true;
}

std::string GetTopStackWithoutCommonLib(const std::string& description)
{
    std::string topstack;
    std::string record = description;
    std::smatch stackCaptured;
    std::string stackRecord =
    "  #[\\d+] " + std::string("0[xX][0-9a-fA-F]+") +
    "[\\s\\?(]+" +
    "[^\\+ ]+/([a-zA-Z0-9_.]+)(\\.z)?(\\.so)?\\+" + std::string("0[xX][0-9a-fA-F]+");
    static const std::regex STACK_RE(stackRecord);

    while (std::regex_search(record, stackCaptured, STACK_RE)) {
        if (topstack.size() == 0) {
            topstack = stackCaptured[1].str();
        }
        record = stackCaptured.suffix().str();
    }

    return topstack;
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