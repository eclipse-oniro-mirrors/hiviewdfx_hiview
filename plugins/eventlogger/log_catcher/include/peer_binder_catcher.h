/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef EVENT_LOGGER_PEER_BINDER_LOG_CATCHER
#define EVENT_LOGGER_PEER_BINDER_LOG_CATCHER
#include <fstream>
#include <string>
#include <memory>
#include <vector>

#include "sys_event.h"

#include "event_log_catcher.h"
namespace OHOS {
namespace HiviewDFX {
class PeerBinderCatcher : public EventLogCatcher {
public:
    explicit PeerBinderCatcher();
    ~PeerBinderCatcher() override{};
    bool Initialize(const std::string& perfCmd, int layer, int pid) override;
    int Catch(int fd, int jsonFd) override;
    void Init(std::shared_ptr<SysEvent> event, const std::string& filePath, std::set<int>& catchedPids);

private:
    struct BinderInfo {
        int client;
        int server;
        int wait;
    };
    struct OutputBinderInfo {
        std::string info = "";
        int pid = 0;
    };

    int pid_ = 0;
    int layer_ = 0;
    std::string perfCmd_ = "";
    std::string binderPath_ = "/proc/transaction_proc";
    std::shared_ptr<SysEvent> event_ = nullptr;
    std::set<int> catchedPids_ = {0};
    std::map<int, std::list<PeerBinderCatcher::BinderInfo>> BinderInfoParser(std::ifstream& fin,
        int fd, int jsonFd) const;
    void BinderInfoParser(std::ifstream& fin, int fd,
        std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
        std::list<PeerBinderCatcher::OutputBinderInfo>& outputBinderInfoList) const;
    void GetFileToList(std::string line, std::vector<std::string>& strList) const;
    void ParseBinderCallChain(std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
    std::set<int>& pids, int pid) const;
    std::set<int> GetBinderPeerPids(int fd, int jsonFd) const;
    bool IsAncoProc(int pid) const;
    void CatcherStacktrace(int fd, int pid) const;
    void AddBinderJsonInfo(std::list<OutputBinderInfo> outputBinderInfoList, int jsonFd) const;
#ifdef HAS_HIPERF
    void ForkToDumpHiperf(const std::set<int>& pids);
    void DoExecHiperf(const std::string& fileName, const std::set<int>& pids);
#endif
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_PEER_BINDER_LOG_CATCHER