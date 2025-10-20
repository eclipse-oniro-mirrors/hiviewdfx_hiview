/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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
#ifdef BINDER_CATCHER_ENABLE
class PeerBinderCatcher : public EventLogCatcher {
public:
    explicit PeerBinderCatcher();
    ~PeerBinderCatcher() override{};
    bool Initialize(const std::string& perfCmd, int layer, int pid) override;
    int Catch(int fd, int jsonFd) override;
    void Init(std::shared_ptr<SysEvent> event, const std::string& filePath, std::set<int>& catchedPids);

private:
    struct BinderInfo {
        int clientPid;
        int clientTid;
        int serverPid;
        int serverTid;
        int wait;
    };
    struct OutputBinderInfo {
        std::string info = "";
        int pid = 0;
    };
    struct ParseBinderParam {
        int eventPid;
        int eventTid;
    };

    int pid_ = 0;
    int layer_ = 0;
    std::string perfCmd_ = "";
    std::string binderPath_ = "/proc/transaction_proc";
    std::shared_ptr<SysEvent> event_ = nullptr;
    std::set<int> catchedPids_ = {0};
    std::map<int, std::list<PeerBinderCatcher::BinderInfo>> BinderInfoParser(std::ifstream& fin,
        int fd, int jsonFd, std::set<int>& asyncPids) const;
    void BinderInfoParser(std::ifstream& fin, int fd,
        std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
        std::list<PeerBinderCatcher::OutputBinderInfo>& outputBinderInfoList, std::set<int>& asyncPids) const;
    void BinderInfoLineParser(std::ifstream& fin, int fd,
        std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
        std::list<PeerBinderCatcher::OutputBinderInfo>& outputBinderInfoList,
        std::map<uint32_t, uint32_t>& asyncBinderMap,
        std::vector<std::pair<uint32_t, uint64_t>>& freezeAsyncSpacePairs) const;
    std::vector<std::string> GetFileToList(std::string line) const;
    std::string StrSplit(const std::string& str, uint16_t index) const;
    void SaveBinderLineToFd(int fd, const std::string& line, bool& isBinderMatchup) const;
    void ParseBinderCallChain(std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
        std::set<int>& pids, int pid, const PeerBinderCatcher::ParseBinderParam& params, bool getTerminal);
    std::set<int> GetBinderPeerPids(int fd, int jsonFd, std::set<int>& asyncPids);
    bool IsAncoProc(int pid) const;
    void CatcherFfrtStack(int fd, int pid) const;
    void CatcherStacktrace(int fd, int pid, bool sync = true);
    void AddBinderJsonInfo(std::list<OutputBinderInfo> outputBinderInfoList, int jsonFd) const;
#ifdef HAS_HIPERF
    void DumpHiperf(const std::set<int>& pids, int processId, const std::string& perfCmd);
    void DoExecHiperf(const std::string& fileName, const std::set<int>& pids, int processId,
        const std::string& perfCmd);
#endif
    int32_t GetUidByPid(const int32_t pid);
    std::string CatchSyncPid(int fd, const std::set<int>& asyncPids, const std::set<int>& syncPids);
};
#endif // BINDER_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_PEER_BINDER_LOG_CATCHER