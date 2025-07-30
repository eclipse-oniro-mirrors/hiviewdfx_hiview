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
#include "faultlog_util.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hisysevent_easy.h"
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
constexpr unsigned MAX_HISYSEVENT_SIZE = 1000;
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
        HILOG_INFO(LOG_CORE, "[gwp_asan] %{public}s", gwpasanlog.c_str());
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
    if (logPath == nullptr || strlen(logPath) == 0 ||
        setAsanOptionTypeList.find(faultType) == setAsanOptionTypeList.end()) {
        currInfo.logPath = "faultlogger";
        HILOG_INFO(LOG_CORE, "Logpath is null, set as default path: faultlogger");
    } else {
        currInfo.logPath = std::string(logPath);
    }
    currInfo.pid = getprocpid();
    currInfo.uid = getuid();
    currInfo.faultType = faultType;
    currInfo.errType = GetErrorTypeFromBuffer(gwpAsanBuffer, faultType);
    currInfo.moduleName = GetNameByPid(currInfo.pid);
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
    bool isSendHisysevent = false;
    WriteCollectedData(currInfo, isSendHisysevent);
    if (isSendHisysevent) {
        SendSanitizerHisysevent(currInfo);
    }
}

void SendSanitizerHisysevent(const GwpAsanCurrInfo& currInfo)
{
    std::stringstream ssPrefix;
    ssPrefix << "FAULT_TYPE:" << currInfo.faultType <<
        ";MODULE:" << currInfo.moduleName <<
        ";REASON:" << currInfo.errType <<
        ";PID:" << currInfo.pid <<
        ";UID:" << currInfo.uid <<
        ";HAPPEN_TIME:" << currInfo.happenTime <<
        ";FINGERPRINT:" << currInfo.hash <<
        ";FIRST_FRAME:" << currInfo.errType <<
        ";SECOND_FRAME:" << currInfo.topStack;
    if (!currInfo.telemetryId.empty()) {
        ssPrefix << ";TELEMETRY_ID:" << currInfo.telemetryId;
    }

    std::string prefixStr = ssPrefix.str();
    size_t maxSummaryLen = 0;
    const std::string summaryPrefix = ";SUMMARY:";
    if (prefixStr.size() + summaryPrefix.size() >= MAX_HISYSEVENT_SIZE) {
        return;
    }
    maxSummaryLen = MAX_HISYSEVENT_SIZE - prefixStr.size() - summaryPrefix.size();
    std::string summary = currInfo.description.substr(0, maxSummaryLen);

    std::stringstream ssParams;
    ssParams << prefixStr << summaryPrefix << summary;
    std::string params = ssParams.str();
    int ret = HiSysEventEasyWrite(
        OHOS::HiviewDFX::HiSysEvent::Domain::RELIABILITY,
        ADDR_SANITIZER_EVENT,
        HiSysEventEasyType::EASY_EVENT_TYPE_FAULT,
        params.c_str()
    );
    if (ret < 0) {
        HILOG_ERROR(LOG_CORE, "Sanitizer send easyhisysevent failed, ret = %{public}d", ret);
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

void WriteCollectedData(const GwpAsanCurrInfo& currInfo, bool& isSendHisysevent)
{
    if (currInfo.logPath != "faultlogger" && WriteToSandbox(currInfo)) {
        HILOG_INFO(LOG_CORE, "Write to sandbox success!");
        return;
    }

    isSendHisysevent = true;
    WriteToFaultLogger(currInfo);
}

void WriteToFaultLogger(const GwpAsanCurrInfo& currInfo)
{
    struct FaultLoggerdRequest request;
    (void)memset_s(&request, sizeof(request), 0, sizeof(request));
    request.type = FaultLoggerType::ADDR_SANITIZER;
    request.pid = currInfo.pid;
    request.time = currInfo.happenTime;
    int fd = RequestFileDescriptorEx(&request);
    if (fd < 0) {
        HILOG_ERROR(LOG_CORE, "failed to RequestFileDescriptorEx, err: %{public}s", strerror(errno));
        return;
    }

    OHOS::HiviewDFX::FileUtil::SaveStringToFd(fd, currInfo.description);
    close(fd);
}

bool WriteToSandbox(const GwpAsanCurrInfo& currInfo)
{
    auto pos = currInfo.logPath.find_last_of('/');
    if (pos == std::string::npos || pos == currInfo.logPath.length() - 1) {
        HILOG_ERROR(LOG_CORE, "Invalid logPath: %{public}s", currInfo.logPath.c_str());
        return false;
    }

    std::string logDir = currInfo.logPath.substr(0, pos);
    std::string fileName = currInfo.logPath.substr(pos + 1);
    std::string realPath;
    if (!OHOS::HiviewDFX::FileUtil::PathToRealPath(logDir, realPath)) {
        HILOG_ERROR(LOG_CORE, "realpath failed on parent dir: %{public}s, err: %{public}s",
            logDir.c_str(), strerror(errno));
        return false;
    }

    const std::regex sandboxRegex("^/data/storage/el[0-9]+(?:/|$)");
    if (!std::regex_search(realPath, sandboxRegex)) {
        HILOG_ERROR(LOG_CORE, "Invalid sandbox path: %{public}s", realPath.c_str());
        return false;
    }

    std::string logFilePath = realPath + "/" + fileName + "." +
        std::to_string(currInfo.pid) + "." +
        std::to_string(currInfo.happenTime);
    int fd = open(logFilePath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_SANITIZER_LOG_MODE);
    if (fd < 0) {
        HILOG_ERROR(LOG_CORE, "Failed to open sandbox file: %{public}s, err: %{public}s",
            logFilePath.c_str(), strerror(errno));
        return false;
    }

    std::ostringstream content;
    content << "Generated by HiviewDFX @OpenHarmony\n"
            << "===============================================================\n"
            << "Device info:"
            << OHOS::HiviewDFX::Parameter::GetString("const.product.name", "Unknown") << "\n"
            << "Build info:"
            << OHOS::HiviewDFX::Parameter::GetString("const.product.software.version", "Unknown") << "\n"
            << "Timestamp:" << currInfo.happenTime << "\n"
            << "Module name:" << currInfo.moduleName << "\n"
            << "Pid:" << std::to_string(currInfo.pid) << "\n"
            << "Uid:" << std::to_string(currInfo.uid) << "\n"
            << "Reason:" << currInfo.errType << "\n"
            << currInfo.description;
    OHOS::HiviewDFX::FileUtil::SaveStringToFd(fd, content.str());
    close(fd);
    return true;
}

bool IsIgnoreStack(const std::string& stack)
{
    const std::unordered_set<std::string> ignoreList = {
        "libclang_rt.hwasan.so",
        "libclang_rt.asan.so",
        "ld-musl-aarch64.so",
        "ld-musl-aarch64-asan.so"
    };
    for (const auto& str : ignoreList) {
        if (stack.find(str, 0) != std::string::npos) {
            return true;
        }
    }
    return false;
}

std::string GetTopStackWithoutCommonLib(const std::string& description)
{
    std::string topstack;
    std::string record = description;
    std::smatch stackCaptured;
    std::string stackRecord =
        std::string("#[\\d+] ") +
        "0[xX][0-9a-fA-F]+" +
        "[\\s\\?(]+" +
        "[^\\+ ]+/([a-zA-Z0-9_.-]+)(\\.z)?(\\.so)?\\+" +
        "0[xX][0-9a-fA-F]+";
    static const std::regex STACK_RE(stackRecord);

    while (std::regex_search(record, stackCaptured, STACK_RE)) {
        std::string current = stackCaptured[1].str();
        if (!IsIgnoreStack(current)) {
            return current;
        }
        topstack = current;
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
