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

#include "process_kill_event_handler/rss_kill_leak_info.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "dfr_util.h"
#include "hisysevent.h"

namespace OHOS {
namespace HiviewDFX {

using namespace std;
DEFINE_LOG_LABEL(0xD002DA0, "RssKillEventHandler");
namespace {
constexpr int WEIGHT_8 = 8;
constexpr int WEIGHT_20 = 20;
constexpr int SIZE_POS = 0;
constexpr int RSS_POS = 1;
constexpr int SWAP_POS = 2;
constexpr int INODE_POS = 3;
constexpr int PARAMS_POS = 4;
constexpr int SMAPS_LEN = 5;
}
std::unordered_map<MemoryClass, std::string> RssKillLeakInfo::memoryClassToStringMap_ = {
    {MemoryClass::MEMORY_CLASS_GRAPH, "graph"},
    {MemoryClass::MEMORY_CLASS_ARK_TS_HEAP, "ark ts heap"},
    {MemoryClass::MEMORY_CLASS_ARK_STATIC_HEAP, "arkts-static heap"},
    {MemoryClass::MEMORY_CLASS_DB, ".db"},
    {MemoryClass::MEMORY_CLASS_DEV, "dev"},
    {MemoryClass::MEMORY_CLASS_DMABUF, "dmabuf"},
    {MemoryClass::MEMORY_CLASS_GUARD, "guard"},
    {MemoryClass::MEMORY_CLASS_HAP, ".hap"},
    {MemoryClass::MEMORY_CLASS_NATIVE_HEAP, "native heap"},
    {MemoryClass::MEMORY_CLASS_SO, ".so"},
    {MemoryClass::MEMORY_CLASS_STACK, "stack"},
    {MemoryClass::MEMORY_CLASS_TTF, ".ttf"},
    {MemoryClass::MEMORY_CLASS_KMP_HEAP, "kotlin heap"},
    {MemoryClass::MEMORY_CLASS_FLUTTER_DART, "dart heap"},
    {MemoryClass::MEMORY_CLASS_RN_HERMES, "rn-hermes heap"},
    {MemoryClass::MEMORY_CLASS_JSVM_V8, "jsvm heap"},
    {MemoryClass::MEMORY_CLASS_ARKWEB_V8, "arkweb-js heap"},
    {MemoryClass::MEMORY_CLASS_ARKWEB_PA, "arkweb-pa heap"},
    {MemoryClass::MEMORY_CLASS_OTHER, "other"}
};

std::unordered_map<MemoryClass, std::function<void(RssKillLeakInfo*, RssSmapInfo&)>> RssKillLeakInfo::classHandlers_ = {
    {MemoryClass::MEMORY_CLASS_HAP, &RssKillLeakInfo::HandleHAP},
    {MemoryClass::MEMORY_CLASS_NATIVE_HEAP, &RssKillLeakInfo::HandleNativeHeap},
    {MemoryClass::MEMORY_CLASS_SO, &RssKillLeakInfo::HandleSO},
    {MemoryClass::MEMORY_CLASS_STACK, &RssKillLeakInfo::HandleStack},
    {MemoryClass::MEMORY_CLASS_TTF, &RssKillLeakInfo::HandleTTF},
    {MemoryClass::MEMORY_CLASS_OTHER, &RssKillLeakInfo::HandleOther},
    {MemoryClass::MEMORY_CLASS_ARK_TS_HEAP, &RssKillLeakInfo::HandleArkTSHeap},
    {MemoryClass::MEMORY_CLASS_DB, &RssKillLeakInfo::HandleDB},
    {MemoryClass::MEMORY_CLASS_DEV, &RssKillLeakInfo::HandleDev},
    {MemoryClass::MEMORY_CLASS_ARK_STATIC_HEAP, &RssKillLeakInfo::HandleArkTsStaHeap},
    {MemoryClass::MEMORY_CLASS_KMP_HEAP, &RssKillLeakInfo::HandleKmpHeap},
    {MemoryClass::MEMORY_CLASS_FLUTTER_DART, &RssKillLeakInfo::HandleArkDartHeap},
    {MemoryClass::MEMORY_CLASS_RN_HERMES, &RssKillLeakInfo::HandleRnHermesHeap},
    {MemoryClass::MEMORY_CLASS_JSVM_V8, &RssKillLeakInfo::HandleJsvmHeap},
    {MemoryClass::MEMORY_CLASS_ARKWEB_V8, &RssKillLeakInfo::HandleArkWebJsHeap},
    {MemoryClass::MEMORY_CLASS_ARKWEB_PA, &RssKillLeakInfo::HandleArkWebPaHeap},
};

bool RssKillLeakInfo::IsAshmemType(const string &str)
{
    string ashmemPrefix = "anon_inode:dev/ashmem/";
    if (ashmemPrefix.size() > str.size()) {
        return false;
    }
    return str.compare(0, ashmemPrefix.size(), ashmemPrefix) == 0;
}

void RssKillLeakInfo::HandleOther(RssSmapInfo &info)
{
    if (IsAshmemType(info.name)) {
        rssInfo_.ashmem += info.reportSize;
    } else {
        if (info.inode == 0) {
            rssInfo_.anonPageOther += info.reportSize;
        } else {
            rssInfo_.filePageOther += info.reportSize;
        }
    }
}

void RssKillLeakInfo::HandleHAP(RssSmapInfo &info)
{
    rssInfo_.hap += info.reportSize;
}

void RssKillLeakInfo::HandleNativeHeap(RssSmapInfo &info)
{
    rssInfo_.nativeHeap += info.reportSize;
}

void RssKillLeakInfo::HandleSO(RssSmapInfo &info)
{
    rssInfo_.so += info.reportSize;
}

void RssKillLeakInfo::HandleStack(RssSmapInfo &info)
{
    rssInfo_.stack += info.reportSize;
}

void RssKillLeakInfo::HandleTTF(RssSmapInfo &info)
{
    rssInfo_.ttf += info.reportSize;
}

void RssKillLeakInfo::HandleArkTSHeap(RssSmapInfo &info)
{
    rssInfo_.arktsHeap += info.reportSize;
}

void RssKillLeakInfo::HandleDB(RssSmapInfo &info)
{
    rssInfo_.db += info.reportSize;
}

void RssKillLeakInfo::HandleDev(RssSmapInfo &info)
{
    rssInfo_.dev += info.reportSize;
}

void RssKillLeakInfo::HandleArkTsStaHeap(RssSmapInfo &info)
{
    rssInfo_.arktsStaHeap += info.reportSize;
}

void RssKillLeakInfo::HandleKmpHeap(RssSmapInfo &info)
{
    rssInfo_.kotlinHeap += info.reportSize;
}

void RssKillLeakInfo::HandleArkDartHeap(RssSmapInfo &info)
{
    rssInfo_.dartHeap += info.reportSize;
}

void RssKillLeakInfo::HandleRnHermesHeap(RssSmapInfo &info)
{
    rssInfo_.rnHermesHeap += info.reportSize;
}

void RssKillLeakInfo::HandleJsvmHeap(RssSmapInfo &info)
{
    rssInfo_.jsvmHeap += info.reportSize;
}

void RssKillLeakInfo::HandleArkWebJsHeap(RssSmapInfo &info)
{
    rssInfo_.arkwebV8 += info.reportSize;
}

void RssKillLeakInfo::HandleArkWebPaHeap(RssSmapInfo &info)
{
    rssInfo_.arkwebPa += info.reportSize;
}

RssKillLeakInfo::RssKillLeakInfo(const ProcesssKillEventParam &eventParam) : ProcessKillLeakInfo(eventParam)
{
}

void RssKillLeakInfo::SysEventReport()
{
    string version = GetApplicationVersion();
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK", HiSysEvent::EventType::FAULT,
        "PID", pid_,
        "UID", uid_,
        "HAPPEN_TIME", leakedTime_,
        "FIRST_FRAME", "RSS_LEAK",
        "SECOND_FRAME", "OVER_LIMIT",
        "MODULE", processName_,
        "VERSION", version.empty() ? "/" : version,
        "LOGPATH", GetFilePath() + GetFileName(),
        "SUMMARY", GetDetailMsg(),
        "APP_RUNNING_ID", appRunningId_,
        "FINGERPRINT",
        DfrUtil::GetInstance().CalcFingerPrint(processName_ + version + eventName_ + "OVER_LIMIT", 0));
}

void RssKillLeakInfo::AppEventReport()
{
    string version = GetApplicationVersion();
    AppEventHandler::ResourceOverLimitInfo rolInfo;
    rolInfo.pid = pid_;
    rolInfo.uid = uid_;
    rolInfo.bundleName = processName_;
    rolInfo.bundleVersion = version.empty() ? "/" : version;
    rolInfo.appRunningUniqueId = appRunningId_;
    std::vector<std::string> logPath;
    logPath.push_back("\"" + GetFilePath() + GetFileName() + "\"");
    rolInfo.logPath = logPath;
    rolInfo.resourceType = "rss_memory";
    rolInfo.level = "kill";
    rolInfo.rss = static_cast<uint64_t>(rss_);
    rolInfo.rssInfo = rssInfo_;
    bool ret = DfrUtil::GetInstance().PostAppEvent(rolInfo);
    if (!ret) {
        HIVIEW_LOGE("post resource limit event failed");
    } else {
        HIVIEW_LOGI("post resource limit event success");
    }
}

string RssKillLeakInfo::ProcessSmapsClassByInfo(shared_ptr<UCollectUtil::MemoryCollector> &collector,
                                                vector<string> &tokens, string &smapsName)
{
    int rss = 0;
    int swap = 0;
    int inode = 0;
    if (!DfrUtil::GetInstance().SafeStoi(tokens[RSS_POS], rss) ||
        !DfrUtil::GetInstance().SafeStoi(tokens[SWAP_POS], swap) ||
        !DfrUtil::GetInstance().SafeStoi(tokens[INODE_POS], inode)) {
        HIVIEW_LOGE("safestoi failed when get rss, swap or inode");
        return "";
    }
    if (!collector) {
        HIVIEW_LOGE("collector is nullptr");
        return "";
    }
    MemoryClass smapsclass = collector->MapMemoryTypeToClass(collector->MapNameToMemoryType(smapsName));
    auto func = classHandlers_.find(smapsclass);
    if (func != classHandlers_.end()) {
        RssSmapInfo smapsInfo(smapsName, rss + swap, inode);
        func->second(this, smapsInfo);
    }
    auto it = memoryClassToStringMap_.find(smapsclass);
    return it != memoryClassToStringMap_.end() ? it->second : "UNKNOWN";
}

string RssKillLeakInfo::GetMemorySmapsName(vector<string> &tokens)
{
    string smapsName = "";
    for (size_t index = PARAMS_POS + 1; index < tokens.size() - 1; ++index) {
        smapsName += tokens[index] + " ";
    }
    return smapsName + tokens.back();
}

std::vector<std::string> RssKillLeakInfo::ParseLineToTokens(const std::string &line)
{
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        if (token == "kB") {
            continue;
        }
        tokens.push_back(token);
    }
    return tokens;
}

bool RssKillLeakInfo::IsValidTokens(std::vector<std::string> &tokens, long long &size)
{
    if (tokens.size() == SMAPS_LEN) {
        tokens.push_back("[anon]");
    }
    if (tokens.empty() || tokens.size() <= SMAPS_LEN) {
        return false;
    }
    return DfrUtil::GetInstance().SafeStoll(tokens[SIZE_POS], size);
}

void RssKillLeakInfo::AccumulateClusterData(SmapsClusterInfo &clusterInfo, long long size,
                                            const std::vector<std::string> &tokens, long long &totalRss)
{
    clusterInfo.size += size;

    long long rss = 0;
    if (tokens.size() > RSS_POS && DfrUtil::GetInstance().SafeStoll(tokens[RSS_POS], rss)) {
        clusterInfo.rss += rss;
        totalRss += rss;
    }

    long long swap = 0;
    if (tokens.size() > SWAP_POS && DfrUtil::GetInstance().SafeStoll(tokens[SWAP_POS], swap)) {
        clusterInfo.swap += swap;
        totalRss += swap;
    }

    clusterInfo.hits++;
}

void RssKillLeakInfo::RefineOtherCategory(std::string &memClass, const std::vector<std::string> &tokens,
                                          const std::string &smapsName)
{
    if (memClass != "other") {
        return;
    }

    int inode = 0;
    if (!DfrUtil::GetInstance().SafeStoi(tokens[INODE_POS], inode)) {
        return;
    }
    if (inode == 0) {
        memClass = "AnonPage other";
    } else {
        memClass = "FilePage other";
    }
}

string RssKillLeakInfo::HandleDetailInfo(const string &detailInfo)
{
    string line;
    stringstream finBufferStream(detailInfo);
    shared_ptr<UCollectUtil::MemoryCollector> collector = UCollectUtil::MemoryCollector::Create();
    std::vector<std::pair<string, SmapsClusterInfo>> clusterList;
    std::unordered_map<string, size_t> clusterIndexMap;
    rssInfo_ = {};

    while (std::getline(finBufferStream, line)) {
        if (line.find("process_name:") != string::npos) {
            continue;
        }
        std::vector<std::string> tokens = ParseLineToTokens(line);
        long long size = 0;
        if (!IsValidTokens(tokens, size)) {
            continue;
        }
        string smapsName = GetMemorySmapsName(tokens);

        size_t index;
        auto it = clusterIndexMap.find(smapsName);
        if (it == clusterIndexMap.end()) {
            SmapsClusterInfo info;
            info.memClass = ProcessSmapsClassByInfo(collector, tokens, smapsName);
            RefineOtherCategory(info.memClass, tokens, smapsName);

            clusterList.push_back({smapsName, info});
            index = clusterList.size() - 1;
            clusterIndexMap[smapsName] = index;
        } else {
            index = it->second;
            ProcessSmapsClassByInfo(collector, tokens, smapsName);
        }
        AccumulateClusterData(clusterList[index].second, size, tokens, rss_);
    }

    std::ostringstream out;
    out << setw(WEIGHT_8) << "Size" << "    " << setw(WEIGHT_8) << "Rss" << "    " << setw(WEIGHT_8) << "Swap"
        << "    " << setw(WEIGHT_20) << "Category" << "    " << setw(WEIGHT_8) << "Counts" << "    " << "Name" << endl;
    for (const auto& entry : clusterList) {
        const SmapsClusterInfo& info = entry.second;
        out << setw(WEIGHT_8) << info.size << "    " << setw(WEIGHT_8) << info.rss << "    "
            << setw(WEIGHT_8) << info.swap << "    " << setw(WEIGHT_20) << info.memClass << "    "
            << setw(WEIGHT_8) << info.hits << "    " << entry.first << endl;
    }

    return out.str();
}

void RssKillLeakInfo::SubSysEventReport()
{
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK_RSS", HiSysEvent::EventType::FAULT,
        "APP_RUNNING_ID", appRunningId_,
        "MODULE", processName_,
        "ADJ", adj_,
        "DEVICE_RAM_SIZE", DfrUtil::GetInstance().GetRamSize(),
        "NATIVE_HEAP", rssInfo_.nativeHeap,
        "HAP", rssInfo_.hap,
        "STACK", rssInfo_.stack,
        "DB", rssInfo_.db,
        "SO", rssInfo_.so,
        "DEV", rssInfo_.dev,
        "TTF", rssInfo_.ttf,
        "ASHMEM", rssInfo_.ashmem,
        "ARKTS_HEAP", rssInfo_.arktsHeap,
        "ANON_PAGE_OTHER", rssInfo_.anonPageOther,
        "FILE_PAGE_OTHER", rssInfo_.filePageOther,
        "ARKTS_STA_HEAP", rssInfo_.arktsStaHeap,
        "KOTLIN_HEAP", rssInfo_.kotlinHeap,
        "FLUTTER_DART_HEAP", rssInfo_.dartHeap,
        "RN_HERMES_HEAP", rssInfo_.rnHermesHeap,
        "JSVM_HEAP", rssInfo_.jsvmHeap,
        "ARKWEB_V8", rssInfo_.arkwebV8,
        "ARKWEB_PA", rssInfo_.arkwebPa,
        "RSS_TOTAL_SIZE", rss_);
}

void RssKillLeakInfo::HandleSubeventInfo(const std::string &data) {}

void RssKillLeakInfo::DumpGeneralInfo(ofstream &fout)
{
    fout << endl;
    fout << "Generated by HiviewDFX @OpenHarmony" << endl;
    fout << "LOGGER_MEMCHECK_GERNAL_INFO" << endl;
    fout << "\tpidNumber: " << GetPid() << endl;
    fout << "\tprocessName: " << GetProcessName() << endl;
    fout << "\tkillTime: " << leakedTime_ << endl;
    fout << "\tRssMemory: " << rss_ << endl;
    fout << "\tkillReason: " << GetKillReason() << endl;
    fout << endl;
}
} // HiviewDFX
} // OHOS
