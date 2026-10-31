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
#include "notify_leak_detector.h"
#include "old_log_cleaner.h"

#include <cinttypes>

#include "fault_common.h"
#include "hiview_logger.h"
#include <parameters.h>

using namespace std;

enum TypePosIndex {
    POS_INDEX_NAME = 0,
    POS_INDEX_PID = 1,
    POS_INDEX_TID,
};

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr size_t JSHEAP_SIZE = 4;
constexpr size_t DYNAMIC_JSHEAP_SIZE = 3;
constexpr size_t STATIC_JSHEAP_SIZE = 3;
constexpr size_t KMPHEAP_SIZE = 3;
constexpr size_t LEAKSTACK_SIZE = 3;
constexpr int INDEX_NAME = 0;
constexpr int INDEX_PID = 1;
constexpr int INDEX_JSHEAP_TID = 2;
constexpr int INDEX_JSHEAP_SUFFIX = 3;
// jsheap-pid-timestamp.rawheap: 3 segments when split by '-'
constexpr size_t JSHEAP_NO_TID_SEGMENTS = 3;
constexpr int OLD_CLEAN_DELAY = 70 * US_PER_SECOND; // must exceed DumpState polling time for static_jsheap
constexpr int REACH_THRESHOLD_DELAY = 30 * US_PER_SECOND;
constexpr int WAIT_RAWHEAP_DELAY = 30 * US_PER_SECOND;
constexpr int TIME_FOR_WAIT_EVENT = 3 * US_PER_SECOND;
constexpr int MAX_RETRY_CNT = 3;

const string JSHEAP_PREFIX = "jsheap";
const string HYBRID_JSHEAP_PREFIX = "hybrid_jsheap";
const string STATIC_JSHEAP_PREFIX = "static_jsheap";
const string RAWHEAP_SUFFIX = "rawheap";
const string CJHEAP_PREFIX = "cjheap";
const string LEAKSTACK_PREFIX = "leakstack";
const string LEAKLIST_PREFIX = "leaklist";
const string NATIVEHEAP_PREFIX = "nativeheap";
const string KMPHEAP_PREFIX = "kotlinheap";
const string JSVMHEAP_PREFIX = "jsvmheap";
const string ARKWEBJS_HEAP_PREFIX = "arkweb_jsheap";
const string REACH_THRESHOLD_TYPE = "thresholdReachedDump";

constexpr int MAIN_INTERVAL = SEC_PER_HOUR; // one hour
constexpr int MAIN_INTERVAL_TEST = 10;
constexpr int QUOTA_UPDATE_INTERVAL = 7 * SEC_PER_DAY;
constexpr int MAX_DUMP_CNT_PER_WEEK = 5;
constexpr int DUMP_CNT_PER_PROCESS_PER_WEEK = 1;
constexpr int MAX_HIDUMP_CNT_PER_WEEK = 5;
constexpr int HIDUMP_CNT_PER_PROCESS_PER_WEEK = 1;
}

DEFINE_LOG_LABEL(0xD002DA0, "NotifyLeakDetector");

NotifyLeakDetector::NotifyLeakDetector() {}

NotifyLeakDetector::~NotifyLeakDetector() {}

ErrCode NotifyLeakDetector::MainProcess()
{
    return SUCCESSED;
}

void NotifyLeakDetector::HandleDetector(int interval)
{
    if (!ShouldDoMainProcess(interval)) {
        return;
    }
    OldLogCleaner::GetInstance().CheckAndAgingRawheapFiles();
    string lastTimeStr = system::GetParameter("persist.hiview.oomdump.lasttime", "");
    if (lastTimeStr.empty()) {
        HIVIEW_LOGI("there is no parameter for omm, create");
        ResetDumpCntAndTime();
        return;
    }
    time_t lastTime = DfrUtil::GetInstance().StrToTimeStamp(lastTimeStr);
    time_t currentTime = time(nullptr);
    if (currentTime -  lastTime > QUOTA_UPDATE_INTERVAL) {
        HIVIEW_LOGI("there is over one week for omm, reset parameter");
        ResetDumpCntAndTime();
    }
}

void NotifyLeakDetector::ResetDumpCntAndTime()
{
    // last_time_stamp
    system::SetParameter("persist.hiview.oomdump.lasttime", DfrUtil::GetInstance().GetRealTime());
    //  dump_cnt_per_week
    system::SetParameter("persist.hiview.oomdump.maxcount", to_string(MAX_DUMP_CNT_PER_WEEK));
    //  dump_cnt_per_process_per_week
    system::SetParameter("persist.hiview.oomdump.process.maxcount", to_string(DUMP_CNT_PER_PROCESS_PER_WEEK));
    NotifyLeakInfo::ResetHeapDumpCnt(MAX_DUMP_CNT_PER_WEEK); // info::Cnt

    /// hidebugdump
    //  dump_cnt_per_week
    system::SetParameter("persist.hiview.hidebugdump.maxcount", to_string(MAX_HIDUMP_CNT_PER_WEEK));
    hidumpCnt_.store(MAX_HIDUMP_CNT_PER_WEEK);
    //  dump_cnt_per_process_per_week
    system::SetParameter("persist.hiview.hidebugdump.process.maxcount", to_string(HIDUMP_CNT_PER_PROCESS_PER_WEEK));
}

bool NotifyLeakDetector::ShouldDoMainProcess(int interval)
{
    if (mainInterval_ <= 0) {
        mainInterval_ = DfrUtil::GetInstance().IsMemTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL;
        return true;
    }
    mainInterval_ -= interval;
    return false;
}

bool NotifyLeakDetector::ParseKmpHeapType(const vector<string> &splitedNames, NotifyType &type, pid_t &pid, pid_t &tid)
{
    if (splitedNames.size() < KMPHEAP_SIZE) {
        HIVIEW_LOGE("kotlinheap filename format error, filesize: %{public}zu", splitedNames.size());
        return false;
    }
    const auto &pidStr = splitedNames[POS_INDEX_PID];
    if (!DfrUtil::GetInstance().SafeStoi(pidStr, pid)) {
        HIVIEW_LOGE("safestoi failed when get pid for kotlinheap");
        return false;
    }
    tid = pid;  // for kmp, tid equals pid
    type = KMP_LEAK;
    HIVIEW_LOGI("kotlinheap file pid: %{public}d tid: %{public}d", pid, tid);
    return true;
}

bool NotifyLeakDetector::ParseJsHeapType(const string &typeName, const vector<string> &splitedNames,
                                         NotifyType &type, pid_t &pid, pid_t &tid)
{
    if (splitedNames.size() < DYNAMIC_JSHEAP_SIZE) {
        HIVIEW_LOGE("%{public}s filename format error", typeName.c_str());
        return false;
    }
    const auto &pidStr = splitedNames[POS_INDEX_PID];
    if (!DfrUtil::GetInstance().SafeStoi(pidStr, pid)) {
        HIVIEW_LOGE("safestoi failed when get pid for %{public}s", typeName.c_str());
        return false;
    }
    if (splitedNames.size() >= JSHEAP_SIZE) {
        // jsheap-pid-tid-timestamp.rawheap (4 segments)
        const auto &tidStr = typeName == JSHEAP_PREFIX ? splitedNames[POS_INDEX_TID] : splitedNames[POS_INDEX_PID];
        if (!DfrUtil::GetInstance().SafeStoi(tidStr, tid)) {
            HIVIEW_LOGE("safestoi failed when get tid for %{public}s", typeName.c_str());
            return false;
        }
    } else {
        // jsheap-pid-timestamp.rawheap (3 segments, no tid)
        tid = 0;
    }
    type = (typeName == JSHEAP_PREFIX) ? JS_LEAK
        : (typeName == ARKWEBJS_HEAP_PREFIX ? ARKWEB_LEAK : JSVM_LEAK);
    HIVIEW_LOGI("%{public}s file pid: %{public}d tid: %{public}d", typeName.c_str(), pid, tid);
    return true;
}

bool NotifyLeakDetector::ParseNotifyType(const string &fileName, NotifyType &type, pid_t &pid, pid_t &tid)
{
    HIVIEW_LOGD("new notify file: %{public}s", fileName.c_str());
    vector<string> splitedNames = DfrUtil::GetInstance().Split(fileName, '-');
    const string &typeName = splitedNames[0];

    if (typeName == KMPHEAP_PREFIX) {
        return ParseKmpHeapType(splitedNames, type, pid, tid);
    }

    if (typeName == STATIC_JSHEAP_PREFIX) {
        // static_jsheap-{pid}-{timestamp}.rawheap, only 3 parts, no tid
        if (splitedNames.size() < STATIC_JSHEAP_SIZE) {
            HIVIEW_LOGE("static_jsheap filename format error, filename: %{public}s", fileName.c_str());
            return false;
        }
        if (!DfrUtil::GetInstance().SafeStoi(splitedNames[POS_INDEX_PID], pid)) {
            HIVIEW_LOGE("safestoi failed when get pid for static_jsheap");
            return false;
        }
        tid = 0; // no tid in static_jsheap filename
        type = JS_LEAK;
        HIVIEW_LOGI("static_jsheap file pid: %{public}d tid: %{public}d", pid, tid);
        return true;
    }

    if (typeName == JSHEAP_PREFIX || typeName == ARKWEBJS_HEAP_PREFIX || typeName == JSVMHEAP_PREFIX) {
        return ParseJsHeapType(typeName, splitedNames, type, pid, tid);
    }

    if (typeName == LEAKSTACK_PREFIX) {
        if (splitedNames.size() < LEAKSTACK_SIZE) {
            HIVIEW_LOGE("LEAK_STACK filename format error, filename: %{public}s", fileName.c_str());
            return false;
        }
        const auto &pidStr = splitedNames[POS_INDEX_PID];
        if (!DfrUtil::GetInstance().SafeStoi(pidStr, pid)) {
            HIVIEW_LOGE("safestoi failed when get pid, filename: %{public}s", fileName.c_str());
            return false;
        }

        type = LEAK_STACK;
        HIVIEW_LOGI("leakstack file pid: %{public}d tid: %{public}d", pid, tid);
        return true;
    }

    HIVIEW_LOGE("unknonw filename, filename: %{public}s", fileName.c_str());
    return false;
}

string NotifyLeakDetector::GetProcessInfoKey(pid_t pid, pid_t tid)
{
    return to_string(pid) + " " + to_string(tid);
}

bool NotifyLeakDetector::IsJsHeapNoTidFile(const string &filename)
{
    if (filename.length() < JSHEAP_PREFIX.length() ||
        filename.substr(0, JSHEAP_PREFIX.length()) != JSHEAP_PREFIX) {
        return false;
    }
    // jsheap-pid-timestamp.rawheap has 3 segments when split by '-'
    // jsheap-pid-tid-timestamp.rawheap has 4 segments
    vector<string> parts = DfrUtil::GetInstance().Split(filename, '-');
    return parts.size() == JSHEAP_NO_TID_SEGMENTS;
}

bool NotifyLeakDetector::CopyJsHeapNoTidFile(pid_t pid, const string &notifyDir,
                                             const string &filename)
{
    string oldPath = notifyDir + "/" + filename;
    if (DfrUtil::GetInstance().IsFileEmpty(oldPath)) {
        HIVIEW_LOGW("jsheap no-tid copy: source file is empty");
        return false;
    }
    string processName = DfrUtil::GetInstance().GetProcessName(pid);
    auto notifyInfo = make_shared<NotifyLeakInfo>(pid, processName);
    notifyInfo->SetLeakedTime();
    string newPath = notifyInfo->GetJsLeakNoTidFilePath();
    if (OldLogCleaner::GetInstance().CopyFile(oldPath, newPath, processName, notifyInfo->GetJsLeakNoTidFileName())) {
        HIVIEW_LOGI("jsheap no-tid copy success, dst: %{public}s", newPath.c_str());
    } else {
        HIVIEW_LOGW("jsheap no-tid copy failed");
    }
    DfrUtil::GetInstance().RemoveFile(oldPath);
    return true;
}

void NotifyLeakDetector::HandleJSLeak(pid_t pid, pid_t tid, const string &notifyDir, const string &filename,
                                      int retryCnt)
{
    if (IsStaticJsHeapFile(filename)) {
        HandleStaticJsHeap(pid, tid, notifyDir, filename, retryCnt);
        return;
    }

    // jsheap-pid-timestamp.rawheap: copy to memleak-js-packagename-pid.rawheap
    if (IsJsHeapNoTidFile(filename)) {
        CopyJsHeapNoTidFile(pid, notifyDir, filename);
        return;
    }

    // jsheap: match by pid+tid key
    string processInfoKey = GetProcessInfoKey(pid, tid);
    if (processInfoList_.find(processInfoKey) == processInfoList_.end()) {
        HandleJSLeakRetry(pid, tid, notifyDir, filename, retryCnt);
        return;
    }
    HandleJSLeakProcess(pid, tid, notifyDir, filename, processInfoKey);
}

void NotifyLeakDetector::HandleJSLeakRetry(pid_t pid, pid_t tid, const string &notifyDir,
                                           const string &filename, int retryCnt)
{
    if (retryCnt < MAX_RETRY_CNT) {
        ffrt::submit([=] {
            std::lock_guard<ffrt::mutex> lock(notifyDetectorMtx_);
            HandleJSLeak(pid, tid, notifyDir, filename, retryCnt + 1);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyProcessCreate").delay(TIME_FOR_WAIT_EVENT)});
        HIVIEW_LOGW("%{public}s have not got hisysevent, continue waiting, retryCnt: %{public}d",
            filename.c_str(), retryCnt);
        return;
    }
    HIVIEW_LOGE("%{public}s have not got hisysevent, maybe send hisysevent too many times",
        filename.c_str());
}

void NotifyLeakDetector::HandleJSLeakProcess(pid_t pid, pid_t tid, const string &notifyDir,
                                             const string &filename, const string &processInfoKey)
{
    auto &infos = processInfoList_[processInfoKey];
    int targetIndex = -1;
    for (size_t i = 0; i < infos.size(); i++) {
        if (infos[i].rawheapFile.empty()) {
            infos[i].rawheapFile = filename;
            infos[i].hasGotRawheap = true;
            targetIndex = static_cast<int>(i);
            break;
        }
    }
    if (targetIndex == -1) {
        HIVIEW_LOGE("JSLEAK %{public}s lost some sysevent, maybe send hisysevent too many times",
            filename.c_str());
        return;
    }

    int waitTime;
    if (infos[targetIndex].heapdumpType == OOM_TYPE || infos[targetIndex].heapdumpType == "EXT_PSS_SYSTEM_TELEMETRY" ||
        infos[targetIndex].heapdumpType == "RSS_SYSTEM_TELEMETRY") {
        waitTime = 0;
    } else if (infos[targetIndex].heapdumpType == REACH_THRESHOLD_TYPE) {
        waitTime = REACH_THRESHOLD_DELAY;
    } else {
        HIVIEW_LOGE("JSLEAK %{public}s unknown heapdumpType", filename.c_str());
        return;
    }
    HIVIEW_LOGI("JSLEAK %{public}s will dump after %{public}d milliseconds", filename.c_str(), waitTime);

    auto notifyInfo = MakeNotifyInfo(pid, tid, notifyDir, filename, infos[targetIndex]);
    DumpLeak(notifyInfo, waitTime);
}

void NotifyLeakDetector::HandleStaticJsHeap(pid_t pid, pid_t tid, const string &notifyDir,
                                            const string &filename, int retryCnt)
{
    // static_jsheap has no tid in filename, need to search by PID
    int targetIndex = -1;
    for (auto &entry : processInfoList_) {
        auto &infos = entry.second;
        for (size_t i = 0; i < infos.size(); i++) {
            if (infos[i].pid == pid && infos[i].staticRawheapFile.empty()) {
                infos[i].staticRawheapFile = filename;
                infos[i].hasGotStaticRawheap = true;
                targetIndex = static_cast<int>(i);
                break;
            }
        }
        if (targetIndex != -1) {
            break;
        }
    }
    if (targetIndex == -1) {
        if (retryCnt < MAX_RETRY_CNT) {
            ffrt::submit([=] {
                notifyDetectorMtx_.lock();
                HandleJSLeak(pid, tid, notifyDir, filename, retryCnt + 1);
                notifyDetectorMtx_.unlock();
                }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyProcessCreate").delay(TIME_FOR_WAIT_EVENT)});
            HIVIEW_LOGW("static_jsheap %{public}s have not got hisysevent, continue waiting, retryCnt: %{public}d",
                filename.c_str(), retryCnt);
            return;
        }
        // late arrival: jsheap pipeline already finished, copy static_jsheap directly
        HIVIEW_LOGI("static_jsheap %{public}s arrived late, copying independently", filename.c_str());
        CopyStaticJsHeapLate(pid, notifyDir, filename);
        return;
    }
    HIVIEW_LOGI("JSLEAK static_jsheap %{public}s stored, waiting for jsheap to trigger dump", filename.c_str());
}

shared_ptr<NotifyLeakInfo> NotifyLeakDetector::MakeNotifyInfo(pid_t pid, pid_t tid, const string &notifyDir,
                                                              const string &filename, const NotifyProcessInfo &info)
{
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(GetJSLeakNotifyInfo(pid, info));
    vector<string> splitedNames = DfrUtil::GetInstance().Split(filename, '-');
    const string &typeName = splitedNames[0];
    const std::map<std::string, NotifyType> typeMap = {
        {JSHEAP_PREFIX, JS_LEAK},
        {ARKWEBJS_HEAP_PREFIX, ARKWEB_LEAK},
        {JSVMHEAP_PREFIX, JSVM_LEAK},
        {KMPHEAP_PREFIX, KMP_LEAK}
    };
    auto it = typeMap.find(typeName);
    if (it != typeMap.end()) {
        notifyInfo->SetNotifyType(it->second);
    }
    notifyInfo->SetNotifyDir(notifyDir);
    notifyInfo->SetNotifyFileName(filename);
    notifyInfo->SetTid(tid);
    notifyInfo->GetHapVersion();
    notifyInfo->SetLeakedTime();
    notifyInfo->SetAppRunningId(info.appRunningId);
    notifyInfo->SetEventConfig(info.eventConfig);
    notifyInfo->SetUidDirect(info.uid);
    notifyInfo->SetHeapdumpType(info.heapdumpType);
    notifyInfo->SetArktsType(info.arktsType);
    if (!info.staticRawheapFile.empty()) {
        notifyInfo->SetStaticJsLeakNotifyFileName(info.staticRawheapFile);
    }

    return notifyInfo;
}

void NotifyLeakDetector::DumpLeak(shared_ptr<NotifyLeakInfo> notifyInfo, int waitTime)
{
    ffrt::submit([=] {
        string key = GetProcessInfoKey(notifyInfo->GetPid(), notifyInfo->GetTid());
        notifyDetectorMtx_.lock();
        if (processInfoList_.count(key) == 0) {
            notifyDetectorMtx_.unlock();
            HIVIEW_LOGI("%{public}s:%{public}d:%{public}d no need to dump file: %{public}s",
                notifyInfo->GetProcessName().c_str(), notifyInfo->GetPid(), notifyInfo->GetTid(),
                notifyInfo->GetNotifyFileName().c_str());
            string src = notifyInfo->GetNotifyDir() + "/" + notifyInfo->GetNotifyFileName();
            if (!DfrUtil::GetInstance().RemoveFile(src)) {
                HIVIEW_LOGW("failed to remove file: %{public}s", src.c_str());
            }
            return;
        }
        // Re-read static_jsheap info that may have arrived between MakeNotifyInfo and now
        auto &infos = processInfoList_[key];
        for (size_t i = 0; i < infos.size(); i++) {
            if (!infos[i].staticRawheapFile.empty() && !notifyInfo->HasStaticJsHeap()) {
                notifyInfo->SetStaticJsLeakNotifyFileName(infos[i].staticRawheapFile);
                HIVIEW_LOGI("static_jsheap %{public}s found in processInfoList_ at dump time",
                    infos[i].staticRawheapFile.c_str());
                break;
            }
        }
        notifyDetectorMtx_.unlock();
        ExeNextStateProcess(notifyInfo, PROC_DUMP_STATE);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyProcessCreate").delay(waitTime)});
}

void NotifyLeakDetector::HandleThirdPartyLeak(pid_t pid, pid_t tid, const string &notifyDir, const string &filename,
    int retryCnt)
{
    string processInfoKey = GetProcessInfoKey(pid, tid);
    if (processInfoList_.find(processInfoKey) == processInfoList_.end()) {
        if (retryCnt < MAX_RETRY_CNT) {
            ffrt::submit([=] {
                std::lock_guard<ffrt::mutex> lock(notifyDetectorMtx_);
                HandleThirdPartyLeak(pid, tid, notifyDir, filename, retryCnt + 1);
                }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyProcessCreate").delay(TIME_FOR_WAIT_EVENT)});
            HIVIEW_LOGW("%{public}s have not got hisysevent, continue waiting, retryCnt: %{public}d",
                filename.c_str(), retryCnt);
            return;
        }
        HIVIEW_LOGE("%{public}s have not got hisysevent, maybe send hisysevent too many times", filename.c_str());
        return;
    }
    auto &infos = processInfoList_[processInfoKey];
    int targetIndex = -1;
    for (size_t i = 0; i < infos.size(); i++) {
        if (infos[i].rawheapFile.empty()) {
            infos[i].rawheapFile = filename;
            infos[i].hasGotRawheap = true;
            targetIndex = static_cast<int>(i);
            break;
        }
    }
    if (targetIndex == -1) {
        HIVIEW_LOGE("%{public}s lost some sysevent, maybe send hisysevent too many times", filename.c_str());
        return;
    }

    int waitTime;
    if (infos[targetIndex].heapdumpType == "EXT_PSS_SYSTEM_TELEMETRY" ||
        infos[targetIndex].heapdumpType == "RSS_SYSTEM_TELEMETRY") {
        waitTime = 0;
    } else {
        HIVIEW_LOGE("%{public}s unknown heapdumpType", filename.c_str());
        return;
    }
    HIVIEW_LOGI("%{public}s will dump after %{public}d milliseconds", filename.c_str(), waitTime);

    auto notifyInfo = MakeNotifyInfo(pid, tid, notifyDir, filename, infos[targetIndex]);
    DumpLeak(notifyInfo, waitTime);
}

bool NotifyLeakDetector::GetNotifyState(string notifyDir, string fileName, pid_t &pid)
{
    NotifyType type;
    pid_t tid = 0;
    if (!ParseNotifyType(fileName, type, pid, tid)) {
        HIVIEW_LOGE("ParseNotifyType failed");
        return false;
    }
    RemoveTempFileDelayed(notifyDir, fileName);
    if (type == JS_LEAK) {
        HIVIEW_LOGI("JS_LEAK type, maybe need to wait to dump");
        HandleJSLeak(pid, tid, notifyDir, fileName);
        return false;
    }
    if (type == ARKWEB_LEAK || type == JSVM_LEAK || type == KMP_LEAK) {
        HIVIEW_LOGI("%{public}d type, maybe need to wait to dump", type);
        HandleThirdPartyLeak(pid, tid, notifyDir, fileName);
        return false;
    }

    HIVIEW_LOGI("LEAK_STACK type, will dump now");
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(GetLeakStackNotifyInfo(pid));
    notifyInfo->SetNotifyType(type);
    notifyInfo->SetNotifyDir(notifyDir);
    notifyInfo->SetNotifyFileName(fileName);
    notifyInfo->SetTid(tid);
    notifyInfo->GetHapVersion();
    notifyInfo->SetLeakedTime();
    leakingPidsInfo_.insert(make_pair(pid, notifyInfo));
    return true;
}

shared_ptr<FaultInfoBase> NotifyLeakDetector::GetJSLeakNotifyInfo(pid_t pid, NotifyProcessInfo notifyProcessInfo)
{
    shared_ptr<FaultInfoBase> faultInfo = make_shared<NotifyLeakInfo>(pid, notifyProcessInfo.processName);
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    notifyInfo->SetLeakGrade(LEAK_CONTROL);
    notifyInfo->SetLimitSize(notifyProcessInfo.limitSize);
    notifyInfo->SetLiveObjectSize(notifyProcessInfo.liveObjectSize);
    notifyInfo->SetHeapdumpType(notifyProcessInfo.heapdumpType);
    if (notifyInfo->GetHeapdumpType() != OOM_TYPE &&
        notifyInfo->GetHeapdumpType() != "EXT_PSS_SYSTEM_TELEMETRY" &&
        notifyInfo->GetHeapdumpType() != "RSS_SYSTEM_TELEMETRY") {
        notifyInfo->SetLeakGrade(LEAK_ERROR);
    }
    return faultInfo;
}

shared_ptr<FaultInfoBase> NotifyLeakDetector::GetLeakStackNotifyInfo(pid_t pid)
{
    string processName = DfrUtil::GetInstance().GetProcessName(pid);
    shared_ptr<FaultInfoBase> faultInfo = make_shared<NotifyLeakInfo>(pid, processName);
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    notifyInfo->SetLeakGrade(LEAK_WARNING);
    return faultInfo;
}

void NotifyLeakDetector::RemoveFinishedInfo(int64_t pid)
{
    notifyDetectorMtx_.lock();
    if (leakingPidsInfo_.find(pid) != leakingPidsInfo_.end()) {
        leakingPidsInfo_.erase(pid);
    }
    HIVIEW_LOGD("RemoveFinishedInfo: now leakingPidsInfo_ size is %{public}zu", leakingPidsInfo_.size());
    notifyDetectorMtx_.unlock();
}

void NotifyLeakDetector::ReportAppeventImmediately(const struct NotifyProcessInfo& processInfo)
{
    AppEventHandler::ResourceOverLimitInfo info;
    info.pid = processInfo.pid;
    info.uid = processInfo.uid;
    info.appRunningUniqueId = processInfo.appRunningId;
    info.bundleName = processInfo.processName;
    info.bundleVersion = processInfo.hapVersion;
    info.resourceType = "js_heap";
    info.limitSize = static_cast<uint64_t>(processInfo.limitSize);
    info.liveobjectSize = static_cast<uint64_t>(processInfo.liveObjectSize);
    DfrUtil::GetInstance().PostAppEvent(info);
    HIVIEW_LOGI("js_heap PostEventWithoutLog end");
}

void NotifyLeakDetector::ReportAppeventDelay(const NotifyProcessInfo& info)
{
    ffrt::submit([=] {
        string processInfoKey = GetProcessInfoKey(info.pid, info.tid);
        notifyDetectorMtx_.lock();
        auto &infos = processInfoList_[processInfoKey];
        auto it = std::find_if(infos.begin(), infos.end(),
                               [appRunningId = info.appRunningId](const NotifyProcessInfo& search) {
                                   return search.appRunningId == appRunningId;
                               });
        if (it == infos.end()) {
            HIVIEW_LOGW("processInfoKey: %{public}s not found in processInfoList_, must have send hiappevent",
                        processInfoKey.c_str());
            notifyDetectorMtx_.unlock();
            return;
        }

        if (!it->hasGotRawheap) {
            HIVIEW_LOGW("processInfoKey: %{public}s has got rawheap, send hiappevent by other thread",
                        processInfoKey.c_str());
            notifyDetectorMtx_.unlock();
            return;
        }
        HIVIEW_LOGI("processInfoKey: %{public}s never got rawheap, send hiappevent immediately",
                    processInfoKey.c_str());
        ReportAppeventImmediately(info);
        notifyDetectorMtx_.unlock();
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyProcessCreate").delay(WAIT_RAWHEAP_DELAY)});
}

// on sysevent
void NotifyLeakDetector::ProcessEventListenening(const string &type, const string &pid,
                                                 const NotifyProcessInfo& notifyProcessInfo)
{
    if (type == "hidebugDump") {
        SetHidebugQuota();
        return;
    }
    if (type == "hidumperJsvmRawHeap") {
        UpdateHidumperVariable(pid, "");
        isHidumperRaw_.store(true);
        return;
    } else {
        isHidumperRaw_.store(false);
    }
    if (type == "hidumperRawHeap") {
        UpdateHidumperVariable(pid, "");
        return;
    }
    if (type == "hidumper") {
        UpdateHidumperVariable(pid, "");
        return;
    }
    if (type == "hidumperNativeHeap") {
        UpdateHidumperVariable(pid, "");
        return;
    }
    if (type == "hidumperKmpHeap") {
        UpdateHidumperVariable(pid, "");
        return;
    }
    if (type == "hidumperJsvmHeap") {
        UpdateHidumperVariable(pid, "");
        return;
    }
    if (type == "hidumperArkwebJsHeap") {
        UpdateHidumperVariable(pid, "");
        return;
    }

    if (notifyProcessInfo.willReportAppeventImmediately) {
        ReportAppeventImmediately(notifyProcessInfo);
    } else if (notifyProcessInfo.eventConfig == "event_rawheap") {
        ReportAppeventDelay(notifyProcessInfo);
    }
    UpdateInfoList(notifyProcessInfo);
}

ErrCode NotifyLeakDetector::UpdateInfoList(NotifyProcessInfo info)
{
    string processInfoKey = GetProcessInfoKey(info.pid, info.tid);
    notifyDetectorMtx_.lock();
    HIVIEW_LOGI("%{public}d %{public}d now get type: %{public}s, eventConfig: %{public}s, waiting for rawheap file",
                info.pid, info.tid, info.heapdumpType.c_str(), info.eventConfig.c_str());
    processInfoList_[processInfoKey].push_back(info);
    notifyDetectorMtx_.unlock();
    return SUCCESSED;
}

// on file callback
ErrCode NotifyLeakDetector::LeakNotify(string notifyDir, string fileName)
{
    if (MaybeHidumperFile(fileName)) {
        RemoveTempFileDelayed(notifyDir, fileName);
    }
    notifyDetectorMtx_.lock();
    pid_t pid = 0;
    bool ret = GetNotifyState(notifyDir, fileName, pid);
    if (!ret) {
        notifyDetectorMtx_.unlock();
        return FAILURE;
    }
    for (auto &it : leakingPidsInfo_) {
        const shared_ptr<FaultInfoBase> &faultInfo = it.second;
        if (!faultInfo) {
            HIVIEW_LOGE("faultInfo is null, pid is %{public}" PRId64, it.first);
            continue;
        }
        const pid_t pid = faultInfo->GetPid();
        if (faultInfo->IsDumping()) {
            continue;
        }
        faultInfo->SetIsDumping();
        // one pid info, ont task, exe one by one
        ffrt::submit([=] {
            ExeNextStateProcess(faultInfo, PROC_DUMP_STATE);
            RemoveFinishedInfo(pid);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyLeakDetector_LeakNotify")});
    }
    notifyDetectorMtx_.unlock();
    return SUCCESSED;
}

ErrCode NotifyLeakDetector::ExeNextStateProcess(shared_ptr<FaultInfoBase> faultInfo, FaultStateType nextStatType)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    HIVIEW_LOGI("process %{public}s, now state is %{public}s, next state is %{public}s",
        name.c_str(), STATE_TO_STR[notifyInfo->GetState()], STATE_TO_STR[nextStatType]);
    notifyInfo->SetState(nextStatType);
    if (nextStatType == PROC_FINISHED_STATE) {
        notifyDetectorMtx_.lock();
        auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
        string processInfoKey = GetProcessInfoKey(pid, notifyInfo->GetTid());
        if (notifyInfo->GetNotifyType() == JS_LEAK &&
            processInfoList_.find(processInfoKey) != processInfoList_.end()) {
            processInfoList_.erase(processInfoKey);
        }
        notifyDetectorMtx_.unlock();
        return SUCCESSED;
    }
    FaultStateBase* statObj = NotifyLeakStateContext::GetInstance().GetStateObj(nextStatType);
    if (statObj == nullptr) {
        HIVIEW_LOGE("%{public}s state obj is null", STATE_TO_STR[faultInfo->GetState()]);
        return NULL_POINTER;
    }
    return statObj->StateProcess(faultInfo, *this);
}

FaultStateBase* NotifyLeakDetector::GetStateObj(FaultStateType stateType)
{
    FaultStateBase* stateObj = NotifyLeakStateContext::GetInstance().GetStateObj(stateType);
    if (stateObj == nullptr) {
        HIVIEW_LOGE("%{public}s state obj is null", STATE_TO_STR[stateType]);
        return nullptr;
    }
    return stateObj;
}

void NotifyLeakDetector::SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt)
{
    SetAppLimit(appResourceLimt);
}

void NotifyLeakDetector::UpdateHidumperVariable(const string &pid, const string &fileName)
{
    ffrt::submit([=] {
        hidumperMtx_.lock();
        if (!pid.empty()) {
            HIVIEW_LOGI("update pid: %{public}s", pid.c_str());
            pidForHidumper_ = "-" + pid + "-";
        }
        if (IsLeakListFile(fileName)) {
            HIVIEW_LOGI("update leaklistName: %{public}s", fileName.c_str());
            leaklistName_ = fileName;
        }
        if (IsStaticJsHeapFile(fileName)) {
            HIVIEW_LOGI("update staticJsheapName: %{public}s", fileName.c_str());
            staticJsheapName_ = fileName;
        } else if (IsJsHeapFile(fileName)) {
            HIVIEW_LOGI("update jsheapName: %{public}s", fileName.c_str());
            jsheapName_ = fileName;
        }
        if (IsCjHeapFile(fileName)) {
            HIVIEW_LOGI("update cjheapName: %{public}s", fileName.c_str());
            cjheapName_ = fileName;
        }
        if (IsNativeHeapFile(fileName)) {
            HIVIEW_LOGI("update nativeheapName: %{public}s", fileName.c_str());
            nativeheapName_ = fileName;
        }
        if (IsKmpHeapFile(fileName)) {
            HIVIEW_LOGI("update kmpheapName: %{public}s", fileName.c_str());
            kmpheapName_ = fileName;
        }
        if (IsJsvmHeapFile(fileName)) {
            HIVIEW_LOGI("update jsvmheapName: %{public}s", fileName.c_str());
            jsvmheapName_ = fileName;
        }
        if (IsArkwebJsHeapFile(fileName)) {
            HIVIEW_LOGI("update arkWebJsheapName: %{public}s", fileName.c_str());
            arkwebJsheapName_ = fileName;
        }
        TryToCopyFileForHidumper();
        hidumperMtx_.unlock();
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyLeakDetector_UpdateHidumperVariable")});
}

bool NotifyLeakDetector::MaybeHidumperFile(const string &filename)
{
    bool ret = false;
    if (IsStaticJsHeapFile(filename)) {
        UpdateHidumperVariable("", filename);
        ret = true;
    } else if (IsJsHeapFile(filename)) {
        UpdateHidumperVariable("", filename);
        ret = true;
    }

    if (IsLeakListFile(filename)) {
        UpdateHidumperVariable("", filename);
        ret = true;
    }

    if (IsCjHeapFile(filename)) {
        UpdateHidumperVariable("", filename);
        ret = true;
    }

    if (IsNativeHeapFile(filename)) {
        UpdateHidumperVariable("", filename);
        ret = true;
    }

    if (IsKmpHeapFile(filename)) {
        UpdateHidumperVariable("", filename);
        ret = true;
    }

    if (IsJsvmHeapFile(filename)) {
        UpdateHidumperVariable("", filename);
        ret = true;
    }

    if (IsArkwebJsHeapFile(filename)) {
        UpdateHidumperVariable("", filename);
        ret = true;
    }
    return ret;
}

bool NotifyLeakDetector::IsRawHeapFile(const string &filename)
{
    return (filename.length() >= RAWHEAP_SUFFIX.length() &&
         filename.substr(filename.length() - RAWHEAP_SUFFIX.length()) == RAWHEAP_SUFFIX);
}

bool NotifyLeakDetector::IsJsHeapFile(const string &filename)
{
    const vector<string> heapPrefixes = {
        JSHEAP_PREFIX,
        HYBRID_JSHEAP_PREFIX,
        STATIC_JSHEAP_PREFIX
    };

    for (const auto& prefix : heapPrefixes) {
        if (filename.size() >= prefix.size() &&
            filename.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

bool NotifyLeakDetector::IsDynamicJsHeapFile(const string &filename)
{
    return filename.length() >= JSHEAP_PREFIX.length() &&
        filename.substr(0, JSHEAP_PREFIX.length()) == JSHEAP_PREFIX;
}

bool NotifyLeakDetector::IsStaticJsHeapFile(const string &filename)
{
    return filename.length() >= STATIC_JSHEAP_PREFIX.length() &&
        filename.substr(0, STATIC_JSHEAP_PREFIX.length()) == STATIC_JSHEAP_PREFIX;
}

void NotifyLeakDetector::CopyStaticJsHeapLate(pid_t pid, const string &notifyDir, const string &filename)
{
    string oldPath = notifyDir + "/" + filename;
    if (DfrUtil::GetInstance().IsFileEmpty(oldPath)) {
        HIVIEW_LOGW("static_jsheap late copy: source file is empty");
        return;
    }
    string processName = DfrUtil::GetInstance().GetProcessName(pid);
    auto notifyInfo = make_shared<NotifyLeakInfo>(pid, processName);
    notifyInfo->SetLeakedTime();
    string newPath = notifyInfo->GetStaticJsLeakFilePath();
    if (OldLogCleaner::GetInstance().CopyFile(oldPath, newPath, processName, notifyInfo->GetStaticJsLeakFileName())) {
        HIVIEW_LOGI("static_jsheap late copy success, dst: %{public}s", newPath.c_str());
    } else {
        HIVIEW_LOGW("static_jsheap late copy failed");
    }
    DfrUtil::GetInstance().RemoveFile(oldPath);
}

bool NotifyLeakDetector::IsLeakListFile(const string &filename)
{
    return filename.length() >= LEAKLIST_PREFIX.length() &&
        filename.substr(0, LEAKLIST_PREFIX.length()) == LEAKLIST_PREFIX;
}

bool NotifyLeakDetector::IsCjHeapFile(const string &filename)
{
    return filename.length() >= CJHEAP_PREFIX.length() &&
        filename.substr(0, CJHEAP_PREFIX.length()) == CJHEAP_PREFIX;
}

bool NotifyLeakDetector::IsNativeHeapFile(const string &filename)
{
    return filename.length() >= NATIVEHEAP_PREFIX.length() &&
        filename.substr(0, NATIVEHEAP_PREFIX.length()) == NATIVEHEAP_PREFIX;
}

bool NotifyLeakDetector::IsKmpHeapFile(const string &filename)
{
    return filename.length() >= KMPHEAP_PREFIX.length() &&
        filename.substr(0, KMPHEAP_PREFIX.length()) == KMPHEAP_PREFIX;
}

bool NotifyLeakDetector::IsJsvmHeapFile(const string &filename)
{
    return filename.length() >= JSVMHEAP_PREFIX.length() &&
        filename.substr(0, JSVMHEAP_PREFIX.length()) == JSVMHEAP_PREFIX;
}

bool NotifyLeakDetector::IsArkwebJsHeapFile(const string &filename)
{
    return filename.length() >= ARKWEBJS_HEAP_PREFIX.length() &&
        filename.substr(0, ARKWEBJS_HEAP_PREFIX.length()) == ARKWEBJS_HEAP_PREFIX;
}

void NotifyLeakDetector::TryToCopyFileForHidumper()
{
    if (jsheapName_.find(pidForHidumper_) != string::npos) {
        CopyFileForHidumper(jsheapName_);
        jsheapName_ = "";
    }

    if (staticJsheapName_.find(pidForHidumper_) != string::npos) {
        CopyFileForHidumper(staticJsheapName_);
        staticJsheapName_ = "";
    }

    if (leaklistName_.find(pidForHidumper_) != string::npos) {
        CopyFileForHidumper(leaklistName_);
        leaklistName_ = "";
    }

    if (cjheapName_.find(pidForHidumper_) != string::npos) {
        CopyFileForHidumper(cjheapName_);
        cjheapName_ = "";
    }

    if (nativeheapName_.find(pidForHidumper_) != string::npos) {
        CopyFileForHidumper(nativeheapName_);
        nativeheapName_ = "";
    }

    if (kmpheapName_.find(pidForHidumper_) != string::npos) {
        CopyFileForHidumper(kmpheapName_);
        kmpheapName_ = "";
    }

    if (jsvmheapName_.find(pidForHidumper_) != string::npos) {
        CopyFileForHidumper(jsvmheapName_);
        jsvmheapName_ = "";
    }

    if (arkwebJsheapName_.find(pidForHidumper_) != string::npos) {
        CopyFileForHidumper(arkwebJsheapName_);
        arkwebJsheapName_ = "";
    }
}

void NotifyLeakDetector::CopyFileForHidumper(const string &filename)
{
    string src = "/data/log/faultlog/temp/" + filename;
    string fullFileName = "hidumper-" + filename;
    bool isHidumperRaw = isHidumperRaw_.load();
    bool isDynamicJsHeapRaw = IsDynamicJsHeapFile(filename) && IsRawHeapFile(filename);
    if (isHidumperRaw && !IsLeakListFile(filename)) {
        fullFileName += ".rawheap";
    }
    if (IsKmpHeapFile(filename)) {
        fullFileName += ".kdump";
    }
    if (IsArkwebJsHeapFile(filename)) {
        size_t pos = fullFileName.find(pidForHidumper_);
        if (pos != std::string::npos) {
            fullFileName.erase(pos, pidForHidumper_.length() - 1);
        }
    }
    string dst = string(MEMORY_LEAK_PATH) + "/" + fullFileName;
    bool ret = OldLogCleaner::GetInstance().CopyFile(src, dst, "", fullFileName);
    DfrUtil::GetInstance().RemoveFile(src);
    if (ret) {
        HIVIEW_LOGI("copy file for hidumper success, src:%{public}s dst:%{public}s", src.c_str(), dst.c_str());
    } else {
        HIVIEW_LOGE("copy file for hidumper failed, src:%{public}s dst:%{public}s", src.c_str(), dst.c_str());
        return;
    }
    if ((isHidumperRaw || isDynamicJsHeapRaw) && !DfrUtil::GetInstance().AppendMetadata(dst)) {
        HIVIEW_LOGW("append metadata to %{public}s failed", dst.c_str());
    }
}

void NotifyLeakDetector::SetHidebugQuota()
{
    if (hidumpCnt_.load() <= 0 && system::GetIntParameter("persist.hiview.hidebugdump.maxcount", 0) == 0) {
        HIVIEW_LOGE("There is no quota for hidump");
        return;
    }
    if (hidumpCnt_.load() != system::GetIntParameter("persist.hiview.hidebugdump.maxcount", 0)) {
        HIVIEW_LOGE("local quota != parameter quota, correction local quota");
        hidumpCnt_.store(system::GetIntParameter("persist.hiview.hidebugdump.maxcount", 0));
    }
    hidumpCnt_.fetch_sub(1);
    int ret = system::SetParameter("persist.hiview.hidebugdump.maxcount", to_string(hidumpCnt_.load()));
    HIVIEW_LOGI("set hidump max count callback: %{public}d", ret);
}

void NotifyLeakDetector::RemoveTempFileDelayed(const std::string &notifyDir, const std::string &fileName)
{
    string path = notifyDir + "/" + fileName;
    HIVIEW_LOGI("wait to remove file, path: %{public}s", path.c_str());
    ffrt::submit([=] {
        if (remove(path.c_str()) == 0) {
            HIVIEW_LOGI("success to remove file path: %{public}s", path.c_str());
        } else {
            HIVIEW_LOGE("failed to remove file path: %{public}s, err: %{public}s", path.c_str(), strerror(errno));
        }
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_RemoveTempFileDelayed").delay(OLD_CLEAN_DELAY)});
}

string NotifyLeakDetector::WaitForStaticJsHeapFile(pid_t pid, pid_t tid, int timeoutSeconds)
{
    string key = GetProcessInfoKey(pid, tid);
    for (int retry = 0; retry < timeoutSeconds; retry++) {
        string result;
        bool keyExists = GetStaticJsHeapFromInfoList(key, result);
        if (!keyExists) {
            HIVIEW_LOGI("WaitForStaticJsHeapFile: processInfoList_ key gone, stop waiting");
            return "";
        }
        if (!result.empty()) {
            HIVIEW_LOGI("WaitForStaticJsHeapFile: found after %{public}ds, file=%{public}s", retry, result.c_str());
            return result;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    HIVIEW_LOGI("WaitForStaticJsHeapFile: timeout after %{public}ds", timeoutSeconds);
    return "";
}

bool NotifyLeakDetector::GetStaticJsHeapFromInfoList(const string &key, string &result)
{
    notifyDetectorMtx_.lock();
    bool keyExists = processInfoList_.count(key) > 0;
    if (keyExists) {
        for (auto &info : processInfoList_[key]) {
            if (!info.staticRawheapFile.empty()) {
                result = info.staticRawheapFile;
                break;
            }
        }
    }
    notifyDetectorMtx_.unlock();
    return keyExists;
}
} // namespace HiviewDFX
} // namespace OHOS
