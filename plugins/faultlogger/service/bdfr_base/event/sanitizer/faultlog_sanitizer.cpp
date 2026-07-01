/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "faultlog_sanitizer.h"

#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "constants.h"
#include "dfx_ark.h"
#include "event_publish.h"
#include "faultlog_util.h"
#include "ffrt.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "json/json.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

namespace {
constexpr uintptr_t OFFSET_HEAD = 0x4;                          // head offset
constexpr mode_t DEFAULT_LOG_FILE_MODE = 0664;                  // parse arkts info temp file mode
constexpr uint64_t ARKTS_STACK_MAX_FILE_SIZE = 1024 * 1024 * 5; // 5MB
constexpr int WAIT_CHILD_PROCESS_INTERVAL = 5 * 1000;           // 5ms
constexpr int MAX_RETRY_COUNT = 4000;
constexpr int MIN_HEX_STR_SIZE = 2;

const char *const APP_SANDBOX_PREFIX = "/data/storage/el1/bundle/";
const char *const CONVERT_APP_SANBOPX_PREFIX = "/data/app/el1/bundle/public/";
}

bool FaultLogSanitizer::ShouldParseSandBoxPath(const std::string& line)
{
    return line.find(".hap+") != std::string::npos ||
           line.find(".hsp+") != std::string::npos ||
           line.find(".hqf+") != std::string::npos ||
           line.find(".abc+") != std::string::npos ||
           line.find("[anon:ArkTS Code:/") != std::string::npos;
}

bool FaultLogSanitizer::ConvertPathFromOriginLine(const std::string& line, std::string& pathPrefix,
                                                  const std::string& bundleName)
{
    // #xx pc  (pathPrefix+relativePc)
    size_t leftParentPos = line.find('(');
    if (leftParentPos == std::string::npos) {
        return false;
    }
    size_t plusPos = line.find('+', leftParentPos);
    if (plusPos == std::string::npos) {
        return false;
    }
    if (plusPos <= leftParentPos + 1) {
        return false;
    }

    std::string fullPath = line.substr(leftParentPos + 1, plusPos - leftParentPos - 1);
    size_t lastSlashPos = fullPath.rfind('/');
    if (lastSlashPos == std::string::npos) {
        return false;
    }

    pathPrefix = FileUtil::ExtractFilePath(fullPath);
    std::string fileName = FileUtil::ExtractFileName(fullPath);

    // pathPrefix mean full sandbox path
    if (pathPrefix.find(APP_SANDBOX_PREFIX) != std::string::npos) {
        pathPrefix = CONVERT_APP_SANBOPX_PREFIX + bundleName + "/" + fileName;
    } else {
        pathPrefix = fullPath;
    }
    HIVIEW_LOGI("line: %{public}s, pathPrefix: %{public}s", line.c_str(), pathPrefix.c_str());
    return true;
}

bool IsHexString(const std::string& s)
{
    if (s.size() <= MIN_HEX_STR_SIZE) {
        return false;
    }
    if (!(s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))) {
        return false;
    }
    for (size_t i = MIN_HEX_STR_SIZE; i < s.size(); ++i) {
        char c = s[i];
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

bool FaultLogSanitizer::SafeStoull(const std::string& str, uint64_t& value, int base)
{
    if (str.empty()) {
        HIVIEW_LOGE("str is empty.");
        return false;
    }

    char* pEnd = nullptr;
    errno = 0;
    value = strtoull(str.c_str(), &pEnd, base);

    if (errno == ERANGE) {
        HIVIEW_LOGE("out of range, str: %{public}s", str.c_str());
        return false;
    }

    if (pEnd == str.c_str() || *pEnd != '\0') {
        HIVIEW_LOGE("conversion failed, str: %{public}s", str.c_str());
        return false;
    }

    HIVIEW_LOGD("success, str: %{public}s, result: %{public}" PRIu64, str.c_str(), value);
    return true;
}

bool FaultLogSanitizer::ExtractLoadInfo(const std::string& line, const std::vector<MapInfo>& maps,
    const std::string& bundleName, LoadInfo& info)
{
    // #idx pc  (filePath+relativePc)
    if (line.find('#') == std::string::npos) {
        return false;
    }
    std::istringstream iss(line);
    std::string idx;
    std::string pcStr;
    if (!(iss >> idx >> pcStr)) {
        return false;
    }
    if (!IsHexString(pcStr)) {
        return false;
    }
    // filePath may contain ' '
    std::string fileOffset;
    std::getline(iss, fileOffset);
    if (fileOffset.empty()) {
        return false;
    }
    fileOffset.erase(0, fileOffset.find_first_not_of(' '));

    size_t plusPos = fileOffset.rfind('+');
    if (plusPos == std::string::npos) {
        return false;
    }
    std::string relStr = fileOffset.substr(plusPos + 1);
    if (!relStr.empty() && relStr.back() == ')') {
        relStr.pop_back();
    }
    if (!IsHexString(relStr)) {
        return false;
    }
    if (!ConvertPathFromOriginLine(line, info.fullPath, bundleName)) {
        return false;
    }

    if (!SafeStoull(pcStr, info.pc, FaultLogger::HEX_BASE)) {
        HIVIEW_LOGE("failed to parse pc, pcStr: %{public}s", pcStr.c_str());
        return false;
    }
    if (!SafeStoull(relStr, info.relativePc, FaultLogger::HEX_BASE)) {
        HIVIEW_LOGE("failed to parse relativePc, relStr: %{public}s", relStr.c_str());
        return false;
    }

    for (const auto& m : maps) {
        if (info.pc >= m.start && info.pc < m.end) {
            info.mapBase = m.start;
            break;
        }
    }
    return true;
}

std::string FaultLogSanitizer::ProcessArkTsLine(const std::string& line, const std::string& packageName,
                                                const std::vector<MapInfo>& maps)
{
    LoadInfo info = {0, 0, 0, ""};
    if (!ExtractLoadInfo(line, maps, packageName, info)) {
        return line;
    }
    std::uintptr_t arkExtractorPtr = 0;
    int ret = 0;
    ret = DfxArk::Instance().ArkCreateJsSymbolExtractor(&arkExtractorPtr);
    if (ret < 0) {
        return line;
    }
    JsFunction jsFunc;
    if (info.fullPath.find("anon:ArkTS Code") == std::string::npos &&
        info.fullPath.find(".abc") == std::string::npos) {
        ret = DfxArk::Instance().ParseArkFileInfo(static_cast<uintptr_t>(info.pc - info.mapBase + OFFSET_HEAD),
                                                  0,
                                                  static_cast<uintptr_t>(info.relativePc - (info.pc - info.mapBase)),
                                                  info.fullPath.c_str(),
                                                  arkExtractorPtr, &jsFunc, true);
    } else {
        ret = DfxArk::Instance().ParseArkFileInfo(static_cast<uintptr_t>(info.relativePc + OFFSET_HEAD), 0, 0,
                                                  info.fullPath.c_str(), arkExtractorPtr, &jsFunc, true);
    }
    DfxArk::Instance().ArkDestoryJsSymbolExtractor(arkExtractorPtr);

    if (ret == -1) {
        HIVIEW_LOGE("original line: %{public}s", line.c_str());
        return line;
    }

    size_t hashPos = line.find('#');
    size_t stackFrameEnd = line.find(' ', hashPos);
    std::string stackFrame = line.substr(0, stackFrameEnd);
    std::string result = stackFrame;

    if (jsFunc.functionName[0] != '\0') {
        result += " " + std::string(jsFunc.functionName);
    }
    if (jsFunc.packageName[0] != '\0') {
        result += " " + std::string(jsFunc.packageName);
    }
    result += " (" + std::string(jsFunc.url) + ":" + std::to_string(jsFunc.line) +
              ":" + std::to_string(jsFunc.column) + ")";
    return result;
}

std::vector<MapInfo> FaultLogSanitizer::LoadMaps(std::ifstream& file)
{
    // 0xmapbaseStart-0xmapbaseStartend	xxx
    std::vector<MapInfo> maps;
    std::string line;
    bool inMapSection = false;

    while (std::getline(file, line)) {
        if (line.find("Process memory map follows") != std::string::npos) {
            inMapSection = true;
            continue;
        }
        if (!inMapSection) {
            continue;
        }
        if (line.find(".hap") == std::string::npos && line.find(".hsp") == std::string::npos &&
            line.find(".hqf") == std::string::npos) {
            continue;
        }
        std::istringstream iss(line);
        std::string range;
        std::string fileName;
        if (!(iss >> range >> fileName)) {
            continue;
        }

        auto dashPos = range.find('-');
        if (dashPos == std::string::npos) {
            continue;
        }

        MapInfo info{
            std::strtoull(range.substr(0, dashPos).c_str(), nullptr, FaultLogger::HEX_BASE),
            std::strtoull(range.substr(dashPos + 1).c_str(), nullptr, FaultLogger::HEX_BASE),
            fileName
        };
        maps.push_back(info);
    }
    return maps;
}

bool FaultLogSanitizer::ParserArkTsStackInfo(const std::string& moduleName, const std::string& path)
{
    auto fileSize = FileUtil::GetFileSize(path);
    if (fileSize > ARKTS_STACK_MAX_FILE_SIZE) {
        HIVIEW_LOGE("File size exceeds limit, path: %{public}s, size: %{public}" PRIu64,
                    path.c_str(), fileSize);
        return false;
    }
    std::ifstream srcLogFile(path);
    if (!srcLogFile.is_open()) {
        HIVIEW_LOGE("Failed to open src file: %{public}s", path.c_str());
        return false;
    }
    std::string tempPath = path + ".tmp";
    FILE* fp = fopen(tempPath.c_str(), "w");
    if (fp == nullptr) {
        HIVIEW_LOGE("Failed to open temp file: %{public}s", tempPath.c_str());
        srcLogFile.close();
        return false;
    }
    chmod(tempPath.c_str(), DEFAULT_LOG_FILE_MODE);
    std::vector<MapInfo> maps = LoadMaps(srcLogFile);

    int tempFileFd = fileno(fp);
    std::string line;

    srcLogFile.clear();
    srcLogFile.seekg(0, std::ios::beg);

    while (std::getline(srcLogFile, line)) {
        if (srcLogFile.eof()) {
            break;
        }
        if (!srcLogFile.good()) {
            break;
        }
        if (ShouldParseSandBoxPath(line)) {
            line = ProcessArkTsLine(line, moduleName, maps);
        }
        FileUtil::SaveStringToFd(tempFileFd, line);
        FileUtil::SaveStringToFd(tempFileFd, "\n");
    }
    srcLogFile.close();
    fsync(tempFileFd);
    (void)fclose(fp);
    fp = nullptr;

    return FileUtil::RenameFile(tempPath.c_str(), path.c_str());
}

bool FaultLogSanitizer::ForkProcessParseArkTsStackInfo(const std::string& moduleName, const std::string& path)
{
    if (moduleName.empty() || path.empty()) {
        return false;
    }
    pid_t childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("failed to fork process, err: %{public}s", strerror(errno));
        return false;
    }
    if (childPid == 0) {
        bool ret = ParserArkTsStackInfo(moduleName, path);
        if (!ret) {
            HIVIEW_LOGE("moduleName: %{public}s, err: %{public}s", moduleName.c_str(), strerror(errno));
            _exit(-1);
        }
        _exit(0);
    } else {
        int count = MAX_RETRY_COUNT;
        int ret = waitpid(childPid, nullptr, WNOHANG);
        while (count > 0 && (ret == 0)) {
            usleep(WAIT_CHILD_PROCESS_INTERVAL);
            count--;
            ret = waitpid(childPid, nullptr, WNOHANG);
        }

        if (ret == childPid) {
            return true;
        }
        if (ret < 0) {
            return false;
        }

        HIVIEW_LOGW("waitpid timeout, kill child process: %{public}d", childPid);
        kill(childPid, SIGKILL);
        int retryCount = MAX_RETRY_COUNT;
        while (retryCount > 0 && waitpid(childPid, nullptr, WNOHANG) == 0) {
            usleep(WAIT_CHILD_PROCESS_INTERVAL);
            retryCount--;
        }
        return false;
    }
    return true;
}

void FaultLogSanitizer::ReportSanitizerToAppEvent(std::shared_ptr<SysEvent> sysEvent)
{
    std::string summary = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue(FaultKey::SUMMARY));
    HIVIEW_LOGD("ReportSanitizerAppEvent:summary:%{public}s.", summary.c_str());

    Json::Value params;
    params["time"] = sysEvent->happenTime_;
    auto reason = sysEvent->GetEventValue(FaultKey::REASON);
    params["type"] = reason;
    if (reason.find("FDSAN") != std::string::npos) {
        params["type"] = "FDSAN";
        HIVIEW_LOGI("info reason: %{public}s, set sysEvent reason FDSAN", reason.c_str());
    } else if (reason.find("ARKTS_ENVSAN") != std::string::npos) {
        params["type"] = "ARKTS_ENVSAN";
        HIVIEW_LOGI("info reason: %{public}s, set sysEvent reason ARKTS_ENVSAN", reason.c_str());
    }
    Json::Value externalLog(Json::arrayValue);
    std::string logPath = sysEvent->GetEventValue(FaultKey::LOG_PATH);
    if (!logPath.empty()) {
        externalLog.append(logPath);
    }
    params["external_log"] = externalLog;
    params["bundle_version"] = sysEvent->GetEventValue(FaultKey::MODULE_VERSION);
    params["bundle_name"] = sysEvent->GetEventValue(FaultKey::MODULE_NAME);
    params["pid"] = sysEvent->GetPid();
    params["uid"] = sysEvent->GetUid();
    params["app_running_unique_id"] = sysEvent->GetEventValue("APP_RUNNING_UNIQUE_ID");
    std::string paramsStr = Json::FastWriter().write(params);
    HIVIEW_LOGD("ReportSanitizerAppEvent: uid:%{public}d, json:%{public}s.",
        sysEvent->GetUid(), paramsStr.c_str());
    EventPublish::GetInstance().PushEvent(sysEvent->GetUid(), "ADDRESS_SANITIZER",
        HiSysEvent::EventType::FAULT, paramsStr);
}

bool FaultLogSanitizer::ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent) const
{
    if (!info_.reportToAppEvent || !sysEvent) {
        return false;
    }
    auto task = [sysEvent] {
        ReportSanitizerToAppEvent(sysEvent);
    };
    constexpr uint64_t delayTime = 2 * 1000 * 1000; // Delay for 2 seconds to wait for ffrt log generation
    ffrt::submit(task, ffrt::task_attr().name("sanitizer_wait_ffrt").delay(delayTime));
    return true;
}

FaultLogSanitizer::FaultLogSanitizer()
{
    faultType_ = FaultLogType::ADDR_SANITIZER;
}

std::string FaultLogSanitizer::GetFaultModule(SysEvent& sysEvent) const
{
    return sysEvent.GetEventValue(FaultKey::MODULE_NAME);
}

void FaultLogSanitizer::ParseSanitizerEasyEvent(SysEvent& sysEvent)
{
    std::string data = sysEvent.GetEventValue("DATA");
    if (data.empty()) {
        HIVIEW_LOGW("Sanitizer receive empty hiSysEventEasy");
        return;
    }
    size_t start = 0;
    while (start < data.size()) {
        size_t end = data.find(';', start);
        if (end == std::string::npos) {
            end = data.size();
        }
        size_t pos = data.find(':', start);
        if (pos != std::string::npos && pos > start && pos < end) {
            std::string key = data.substr(start, pos - start);
            if (key == "SUMMARY") {
                std::string value = data.substr(pos + 1);
                sysEvent.SetEventValue(key, value);
                break;
            }
            std::string value = data.substr(pos + 1, end - pos - 1);
            sysEvent.SetEventValue(key, value);
        }
        start = end + 1;
    }
    sysEvent.SetEventValue("DATA", "");
}

FaultLogInfo FaultLogSanitizer::FillFaultLogInfo(SysEvent& sysEvent)
{
    auto info = FaultLogEventPipeline::FillFaultLogInfo(sysEvent);
    if (info.reason.find("FDSAN") != std::string::npos) {
        info.pid = sysEvent.GetEventIntValue("PID");
        info.time = sysEvent.GetEventIntValue("HAPPEN_TIME");
        info.logPath = GetDebugSignalTempLogName(info);
        info.summary = "";
        info.sanitizerType = "FDSAN";
    } else if (info.reason.find("ARKTS_ENVSAN") != std::string::npos) {
        info.pid = sysEvent.GetEventIntValue("PID");
        info.time = sysEvent.GetEventIntValue("HAPPEN_TIME");
        info.logPath = GetDebugSignalTempLogName(info);
        info.summary = "";
        info.sanitizerType = "ARKTS_ENVSAN";
    } else if (info.reason.find("DEBUG SIGNAL") != std::string::npos) {
        info.pid = sysEvent.GetEventIntValue(FaultKey::MODULE_PID);
        info.time = sysEvent.GetEventIntValue(FaultKey::HAPPEN_TIME);
        info.reportToAppEvent = false;
        info.dumpLogToFaultlogger = false;
        info.logPath = GetDebugSignalTempLogName(info);
    } else {
        ParseSanitizerEasyEvent(sysEvent);
        info.module = sysEvent.GetEventValue(FaultKey::MODULE_NAME);
        info.sanitizerType = sysEvent.GetEventValue(FaultKey::FAULT_TYPE);
        info.reason = sysEvent.GetEventValue(FaultKey::REASON);
        info.logPath = GetSanitizerTempLogName(info.pid, sysEvent.GetEventValue(FaultKey::HAPPEN_TIME));
        info.sectionMap[FaultKey::APP_RUNNING_UNIQUE_ID] = sysEvent.GetEventValue("APP_RUNNING_UNIQUE_ID");
        info.summary = "";
    }
    return info;
}

void FaultLogSanitizer::UpdateFaultLogInfo()
{
    if (info_.reason.find("FDSAN") != std::string::npos) {
        info_.sectionMap["APPEND_ORIGIN_LOG"] = info_.logPath;
        info_.logPath = "";
    }
    if (info_.reason.find("ARKTS_ENVSAN") != std::string::npos) {
        info_.sectionMap["APPEND_ORIGIN_LOG"] = info_.logPath;
        info_.logPath = "";
    }
}

void FaultLogSanitizer::UpdateSysEvent(SysEvent& sysEvent)
{
    // after log file create, parse arkTs stack
    ForkProcessParseArkTsStackInfo(info_.module, info_.logPath);
    // DEBUG SIGNAL does not need to update HAPPEN_TIME
    if (info_.reason.find("DEBUG SIGNAL") == std::string::npos) {
        sysEvent.SetEventValue(FaultKey::HAPPEN_TIME, sysEvent.happenTime_);
    }
    FaultLogEventPipeline::UpdateSysEvent(sysEvent);
}
} // namespace HiviewDFX
} // namespace OHOS
