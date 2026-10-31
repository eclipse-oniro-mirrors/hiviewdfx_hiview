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

#ifndef DFR_FAULT_DETECTOR_MANAGER_H
#define DFR_FAULT_DETECTOR_MANAGER_H

#include "plugin.h"
#include "plugin_factory.h"
#include "app_event_publisher.h"
#include "sys_event.h"
#include "fault_common.h"
#include "native_leak_detector.h"
#include "native_leak_xperf_detector.h"
#include "kernel_leak_detector.h"
#include "notify_leak_detector.h"
#include "notify_leak_deal_fork_process.h"
#include "thread_leak_detector.h"
#include "fd_leak_detector.h"
#include "common_status_listener.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {

class FaultDetectorManager : public Singleton<FaultDetectorManager> {
    DECLARE_SINGLETON(FaultDetectorManager);
public:
    bool OnEvent(std::shared_ptr<Event> &event);
    bool ReadyToLoad();
    void OnLoad();
    void OnUnload();
    void OnEventListeningCallback(const Event &msg);
    void AddAppEventHandler(std::shared_ptr<AppEventHandler> handler);
    bool OnFileDescriptorEvent(int fd, int type);
    int32_t GetPollFd();
    int32_t GetPollType();
    void Dump(int fd, const std::vector<std::string> &cmds);
    void UpdateQuota();
    void NotifyProcessCreate(const std::string &procName, pid_t pid, uint32_t subProcessType);
    void NotifyProcessDied(const std::string &procName, pid_t pid);

    static void OnForegroundChanged(const Dubai::ForegroundAttr &attr);
    static bool CheckIfAppForeGround(const std::string &name);

private:
    bool MatchAndNotifyEvent(struct inotify_event *event);
    void DumpFileList(int fd, const std::string &path, const std::string &type);
    void PrepareFaultDetectorEnv();
    void MonitorProcess();
    void HandleHidumperEventListening();
    void GpuMemoryOverReport(SysEvent &sysEvent);
    void HandleLowmemEvent(SysEvent &sysEvent);
    void HandleProcessKillEvent(SysEvent &sysEvent);
    void HandleProcessKillTelemetry(SysEvent &sysEvent);
    void HandleKernelEvent(SysEvent &sysEvent);
    void HandleVmaEvent(SysEvent &sysEvent);
    void HandleSplitMemoryEvent(std::shared_ptr<Event> &event);
    void HandleMemmgrEvent(std::shared_ptr<Event> &event);
    void HandleForkDumpEvent(std::shared_ptr<Event> &event);
    void HandelXperfEvent(std::shared_ptr<Event> &event);
    void HandleAshmemEvent(SysEvent &sysEvent);
    void HandleArkStats(SysEvent &sysEvent);
    void HandleSetMemory(SysEvent &sysEvent);
    void GetNotifyProcessInfo(SysEvent &sysEvent, NotifyProcessInfo &notifyProcessInfo);
    void HandleKernelZoneEvent(SysEvent &sysEvent);
    void GetKzPidForKill(const std::string &procMsg, std::pair<pid_t, int> &procForReport,
                         std::pair<pid_t, int> &procForKill);
    std::string GetKzObjForReport(const std::string &kobjMsg);
    void ReportKillReason(SysEvent &sysEvent);
    void HandleSysloadDetectorEvent(SysEvent &sysEvent);
    void HandleKernelThreadEvent(SysEvent &sysEvent);
    void HandleHisysevent(SysEvent &sysEvent);
    void HandleRequestUiTree(std::shared_ptr<Event> &event);
    std::string GetInfoFromString(const std::string &procMsg, size_t &start, const std::string &sig);
    void HandleProcessKill(SysEvent &sysEvent);
    bool ValidatePathPrefix(const std::string &path, std::string &realPath);
private:
    std::vector<DetectorType> detectorList_;

    int inotifyFd_ { -1 };
    std::string notifyEventName_;
    std::map<std::string, int> fileMap_;
    bool isLoopContinue_ { false };
    ffrt::task_handle processTaskHandle_;
    const int taskInterval_;
    static ffrt::mutex foregroundMtx_;
    static std::vector<std::string> foregroundApps_;
    std::shared_ptr<CommonStatusListener> commonStatusListener_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FAULT_DETECTOR_MANAGER_H
