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

#include "process_kill_event_handler.h"
#include <fcntl.h>
#include <fstream>
#include <mutex>
#include <regex>
#include <string>
#include <unistd.h>
#include "bundle_mgr_client.h"
#include "fault_info_base.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "hitrace_meter.h"
#include "dfr_util.h"
#include "old_log_cleaner.h"
#include "ashmem_kill_leak_info.h"
#include "gpu_kill_leak_info.h"
#include "native_leak_detector.h"
#include "thread_leak_detector.h"
#include "dma_kill_leak_info.h"
#include "native_leak_detector.h"
#include "rss_kill_leak_info.h"
#include "kernel_thread_kill_leak_info.h"
#include "stability_check_kill_leak_info.h"

namespace OHOS {
namespace HiviewDFX {
using namespace std;
DEFINE_LOG_LABEL(0xD002DA0, "ProcessKillEventHandler");
namespace {
constexpr int MAX_WAIT_TIME_THRESHOLDKILL = 1000;    //ms
constexpr int INTERVAL_THRESHOLDKILL = 100;   //ms
constexpr int US_PER_MS = 1000;
}

ProcessKillLeakInfo::ProcessKillLeakInfo(const ProcesssKillEventParam &eventParam)
    : LeakEventInfoBase(eventParam.eventName)
{
    SetLeakedTime();
    pid_ = eventParam.pid;
    uid_ = eventParam.uid;
    adj_ = eventParam.adj;
    processName_ = eventParam.processName;
    detailMsg_ = eventParam.msg;
    killReason_ = eventParam.reason;
    appRunningId_ = eventParam.appRunningId;
}

void ProcessKillLeakInfo::SetPid(pid_t pid)
{
    pid_ = pid;
}

pid_t ProcessKillLeakInfo::GetPid()
{
    return pid_;
}

void ProcessKillLeakInfo::SetUid(pid_t uid)
{
    uid_ = uid;
}

pid_t ProcessKillLeakInfo::GetUid()
{
    return uid_;
}

void ProcessKillLeakInfo::SetProcessName(string processName)
{
    processName_ = processName;
}

string ProcessKillLeakInfo::GetProcessName()
{
    return processName_;
}

void ProcessKillLeakInfo::SetKillReason(string killReason)
{
    killReason_ = killReason;
}

string ProcessKillLeakInfo::GetKillReason()
{
    return killReason_;
}

void ProcessKillLeakInfo::SetDetailMsg(string detailMsg)
{
    detailMsg_ = detailMsg;
}

string ProcessKillLeakInfo::GetDetailMsg()
{
    return detailMsg_;
}

void ProcessKillLeakInfo::SetCmdType(uint32_t cmdType)
{
    cmdType_ = cmdType;
}

uint32_t ProcessKillLeakInfo::GetCmdType()
{
    return cmdType_;
}

void ProcessKillLeakInfo::SetInfoType(uint32_t infoType)
{
    infoType_ = infoType;
}

uint32_t ProcessKillLeakInfo::GetInfoType()
{
    return infoType_;
}

void ProcessKillLeakInfo::SetAppRunningId(string appRunningId)
{
    appRunningId_ = appRunningId;
}

string ProcessKillLeakInfo::GetAppRunningId()
{
    return appRunningId_;
}

string ProcessKillLeakInfo::GetFileName()
{
    return "/memleak-kernel-" + processName_ + "-0-" + leakedTime_ + ".txt";
}

bool ProcessKillLeakInfo::IsDumpMessage()
{
    return true;
}

string ProcessKillLeakInfo::GetApplicationVersion()
{
    if (version_ == "unknown") {
        version_ = DfrUtil::GetInstance().GetApplicationVersionByUid(uid_);
    }
    return version_;
}

void ProcessKillLeakInfo::DumpDetailInfo(ofstream &fout, const string &detailInfo, const string &logType)
{
    DumpGeneralInfo(fout);
    fout << "*****************************" << endl;
    fout << logType << endl;
    fout << detailInfo << endl;
    fout << "************ endl ************" << endl;
}

shared_ptr<LeakEventInfoBase> ProcessKillEventHandler::HandleMessage(SysEvent sysEvent)
{
    if (sysEvent.GetEventIntValue("UID") < MIN_APP_USERID) {
        //judge is app
        return nullptr;
    }
    shared_ptr<LeakEventInfoBase> eventInfo = nullptr;
    ProcesssKillEventParam eventParam {
        sysEvent.GetEventIntValue("PID"),
        sysEvent.GetEventIntValue("UID"),
        sysEvent.GetEventIntValue("ADJ"),
        sysEvent.GetEventValue("REASON"), // eventName
        sysEvent.GetEventValue("PROCESS_NAME"),
        sysEvent.GetEventValue("MSG"),
        sysEvent.GetEventValue("INNER_MSG"),
        sysEvent.GetEventValue("REASON"),
        sysEvent.GetEventValue("APP_RUNNING_UNIQUE_ID"),
        sysEvent.GetEventIntValue("EVENT_PARAM_FIRST"),
        sysEvent.GetEventIntValue("EVENT_PARAM_SECOND")
    };
    if (sysEvent.GetEventValue("REASON") == "ASHMEM_THRESHOLD_KILLER") {
        eventInfo = make_shared<AshmemKillLeakInfo>(eventParam);
    } else if (sysEvent.GetEventValue("REASON") == "DMA_THRESHOLD_KILLER") {
        eventInfo = make_shared<DmaKillLeakInfo>(eventParam);
    } else if (sysEvent.GetEventValue("REASON") == "GPU_THRESHOLD_KILLER") {
        eventInfo = make_shared<GpuKillLeakInfo>(eventParam);
    } else if (sysEvent.GetEventValue("REASON") == "RSS_THRESHOLD_KILLER") {
        eventInfo = make_shared<RssKillLeakInfo>(eventParam);
    } else if (sysEvent.GetEventValue("REASON") == "StabilityCheckKill") {
        if (eventParam.innerMsg.empty() || sysEvent.GetEventIntValue("FOREGROUND") == 0) {
            HIVIEW_LOGW("StabilityCheckKill innerMsg is empty or Backgroud kill, skip");
            return nullptr;
        }
        eventInfo = make_shared<StabilityCheckKillLeakInfo>(eventParam);
    } else if (sysEvent.GetEventValue("REASON") == "THREAD_THRESHOLD_KILLER") {
        string msg = sysEvent.GetEventValue("MSG");
        int64_t threadCount = sysEvent.GetEventIntValue("THREAD_COUNT");
        GetThreadInfo(eventParam.pid, threadCount, msg);    // get info from event KILL_EXCESSIVE_PROCESS
        eventInfo = make_shared<KernelThreadKillLeakInfo>(
            eventParam, threadCount, sysEvent.GetEventIntValue("FOREGROUND"),
            sysEvent.GetEventIntValue("time_"), msg);
    }
    if (eventInfo == nullptr) {
        return nullptr;
    }
    HIVIEW_LOGI("HandleMessage create processKillInfo succeed event : %{public}s",
        sysEvent.GetEventValue("REASON").c_str());
    return eventInfo;
}

ErrCode ProcessKillEventHandler::DumpMessage(shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto processKillInfo = static_pointer_cast<ProcessKillLeakInfo>(eventInfo);
    if (!processKillInfo->IsDumpMessage()) {
        return SUCCESSED;
    }
    std::string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    const string &filePath = processKillInfo->GetFilePath() + processKillInfo->GetFileName();
    const string &realPath = DfrUtil::GetInstance().RealPath(filePath);
    if (realPath.empty()) {
        return FAILURE;
    }
    HIVIEW_LOGD("realpath, path: %{public}s", filePath.c_str());
    std::ofstream fout;
    FileParams params;
    params.procName = processKillInfo->GetProcessName();
    params.fileName = processKillInfo->GetFileName();
    params.logPath = realPath;
    params.foutMode = ios::out | ios::app;
    OldLogCleaner::GetInstance().CreateFile(params, fout);
    if (!fout.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s", filePath.c_str());
        return FAILURE;
    }
    string detailInfo;
    if (processKillInfo->GetEventName() == "RSS_THRESHOLD_KILLER") {
        auto rssKillInfo = static_pointer_cast<RssKillLeakInfo>(processKillInfo);
        DfrUtil::GetInstance().DumpRssKillerSmaps(rssKillInfo->GetPid(), detailInfo);
        if (!detailInfo.empty()) {
            // handle detail info and compute rssmemory
            string resultInfo = rssKillInfo->HandleDetailInfo(detailInfo);
            processKillInfo->DumpDetailInfo(fout, resultInfo, "LOGGER_MEMCHECK_SAMPLIFY_SMAPS_INFO");
        }
    } else if (processKillInfo->GetKillReason() == "StabilityCheckKill") {
        auto stabilityCheckKillInfo = static_pointer_cast<StabilityCheckKillLeakInfo>(processKillInfo);
        stabilityCheckKillInfo->DumpIoctlData(fout);
    } else {
        detailInfo = DfrUtil::GetInstance().SendLeakCmdInfo(processKillInfo->GetPid(),
            processKillInfo->GetCmdType(), processKillInfo->GetInfoType());
        if (!detailInfo.empty()) {
            processKillInfo->HandleSubeventInfo(detailInfo);
            processKillInfo->DumpDetailInfo(fout, detailInfo, "LOGGER_MEMCHECK_PROC_INFO");
        }
    }
    DfrUtil::GetInstance().DumpAppPageInfo(processKillInfo->GetPid(), processKillInfo->GetProcessName(), fout);
    fout.close();
    if (DfrUtil::GetInstance().IsFileEmpty(filePath)) {
        DfrUtil::GetInstance().RemoveFile(filePath);
    }
    return SUCCESSED;
}

ErrCode ProcessKillEventHandler::ReportMessage(shared_ptr<LeakEventInfoBase> eventInfo)
{
    if (eventInfo == nullptr) {
        HIVIEW_LOGE("ReportMessage eventInfo is null");
        return FAILURE;
    }
    auto processKillInfo = static_pointer_cast<ProcessKillLeakInfo>(eventInfo);
    processKillInfo->SysEventReport();
    return SUCCESSED;
}

ErrCode ProcessKillEventHandler::AppEventReport(shared_ptr<LeakEventInfoBase> eventInfo)
{
    if (eventInfo == nullptr) {
        HIVIEW_LOGE("AppEventReport eventInfo is null");
        return FAILURE;
    }
    auto processKillInfo = static_pointer_cast<ProcessKillLeakInfo>(eventInfo);
    processKillInfo->AppEventReport();
    return SUCCESSED;
}

ErrCode ProcessKillEventHandler::SubEventReport(shared_ptr<LeakEventInfoBase> eventInfo)
{
    if (eventInfo == nullptr) {
        HIVIEW_LOGE("SubEventReport eventInfo is null");
        return FAILURE;
    }
    auto processKillInfo = static_pointer_cast<ProcessKillLeakInfo>(eventInfo);
    processKillInfo->SubSysEventReport();
    return SUCCESSED;
}

ErrCode ProcessKillEventHandler::HandleEvent(SysEvent sysEvent)
{
    auto eventInfo = HandleMessage(sysEvent);
    if (eventInfo == nullptr) {
        HIVIEW_LOGE("HandleMessage failed. eventName: %{public}s", sysEvent.eventName_.c_str());
        return FAILURE;
    }
    if (DumpMessage(eventInfo) == FAILURE) {
        HIVIEW_LOGE("Dump Event Message failed. eventName: %{public}s", eventInfo->GetEventName().c_str());
        return FAILURE;
    }
    if (ReportMessage(eventInfo) == FAILURE) {
        HIVIEW_LOGE("Report Event Message failed. eventName: %{public}s", eventInfo->GetEventName().c_str());
        return FAILURE;
    }
    if (SubEventReport(eventInfo) == FAILURE) {
        HIVIEW_LOGE("Report SubEvent Message failed. eventName: %{public}s", eventInfo->GetEventName().c_str());
        return FAILURE;
    }
    if (AppEventReport(eventInfo) == FAILURE) {
        HIVIEW_LOGE("AppEventReport failed. eventName: %{public}s", eventInfo->GetEventName().c_str());
        return FAILURE;
    }
    return SUCCESSED;
}

void ProcessKillEventHandler::GetThreadInfo(pid_t pid, int64_t &threadCount, string &msg)
{
    if (msg.empty()) {
        int waitedMs = 0;
        while (waitedMs < MAX_WAIT_TIME_THRESHOLDKILL) {
            auto cachedData = ThreadLeakDetector::GetInstance().GetCachedKernelThreadMsg(pid);
            if (!cachedData.second.empty()) {
                threadCount = cachedData.first;
                msg = cachedData.second;
                break;
            }
            usleep(INTERVAL_THRESHOLDKILL * US_PER_MS);
            waitedMs += INTERVAL_THRESHOLDKILL;
        }
    }
}
} // HiviewDFX
} // OHOS

