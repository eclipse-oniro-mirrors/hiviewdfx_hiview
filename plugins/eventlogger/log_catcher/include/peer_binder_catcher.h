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
class PeerBinderCatcher : public EventLogCatcher {
public:
    explicit PeerBinderCatcher();
    ~PeerBinderCatcher() override{};
    bool Initialize(const std::string& perfCmd, int layer, int pid) override;
    int Catch(int fd, int jsonFd) override;
    void Init(std::shared_ptr<SysEvent> event, const std::string& filePath, std::set<int>& catchedPids);

    static const inline std::string LOGGER_EVENT_PEERBINDER = "PeerBinder";
    static const inline std::string LOGGER_BINDER_DEBUG_PROC_PATH = "/proc/transaction_proc";
    static constexpr int BP_CMD_LAYER_INDEX = 1;
    static constexpr int BP_CMD_PERF_TYPE_INDEX = 2;
    static constexpr int PERF_LOG_EXPIRE_TIME = 60;
    static constexpr size_t BP_CMD_SZ = 3;

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
    enum {
        LOGGER_BINDER_STACK_ONE = 0,
        LOGGER_BINDER_STACK_ALL = 1,
    };

    int pid_ = 0;
    int layer_ = 0;
    std::string perfCmd_ = "";
    std::string binderPath_ = LOGGER_BINDER_DEBUG_PROC_PATH;
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
    void ParseBinderCallChain(std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
        std::set<int>& pids, int pid) const;
    std::set<int> GetBinderPeerPids(int fd, int jsonFd, std::set<int>& asyncPids) const;
    bool IsAncoProc(int pid) const;
    void CatcherFfrtStack(int fd, int pid) const;
    void CatcherStacktrace(int fd, int pid, bool sync = true) const;
    void AddBinderJsonInfo(std::list<OutputBinderInfo> outputBinderInfoList, int jsonFd) const;
#ifdef HAS_HIPERF
    void DumpHiperf(const std::set<int>& pids);
    void DoExecHiperf(const std::string& fileName, const std::set<int>& pids);
#endif
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_PEER_BINDER_LOG_CATCHER