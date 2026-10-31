/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * Author: huangyu
 * Create: 2023-11-16
 */

#include "notify_leak_info.h"
#include <parameters.h>
#include "telemetry_manager.h"
#include "notify_leak_compress_manager.h"
#include "old_log_cleaner.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int SEC_PER_MIN = 60;
constexpr int ARK_TOOL_USED_TIME = 15 * SEC_PER_MIN;
}

ffrt::mutex NotifyLeakInfo::rawheapListMtx_;
map<string, int64_t> NotifyLeakInfo::RawheapDumpList_;
atomic<int> NotifyLeakInfo::dumpCnt_ = system::GetIntParameter("persist.hiview.oomdump.maxcount", 0);

DEFINE_LOG_LABEL(0xD002DA0, "NotifyLeakInfo");

NotifyLeakInfo::NotifyLeakInfo(pid_t pid, string name)
    : FaultInfoBase(pid, name)
{
    // only for arkwev_v8 leak render process pass CouldTelemetry, then compress rawheap
    const std::string suffix = ":render";
    if (processName_.length() >= suffix.length() &&
        processName_.compare(processName_.length() - suffix.length(), suffix.length(), suffix) == 0) {
        bundleName_ = processName_;
    }
}

NotifyLeakInfo::~NotifyLeakInfo() {};

NotifyType NotifyLeakInfo::GetNotifyType() const
{
    return notifyType_;
}

void NotifyLeakInfo::SetNotifyType(NotifyType notifyType)
{
    notifyType_ = notifyType;
}

string NotifyLeakInfo::GetNotifyDir() const
{
    return notifyDir_;
}

void NotifyLeakInfo::SetNotifyDir(string notifyDir)
{
    notifyDir_ = notifyDir;
}

string NotifyLeakInfo::GetNotifyFileName() const
{
    return notifyFileName_;
}

void NotifyLeakInfo::SetNotifyFileName(string notifyDir)
{
    notifyFileName_ = notifyDir;
}

string NotifyLeakInfo::GetBigMallocFileName() const
{
    string bigMallocFilePath = "memleak-leakstack-" + processName_ + "-" + to_string(pid_) + "-" +
        leakedTime_ + ".txt";
    return bigMallocFilePath;
}

string NotifyLeakInfo::GetLeakStack() const
{
    return leakStack_;
}

void NotifyLeakInfo::AddLeakStack(string stackLine)
{
    leakStack_ += stackLine + "\n";
}

pid_t NotifyLeakInfo::GetTid() const
{
    return tid_;
}

void NotifyLeakInfo::SetTid(pid_t tid)
{
    tid_ = tid;
}

int NotifyLeakInfo::GetLimitSize() const
{
    return limitSize_;
}

void NotifyLeakInfo::SetLimitSize(int limitSize)
{
    limitSize_ = limitSize;
}

int NotifyLeakInfo::GetLiveObjectSize() const
{
    return liveObjectSize_;
}

void NotifyLeakInfo::SetLiveObjectSize(int liveObjectSize)
{
    liveObjectSize_ = liveObjectSize;
}

string NotifyLeakInfo::GetHeapdumpType() const
{
    return heapdumpType_;
}

void NotifyLeakInfo::SetHeapdumpType(string heapdumpType)
{
    heapdumpType_ = heapdumpType;
}

string NotifyLeakInfo::GetJsLeakFilePath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetJsLeakFileName();
}

string NotifyLeakInfo::GetJsLeakFileZipPath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetJsLeakFileZipName();
}

string NotifyLeakInfo::GetArkWebLeakFileName() const
{
    string rawHeapFileName = "memleak-arkweb_v8-" + processName_ + "-" + to_string(pid_) +
        "-" + leakedTime_ + ".rawheap";
    return rawHeapFileName;
}

string NotifyLeakInfo::GetArkWebLeakFileZipPath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetArkWebLeakFileZipName();
}

std::string NotifyLeakInfo::GetArkWebLeakFileZipName() const
{
    return "memleak-arkweb_v8-" + processName_ + "-" + to_string(pid_) +
        "-" + leakedTime_ + ".zip";
}

string NotifyLeakInfo::GetArkWebLeakFilePath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetArkWebLeakFileName();
}

string NotifyLeakInfo::GetJSVMLeakFileName() const
{
    string rawHeapFileName = "memleak-jsvm-" + processName_ + "-" + to_string(pid_) +
        "-" + leakedTime_ + ".rawheap";
    return rawHeapFileName;
}

string NotifyLeakInfo::GetJSVMLeakFilePath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetJSVMLeakFileName();
}

std::string NotifyLeakInfo::GetJSVMLeakFileZipName() const
{
    return "memleak-jsvm-" + processName_ + "-" + to_string(pid_) +
        "-" + leakedTime_ + ".zip";
}

string NotifyLeakInfo::GetJSVMLeakFileZipPath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetJSVMLeakFileZipName();
}

string NotifyLeakInfo::GetKMPLeakFileName() const
{
    string rawHeapFileName = "memleak-kotlin-" + processName_ + "-" + to_string(pid_) +
        "-" + leakedTime_ + ".rawheap";
    return rawHeapFileName;
}

string NotifyLeakInfo::GetKMPLeakFilePath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetKMPLeakFileName();
}

std::string NotifyLeakInfo::GetKMPLeakFileZipName() const
{
    return "memleak-kotlin-" + processName_ + "-" + to_string(pid_) +
        "-" + leakedTime_ + ".zip";
}

string NotifyLeakInfo::GetKMPLeakFileZipPath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetKMPLeakFileZipName();
}

std::string NotifyLeakInfo::GetLogPathForSysEvent()
{
    string logPath;

    if (GetNotifyType() == JS_LEAK) {
        logPath = GetJsLeakFileZipPath();
        if (HasStaticJsHeap()) {
            logPath += ";" + GetStaticJsLeakFilePath();
        }
    }
    return logPath;
}

string NotifyLeakInfo::GetJsLeakFileName() const
{
    string rawHeapFileName = "memleak-js-" + processName_ + "-" + to_string(pid_) + "-" + to_string(tid_) +
        "-" + leakedTime_ + ".rawheap";
    return rawHeapFileName;
}

// actual beta leak
std::string NotifyLeakInfo::GetJsLeakFileZipName() const
{
    return "memleak-js-" + processName_ + "-" + to_string(pid_) + "-" + to_string(tid_) +
        "-" + leakedTime_ + ".zip";
}

std::string NotifyLeakInfo::GetJsLeakNoTidFileName() const
{
    return "memleak-js-" + processName_ + "-" + to_string(pid_) + ".rawheap";
}

std::string NotifyLeakInfo::GetJsLeakNoTidFilePath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetJsLeakNoTidFileName();
}

std::string NotifyLeakInfo::GetStaticJsLeakFileName() const
{
    return "memleak-js_static-" + processName_ + "-" + to_string(pid_) +
        "-" + leakedTime_ + ".rawheap";
}

std::string NotifyLeakInfo::GetStaticJsLeakFilePath() const
{
    return "/data/log/reliability/resource_leak/memory_leak/" + GetStaticJsLeakFileName();
}

std::string NotifyLeakInfo::GetStaticJsLeakNotifyFileName() const
{
    return staticNotifyFileName_;
}

void NotifyLeakInfo::SetStaticJsLeakNotifyFileName(const std::string &fileName)
{
    staticNotifyFileName_ = fileName;
}

bool NotifyLeakInfo::HasStaticJsHeap() const
{
    return !staticNotifyFileName_.empty();
}

std::string NotifyLeakInfo::GetArktsType() const
{
    return arktsType_;
}

void NotifyLeakInfo::SetArktsType(const std::string &arktsType)
{
    arktsType_ = arktsType;
}

bool NotifyLeakInfo::IsDynamicArkts() const
{
    return arktsType_ == "dynamic";
}

bool NotifyLeakInfo::IsEverReported()
{
    rawheapListMtx_.lock();
    string name = processName_;
    auto it = RawheapDumpList_.find(name);
    if (it == RawheapDumpList_.end()) {
        rawheapListMtx_.unlock();
        return false;
    }
    int64_t lastReportTime = it->second;
    rawheapListMtx_.unlock();
    if (lastReportTime < 0) {
        return false;
    }

    int64_t now = time(nullptr);
    if (now > lastReportTime + SEC_PER_DAY) {
        HIVIEW_LOGI("process: %{public}s should report, last report time is %{public}s", name.c_str(),
            DfrUtil::GetInstance().TimeStampToStr(lastReportTime).c_str());
        return false;
    }
    HIVIEW_LOGI("process: %{public}s should not report, last report time is %{public}s", name.c_str(),
        DfrUtil::GetInstance().TimeStampToStr(lastReportTime).c_str());
    return true;
}

void NotifyLeakInfo::RecordReport()
{
    rawheapListMtx_.lock();
    int64_t now = time(nullptr);
    HIVIEW_LOGI("record process %{public}s is reported at %{public}s", processName_.c_str(),
        DfrUtil::GetInstance().TimeStampToStr(now).c_str());
    RawheapDumpList_[processName_] = now;
    rawheapListMtx_.unlock();
    if (dumpCnt_.load() > 0) {
        dumpCnt_.fetch_sub(1);
        int ret = system::SetParameter("persist.hiview.oomdump.maxcount", to_string(dumpCnt_.load()));
        HIVIEW_LOGI("set current max count callback: %{public}d", ret);
    }
}

void NotifyLeakInfo::ResetHeapDumpCnt(int cnt)
{
    dumpCnt_.store(cnt);
}

uint64_t NotifyLeakInfo::GetTelemetryType()
{
    if (telemetryType_ > 0) {
        return telemetryType_;
    }
    if (GetTelemetry().CouldTelemetry(EXT_PSS_LEAK_TYPE, GetBundleName())) {
        telemetryType_ = EXT_PSS_LEAK_TYPE;
    } else if (GetTelemetry().CouldTelemetry(RSS_LEAK_TYPE, GetBundleName())) {
        telemetryType_ = RSS_LEAK_TYPE;
    } else {
        telemetryType_ = JS_OOM_TYPE;
    }
    return telemetryType_;
}

struct EventQuota NotifyLeakInfo::GetEventQuota()
{
    uint64_t telemetryType = GetTelemetryType();
    return GetTelemetry().GetEventQuota(telemetryType, GetBundleName());
}

bool NotifyLeakInfo::CouldTelemetry()
{
    uint64_t telemetryType = GetTelemetryType();
    return GetTelemetry().CouldTelemetry(telemetryType, GetBundleName());
}

bool NotifyLeakInfo::CommonCompress()
{
    uint64_t telemetryType = GetTelemetryType();
    return NotifyLeakCompressManager::GetInstance().CommonCompress(telemetryType, GetBundleName());
}

void NotifyLeakInfo::ConsumeQuota()
{
    uint64_t telemetryType = GetTelemetryType();
    GetTelemetry().ConsumeQuota(telemetryType, GetBundleName());
}

void NotifyLeakInfo::SetCommpressStatus(NotifyCompressStatus compressStatus)
{
    compressStatus_ = compressStatus;
}

NotifyCompressStatus NotifyLeakInfo::GetCommpressStatus()
{
    return compressStatus_;
}

bool NotifyLeakInfo::ShouldPostEvent()
{
    bool ret = (compressStatus_ == NEED_COMPRESS_AND_NOT_COMPRESSED || compressStatus_ == NO_COMPRESS);
    ret = (ret && (eventConfig_ != "event")); // has posted app event earlier
    HIVIEW_LOGI("ret: %{public}d, compressStatus_: %{public}d, eventConfig_: %{public}s",
                ret, compressStatus_, eventConfig_.c_str());
    return ret;
}

bool NotifyLeakInfo::ShouldSendHisysevent()
{
    bool ret = (compressStatus_ == NEED_COMPRESS_AND_COMPRESSED || compressStatus_ == NO_COMPRESS);
    HIVIEW_LOGI("ret: %{public}d, compressStatus_: %{public}d", ret, compressStatus_);
    return ret;
}

void NotifyLeakInfo::SetEventConfig(const std::string &eventConfig)
{
    eventConfig_ = eventConfig;
}

std::string NotifyLeakInfo::GetEventConfig()
{
    return eventConfig_;
}

bool NotifyLeakInfo::IsAppWantEvent()
{
    return eventConfig_ == "event_rawheap";
}

void NotifyLeakInfo::SetOnlyAppreportFlag(bool flag)
{
    onlyAppreportFlag_ = flag;
}

bool NotifyLeakInfo::GetOnlyAppreportFlag()
{
    return onlyAppreportFlag_;
}

string NotifyLeakInfo::GetProfilerPath() const
{
    return "";
}

string NotifyLeakInfo::GetProfilerName() const
{
    return "";
}

uint64_t NotifyLeakInfo::GetFaultType() const
{
    if (GetHeapdumpType() == OOM_TYPE) {
        return JS_OOM_TYPE;
    }
    if (GetHeapdumpType() == "RSS_SYSTEM_TELEMETRY") {
        return RSS_LEAK_TYPE;
    }
    if (GetHeapdumpType() == "EXT_PSS_SYSTEM_TELEMETRY") {
        return EXT_PSS_LEAK_TYPE;
    }
    return 0;
}
} // namespace HiviewDFX
} // namespace OHOS
