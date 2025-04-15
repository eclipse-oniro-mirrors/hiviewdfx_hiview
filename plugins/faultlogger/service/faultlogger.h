/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef HIVIEWDFX_HIVIEW_FAULTLOGGER_H
#define HIVIEWDFX_HIVIEW_FAULTLOGGER_H
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "event.h"
#include "json/json.h"
#include "plugin.h"
#include "sys_event.h"

#include "faultlog_info.h"
#include "faultlog_manager.h"
#include "faultlog_query_result_inner.h"
#include "freeze_json_util.h"

namespace OHOS {
namespace HiviewDFX {
struct DumpRequest {
    bool requestDetail;
    bool requestList;
    bool compatFlag;
    std::string fileName;
    std::string moduleName;
    time_t time;
};

class Faultlogger : public Plugin {
public:
    Faultlogger() : mgr_(nullptr), hasInit_(false) {};
    virtual ~Faultlogger(){};

    // implementations of Plugin interfaces
    // for intercepting AppFreeze from collectors pipeline
    bool OnEvent(std::shared_ptr<Event> &event) override;
    bool IsInterestedPipelineEvent(std::shared_ptr<Event> event) override;
    bool CanProcessEvent(std::shared_ptr<Event> event) override;
    bool ReadyToLoad() override;
    void OnLoad() override;

    // dump debug infos through cmdline
    void Dump(int fd, const std::vector<std::string> &cmds) override;

    void AddFaultLog(FaultLogInfo& info);
    std::unique_ptr<FaultLogQueryResultInner> QuerySelfFaultLog(int32_t uid,
        int32_t pid, int32_t faultType, int32_t maxNum);

    static int RunSanitizerd();

private:
    bool VerifiedDumpPermission();
    void AddFaultLogIfNeed(FaultLogInfo& info, std::shared_ptr<Event> event);
    void AddPublicInfo(FaultLogInfo& info);
    static void AddBundleInfo(FaultLogInfo& info);
    static void AddForegroundInfo(FaultLogInfo& info);
    static void UpdateTerminalThreadStack(FaultLogInfo& info);
    void AddCppCrashInfo(FaultLogInfo& info);
    void AddHilog(FaultLogInfo& info);
    void AddDebugSignalInfo(FaultLogInfo& info) const;
    void Dump(int fd, const DumpRequest& request) const;
    void StartBootScan();
    bool JudgmentRateLimiting(std::shared_ptr<Event> event);
    static void HandleNotify(int32_t type, const std::string& fname);
    void ReportCppCrashToAppEvent(const FaultLogInfo& info) const;
    bool GetHilog(int32_t pid, std::string& log) const;
    static int DoGetHilogProcess(int32_t pid, int writeFd);
    void GetStackInfo(const FaultLogInfo& info, std::string& stackInfo) const;
    void ReportJsOrCjErrorToAppEvent(std::shared_ptr<SysEvent> sysEvent, FaultLogType faultType) const;
    void ReportSanitizerToAppEvent(std::shared_ptr<SysEvent> sysEvent) const;
    std::string GetMemoryStrByPid(long pid) const;
    FreezeJsonUtil::FreezeJsonCollector GetFreezeJsonCollector(const FaultLogInfo& info) const;
    void ReportAppFreezeToAppEvent(const FaultLogInfo& info, bool isAppHicollie = false) const;
    void ReportEventToAppEvent(const FaultLogInfo& info);
    bool CheckFaultLog(FaultLogInfo info);
    void CheckFaultLogAsync(const FaultLogInfo& info);
    void FillHilog(const std::string &hilogStr, Json::Value &hilog) const;
    void DeleteHilogInFreezeFile(std::string &readContent, bool &modified) const;
    void FaultlogLimit(const std::string &logPath, int32_t faultType) const;
    FaultLogInfo FillFaultLogInfo(SysEvent &sysEvent) const;
    void AddBootScanEvent();
    static bool ReadHilog(int fd, std::string& log);

    std::unique_ptr<FaultLogManager> mgr_;
    volatile bool hasInit_;
    std::unordered_map<std::string, std::time_t> eventTagTime_;
    class FaultloggerListener;
    std::shared_ptr<FaultloggerListener> eventListener_;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif  // HIVIEWDFX_HIVIEW_FAULTLOGGER_H

