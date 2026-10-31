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

#ifndef DFR_NOTIFY_LEAK_DETECTOR_H
#define DFR_NOTIFY_LEAK_DETECTOR_H

#include <vector>
#include "fault_common.h"
#include "fault_detector_base.h"
#include "notify_leak_dump_state.h"
#include "notify_leak_report_state.h"
#include "notify_leak_app_report_state.h"
#include "notify_leak_removal_state.h"
#include "notify_leak_state_context.h"
#include "notify_leak_info.h"
#include "ffrt.h"
#include "singleton.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
class NotifyLeakDetector : public FaultDetectorBase, public Singleton<NotifyLeakDetector> {
    DECLARE_SINGLETON(NotifyLeakDetector);

public:
    ErrCode MainProcess() override;
    void HandleDetector(int interval) override;
    void OnChangeState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType) override {};
    ErrCode ExeNextStateProcess(std::shared_ptr<FaultInfoBase> faultInfo, FaultStateType stateType) override;
    ErrCode LeakNotify(std::string notifyDir, std::string fileName);
    FaultStateBase* GetStateObj(FaultStateType stateType) override;
    void SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt) override;
    ErrCode UpdateInfoList(NotifyProcessInfo info);
    void ProcessEventListenening(const std::string &type, const std::string &pid,
                                 const NotifyProcessInfo& notifyProcessInfo);
    void UpdateHidumperVariable(const std::string &pid, const std::string &fileName);
    std::string WaitForStaticJsHeapFile(pid_t pid, pid_t tid, int timeoutSeconds);

private:
    bool ShouldDoMainProcess(int interval) override;
    bool ParseNotifyType(const std::string &fileName, NotifyType &type, pid_t &pid, pid_t &tid);
    bool ParseKmpHeapType(const std::vector<std::string> &splitedNames, NotifyType &type, pid_t &pid, pid_t &tid);
    bool ParseJsHeapType(const std::string &typeName, const std::vector<std::string> &splitedNames,
                         NotifyType &type, pid_t &pid, pid_t &tid);
    bool GetNotifyState(std::string notifyDir, std::string fileName, pid_t &pid);
    void RemoveFinishedInfo(int64_t pid);
    std::string GetProcessInfoKey(pid_t pid, pid_t tid);
    bool GetStaticJsHeapFromInfoList(const std::string &key, std::string &result);
    std::shared_ptr<FaultInfoBase> GetJSLeakNotifyInfo(pid_t pid, NotifyProcessInfo notifyProcessInfo);
    std::shared_ptr<FaultInfoBase> GetLeakStackNotifyInfo(pid_t pid);
    bool MaybeHidumperFile(const std::string &filename);
    void TryToCopyFileForHidumper();
    void CopyFileForHidumper(const std::string &filename);
    void HandleJSLeak(pid_t pid, pid_t tid, const std::string &notifyDir, const std::string &filename,
                      int retryCnt = 0);
    void HandleJSLeakRetry(pid_t pid, pid_t tid, const std::string &notifyDir,
                           const std::string &filename, int retryCnt);
    void HandleJSLeakProcess(pid_t pid, pid_t tid, const std::string &notifyDir,
                             const std::string &filename, const std::string &processInfoKey);
    void HandleStaticJsHeap(pid_t pid, pid_t tid, const std::string &notifyDir,
                            const std::string &filename, int retryCnt);
    std::shared_ptr<NotifyLeakInfo> MakeNotifyInfo(pid_t pid, pid_t tid, const std::string &notifyDir,
                                                   const std::string &filename, const NotifyProcessInfo &info);
    void DumpLeak(std::shared_ptr<NotifyLeakInfo> notifyInfo, int waitTime);
    void HandleThirdPartyLeak(pid_t pid, pid_t tid, const std::string &notifyDir, const std::string &filename,
                        int retryCnt = 0);
    void ResetDumpCntAndTime();
    void SetHidebugQuota();
    void ReportAppeventImmediately(const struct NotifyProcessInfo& processInfo);
    void ReportAppeventDelay(const struct NotifyProcessInfo& info);
    void RemoveTempFileDelayed(const std::string &notifyDir, const std::string &fileName);
    bool IsKmpHeapFile(const std::string &filename);
    bool IsLeakListFile(const std::string &filename);
    bool IsJsHeapFile(const std::string &filename);
    bool IsDynamicJsHeapFile(const std::string &filename);
    bool IsStaticJsHeapFile(const std::string &filename);
    void CopyStaticJsHeapLate(pid_t pid, const std::string &notifyDir, const std::string &filename);
    bool IsCjHeapFile(const std::string &filename);
    bool IsNativeHeapFile(const std::string &filename);
    bool IsRawHeapFile(const std::string &filename);
    bool IsJsvmHeapFile(const std::string &filename);
    bool IsArkwebJsHeapFile(const std::string &filename);
    bool IsJsHeapNoTidFile(const std::string &filename);
    bool CopyJsHeapNoTidFile(pid_t pid, const std::string &notifyDir,
                              const std::string &filename);

private:
    ffrt::mutex notifyDetectorMtx_;
    // key: pidString + tidString, value: processInfo
    std::unordered_map<std::string, std::vector<NotifyProcessInfo> > processInfoList_;

    // for hidumper
    ffrt::mutex hidumperMtx_;
    std::string pidForHidumper_ {"-unknown-"};
    std::string leaklistName_;
    std::string jsheapName_;
    std::string staticJsheapName_;
    std::string cjheapName_;
    std::string nativeheapName_;
    std::string kmpheapName_;
    std::string jsvmheapName_;
    std::string arkwebJsheapName_;
    int mainInterval_ { 0 };
    std::atomic<bool> isHidumperRaw_ { false };
    std::atomic<int> hidumpCnt_ { 0 };
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NOTIFY_LEAK_DETECTOR_H

