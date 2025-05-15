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
#include "faultlog_dump.h"

#include <functional>

#include "accesstoken_kit.h"
#include "bundle_mgr_client.h"
#include "constants.h"
#include "faultlog_bundle_util.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "ipc_skeleton.h"
#include "time_util.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FaultLogger;
namespace {
const char * const FILE_SEPARATOR = "******";
constexpr uint32_t DUMP_MAX_NUM = 100;
constexpr int DUMP_PARSE_CMD = 0;
constexpr int DUMP_PARSE_FILE_NAME = 1;
constexpr int DUMP_PARSE_TIME = 2;
constexpr int DUMP_START_PARSE_MODULE_NAME = 3;
constexpr uint32_t MAX_NAME_LENGTH = 4096;
bool IsLogNameValid(const std::string& name)
{
    const int32_t idxOfType = 0;
    const int32_t idxOfMoudle = 1;
    const int32_t idxOfUid = 2;
    const int32_t idxOfTime = 3;
    const int32_t expectedVecSize = 4;
    const size_t tailWithMillSecLen = 7u;
    if (name.empty() || name.size() > MAX_NAME_LENGTH) {
        HIVIEW_LOGI("invalid log name.");
        return false;
    }

    std::vector<std::string> out;
    StringUtil::SplitStr(name, "-", out, true, false);
    if (out.size() != expectedVecSize) {
        return false;
    }

    std::regex reType("^[a-z]+$");
    if (!std::regex_match(out[idxOfType], reType)) {
        HIVIEW_LOGI("invalid type.");
        return false;
    }

    if (!IsModuleNameValid(out[idxOfMoudle])) {
        HIVIEW_LOGI("invalid module name.");
        return false;
    }

    std::regex reDigits("^[0-9]*$");
    if (!std::regex_match(out[idxOfUid], reDigits)) {
        HIVIEW_LOGI("invalid uid.");
        return false;
    }

    if (StringUtil::EndWith(out[idxOfTime], ".log") && out[idxOfTime].length() > tailWithMillSecLen) {
        out[idxOfTime] = out[idxOfTime].substr(0, out[idxOfTime].length() - tailWithMillSecLen);
    }

    if (!std::regex_match(out[idxOfTime], reDigits)) {
        HIVIEW_LOGI("invalid digits.");
        return false;
    }
    return true;
}

bool FillDumpRequest(DumpRequest& request, int status, const std::string& item)
{
    switch (status) {
        case DUMP_PARSE_FILE_NAME:
            if (!IsLogNameValid(item)) {
                return false;
            }
            request.fileName = item;
            break;
        case DUMP_PARSE_TIME:
            if (item.size() == 14) { // 14 : BCD time size
                request.time = TimeUtil::StrToTimeStamp(item, "%Y%m%d%H%M%S");
            } else {
                StringUtil::ConvertStringTo<time_t>(item, request.time);
            }
            break;
        case DUMP_START_PARSE_MODULE_NAME:
            if (!IsModuleNameValid(item)) {
                return false;
            }
            request.moduleName = item;
            break;
        default:
            HIVIEW_LOGI("Unknown status.");
            break;
    }
    return true;
}
}  // namespace

bool FaultLogDump::DumpByFileName(const DumpRequest& request) const
{
    if (!request.fileName.empty()) {
        std::string content;
        if (faultLogManager_->GetFaultLogContent(request.fileName, content)) {
            dprintf(fd_, "%s\n", content.c_str());
        } else {
            dprintf(fd_, "Fail to dump the log.\n");
        }
        return true;
    }
    return false;
}

void FaultLogDump::PrintFileMaps(const DumpRequest& request, std::map<std::string, std::string> fileNameMap) const
{
    dprintf(fd_, "Fault log list:\n");
    dprintf(fd_, "%s\n", FILE_SEPARATOR);
    for (auto it = fileNameMap.begin(); it != fileNameMap.end(); ++it) {
        dprintf(fd_, "%s\n", it->first.c_str());
        if (request.requestDetail) {
            std::string content;
            std::string fullFileName = FAULTLOG_FAULT_LOGGER_FOLDER + it->second;
            if (FileUtil::LoadStringFromFile(fullFileName, content)) {
                dprintf(fd_, "%s\n", content.c_str());
            } else {
                dprintf(fd_, "Fail to dump detail log.\n");
            }
            dprintf(fd_, "%s\n", FILE_SEPARATOR);
        }
    }
    dprintf(fd_, "%s\n", FILE_SEPARATOR);
}

std::map<std::string, std::string> FaultLogDump::FindByModule(const DumpRequest& request) const
{
    auto fileList = faultLogManager_->GetFaultLogFileList(request.moduleName, request.time, -1, 0, DUMP_MAX_NUM);
    if (fileList.empty()) {
        dprintf(fd_, "No fault log exist.\n");
        return {};
    }
    std::map<std::string, std::string> fileNameMap;
    const size_t tailWithMillisecLen = 7;
    for (const auto &file : fileList) {
        std::string fileName = FileUtil::ExtractFileName(file);
        if (fileName.length() <= tailWithMillisecLen) {
            continue;
        }
        if (!request.compatFlag && StringUtil::EndWith(fileName, ".log") == false) {
            continue;
        } else if (request.compatFlag && StringUtil::EndWith(fileName, ".log")) {
            if (fileNameMap[fileName.substr(0, fileName.length() - tailWithMillisecLen)].compare(fileName) < 0) {
                fileNameMap[fileName.substr(0, fileName.length() - tailWithMillisecLen)] = fileName;
            }
            continue;
        }
        fileNameMap[fileName] = fileName;
    }
    return fileNameMap;
}

void FaultLogDump::DumpByModule(const DumpRequest& request) const
{
    auto fileNameMap = FindByModule(request);
    PrintFileMaps(request, fileNameMap);
}

void FaultLogDump::DumpByRequest(const DumpRequest& request) const
{
    if (DumpByFileName(request)) {
        return;
    }
    DumpByModule(request);
}

ParseCmdResult FaultLogDump::HandleCommandOption(const std::string& cmd, int32_t& status, DumpRequest& request) const
{
    const std::map<std::string, std::function<void(int32_t&, DumpRequest&)>> cmdHandlers = {
        {"-f", [](int32_t& status, DumpRequest& request) { status = DUMP_PARSE_FILE_NAME; }},
        {"-l", [](int32_t& status, DumpRequest& request) { request.requestList = true; }},
        {"-t", [](int32_t& status, DumpRequest& request) { status = DUMP_PARSE_TIME; }},
        {"-m", [](int32_t& status, DumpRequest& request) { status = DUMP_START_PARSE_MODULE_NAME; }},
        {"-d", [](int32_t& status, DumpRequest& request) { request.requestDetail = true; }},
        {"Faultlogger", [](int32_t& status, DumpRequest& request) { request.compatFlag = true; }},
        {"-LogSuffixWithMs", [](int32_t& status, DumpRequest& request) { request.compatFlag = false; }}
    };

    if (cmdHandlers.count(cmd)) {
        cmdHandlers.at(cmd)(status, request);
        return ParseCmdResult::SUCCESS;
    }

    if (!cmd.empty() && cmd.at(0) == '-') {
        dprintf(fd_, "Unknown command.\n");
        return ParseCmdResult::UNKNOWN;
    }
    return ParseCmdResult::MAX;
}

bool FaultLogDump::ParseDumpCommands(const std::vector<std::string>& cmds, DumpRequest& request, int32_t& status) const
{
    for (const auto& cmd: cmds) {
        auto result = HandleCommandOption(cmd, status, request);
        if (result == ParseCmdResult::SUCCESS) {
            continue;
        } else if (result == ParseCmdResult::UNKNOWN) {
            return false;
        }
        if (!FillDumpRequest(request, status, cmd)) {
            dprintf(fd_, "invalid parameters.\n");
            return false;
        }
        status = DUMP_PARSE_CMD;
    }
    return true;
}

void FaultLogDump::DumpByCommands(const std::vector<std::string>& cmds) const
{
    if (!VerifiedDumpPermission()) {
        dprintf(fd_, "dump operation is not permitted.\n");
        return;
    }
    DumpRequest request;
    int32_t status = DUMP_PARSE_CMD;
    if (!ParseDumpCommands(cmds, request, status)) {
        return;
    }
    if (status != DUMP_PARSE_CMD) {
        dprintf(fd_, "empty parameters.\n");
        return;
    }

    HIVIEW_LOGI("DumpRequest: detail:%{public}d, list:%{public}d, file:%{public}s, name:%{public}s, time:%{public}lld",
        request.requestDetail, request.requestList, request.fileName.c_str(), request.moduleName.c_str(),
        static_cast<long long>(request.time));
    DumpByRequest(request);
}
bool FaultLogDump::VerifiedDumpPermission() const
{
    using namespace Security::AccessToken;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    return AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.DUMP") == PermissionState::PERMISSION_GRANTED;
}
}  // namespace HiviewDFX
}  // namespace OHOS
