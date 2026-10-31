/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DFR_RSS_KILL_EVENT_HANDLER_H
#define DFR_RSS_KILL_EVENT_HANDLER_H

#include "process_kill_event_handler.h"
#include <functional>
#include "memory.h"
#include "memory_collector.h"
namespace OHOS {
namespace HiviewDFX {

struct RssSmapInfo {
    std::string name = "";
    int reportSize = 0;
    int inode = 0;

    RssSmapInfo(const std::string &name, int reportSize, int inode)
        : name(name), reportSize(reportSize), inode(inode) {}
};

struct SmapsClusterInfo {
    long long size = 0;
    long long rss = 0;
    long long swap = 0;
    std::string memClass;
    int hits = 0;
};

class RssKillLeakInfo : public ProcessKillLeakInfo {
public:
    explicit RssKillLeakInfo(const ProcesssKillEventParam &eventParam);
    void SysEventReport() override;
    void AppEventReport() override;
    void SubSysEventReport() override;
    void HandleSubeventInfo(const std::string &data) override;
    void DumpGeneralInfo(std::ofstream &fout) override;
    std::string HandleDetailInfo(const std::string &detailInfo);

private:
    void HandleHAP(RssSmapInfo &info);
    void HandleNativeHeap(RssSmapInfo &info);
    void HandleSO(RssSmapInfo &info);
    void HandleStack(RssSmapInfo &info);
    void HandleTTF(RssSmapInfo &info);
    void HandleOther(RssSmapInfo &info);
    void HandleArkTSHeap(RssSmapInfo &info);
    void HandleDB(RssSmapInfo &info);
    void HandleDev(RssSmapInfo &info);
    void HandleArkTsStaHeap(RssSmapInfo &info);
    void HandleKmpHeap(RssSmapInfo &info);
    void HandleArkDartHeap(RssSmapInfo &info);
    void HandleRnHermesHeap(RssSmapInfo &info);
    void HandleJsvmHeap(RssSmapInfo &info);
    void HandleArkWebJsHeap(RssSmapInfo &info);
    void HandleArkWebPaHeap(RssSmapInfo &info);

    static bool IsAshmemType(const std::string &str);
    static std::string GetMemorySmapsName(std::vector<std::string> &tokens);
    std::string ProcessSmapsClassByInfo(std::shared_ptr<UCollectUtil::MemoryCollector> &collector,
                                        std::vector<std::string> &tokens, std::string &smapsName);
    static std::vector<std::string> ParseLineToTokens(const std::string &line);
    static bool IsValidTokens(std::vector<std::string> &tokens, long long &size);
    static void AccumulateClusterData(SmapsClusterInfo &clusterInfo, long long size,
                                      const std::vector<std::string> &tokens, long long &totalRss);
    static void RefineOtherCategory(std::string &memClass, const std::vector<std::string> &tokens,
                                    const std::string &smapsName);

private:
    AppEventHandler::DetailRssInfo rssInfo_;
    long long rss_ {0};
    static std::unordered_map<MemoryClass, std::string> memoryClassToStringMap_;
    static std::unordered_map<MemoryClass, std::function<void(RssKillLeakInfo*, RssSmapInfo&)>> classHandlers_;
};
} // HiviewDFX
} // OHOS
#endif // DFR_RSS_KILL_EVENT_HANDLER_H
