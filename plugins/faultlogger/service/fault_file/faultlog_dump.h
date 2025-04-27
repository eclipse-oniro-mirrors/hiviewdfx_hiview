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
#ifndef FAULTLOG_DUMP_H
#define FAULTLOG_DUMP_H

#include <string>
#include <vector>

#include "faultlog_manager.h"

namespace OHOS {
namespace HiviewDFX {
struct DumpRequest {
    bool requestDetail {false};
    bool requestList {false};
    bool compatFlag {false};
    std::string fileName;
    std::string moduleName;
    time_t time {-1};
};

enum class ParseCmdResult {
    SUCCESS,
    UN_KNOWN,
    MAX
};

class FaultLogDump {
public:
    FaultLogDump(int fd, const std::shared_ptr<FaultLogManager>& faultLogManager)
        : fd_(fd), faultLogManager_(faultLogManager) {}
    void DumpByCommands(const std::vector<std::string>& cmds) const;
private:
    bool VerifiedDumpPermission() const;
    void DumpByRequest(const DumpRequest& request) const;
    ParseCmdResult HandleCommandOption(const std::string& cmd, int32_t &status, DumpRequest& request) const;
    bool ParseDumpCommands(const std::vector<std::string>& cmds, DumpRequest& request, int32_t& status) const;
    bool DumpByFileName(const DumpRequest& request) const;
    void DumpByModule(const DumpRequest& request) const;
    void PrintFileMaps(const DumpRequest& request, std::map<std::string, std::string> fileNameMap) const;
    std::map<std::string, std::string> FindByModule(const DumpRequest& request) const;
private:
    int fd_{-1};
    std::shared_ptr<FaultLogManager> faultLogManager_;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif
