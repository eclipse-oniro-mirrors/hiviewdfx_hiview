/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "faultlog_processor_base.h"

#include "constants.h"
#include "dfx_define.h"
#include "faultlog_bundle_util.h"
#include "faultlog_formatter.h"
#include "faultlog_hilog_helper.h"
#include "faultlog_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "log_analyzer.h"
#include "page_history_manager.h"
#include "parameter_ex.h"
#include "process_status.h"
#include "string_util.h"
#include "parameters.h"

namespace OHOS {
namespace HiviewDFX {
using namespace FaultLogger;
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FaultlogHilogHelper;

namespace {
std::string GetSummaryFromSectionMap(int32_t type, const std::map<std::string, std::string>& maps)
{
    std::string key = "";
    switch (type) {
        case CPP_CRASH:
            key = FaultKey::KEY_THREAD_INFO;
            break;
        default:
            break;
    }

    if (key.empty()) {
        return "";
    }

    auto value = maps.find(key);
    if (value == maps.end()) {
        return "";
    }
    return value->second;
}

bool IsFaultTypeSupport(const FaultLogInfo& info)
{
    if ((info.faultLogType <= FaultLogType::ALL) || (info.faultLogType > FaultLogType::CJ_ERROR)) {
        HIVIEW_LOGW("Unsupported fault type");
        return false;
    }
    return true;
}

bool IsSnapshot(const FaultLogInfo& info)
{
    if (info.reason.find("CppCrashKernelSnapshot") != std::string::npos) {
        HIVIEW_LOGI("Skip cpp crash kernel snapshot fault %{public}d", info.pid);
        return true;
    }
    return false;
}

bool IsFaultByIpc(const FaultLogInfo& info)
{
    return info.faultLogType == FaultLogType::CPP_CRASH ||
        info.faultLogType == FaultLogType::SYS_FREEZE ||
        info.faultLogType == FaultLogType::SYS_WARNING ||
        info.faultLogType == FaultLogType::APP_FREEZE ||
        info.faultLogType == FaultLogType::APPFREEZE_WARNING;
}
} // namespace

void FaultLogProcessorBase::AddFaultLog(FaultLogInfo& info, const std::shared_ptr<EventLoop>& workLoop,
    const std::shared_ptr<FaultLogManager>& faultLogManager)
{
    workLoop_ = workLoop;
    faultLogManager_ = faultLogManager;
    if (!IsFaultTypeSupport(info) || IsSnapshot(info)) {
        return;
    }

    ProcessFaultLog(info);

    if (!IsFaultByIpc(info)) {
        return;
    }
    SaveFaultInfoToRawDb(info);
    ReportEventToAppEvent(info);
    DoFaultLogLimit(info);
}

bool FaultLogProcessorBase::VerifyModule(FaultLogInfo& info)
{
    if (!IsValidPath(info.logPath)) {
        HIVIEW_LOGE("The log path is incorrect, and the current log path is: %{public}s.", info.logPath.c_str());
        return false;
    }
    HIVIEW_LOGI("Start saving Faultlog of Process:%{public}d, Name:%{public}s, Reason:%{public}s.",
        info.pid, info.module.c_str(), info.reason.c_str());
    if (info.sectionMap.find("PROCESS_NAME") == info.sectionMap.end()) {
        info.sectionMap["PROCESS_NAME"] = info.module; // save process name
    }
    // Non system processes use UID to pass events to applications
    if (!IsSystemProcess(info.module, info.id) && info.sectionMap["SCBPROCESS"] != "Yes") {
        std::string appName = GetApplicationNameById(info.id);
        if (!appName.empty() && !ExtractSubMoudleName(info.module)) {
            info.module = appName; // if bundle name is not empty, replace module name by it.
        }
    }

    auto tmpModule = info.module;
    if (tmpModule.find("hmos.browser:render") != std::string::npos ||
        tmpModule.find("hmos.browser:gpu") != std::string::npos) {
        tmpModule = tmpModule.substr(0, tmpModule.find("hmos.browser"));
        info.module = tmpModule + "hmos.arkwebcore";
    }

    HIVIEW_LOGD("nameProc %{public}s", info.module.c_str());
    if ((info.module.empty()) ||
        (info.faultLogType != FaultLogType::ADDR_SANITIZER && !IsModuleNameValid(info.module))) {
        HIVIEW_LOGW("Invalid module name %{public}s", info.module.c_str());
        return false;
    }
    return true;
}

void FaultLogProcessorBase::ProcessFaultLog(FaultLogInfo& info)
{
    if (!VerifyModule(info)) {
        return;
    }
    // step1: add common info: uid module name, pid, reason, summary etc
    AddCommonInfo(info);
    // step2: add specific info: need implement in derived class
    AddSpecificInfo(info);
    // step3: generate fault log file
    SaveFaultLogToFile(info);
    PrintFaultLogInfo(info);
}

void FaultLogProcessorBase::SaveFaultLogToFile(FaultLogInfo& info)
{
    if (info.dumpLogToFaultlogger && faultLogManager_) {
        faultLogManager_->SaveFaultLogToFile(info);
    }
}

void FaultLogProcessorBase::DoFaultLogLimit(const FaultLogInfo& info)
{
    bool isNeedLimitFile = (info.dumpLogToFaultlogger && ((info.faultLogType == FaultLogType::CPP_CRASH) ||
        (info.faultLogType == FaultLogType::APP_FREEZE)) && IsFaultLogLimit());
    if (isNeedLimitFile) {
        DoFaultLogLimit(info.logPath, info.faultLogType);
    }
}

void FaultLogProcessorBase::SaveFaultInfoToRawDb(FaultLogInfo& info)
{
    if (faultLogManager_) {
        faultLogManager_->SaveFaultInfoToRawDb(info);
    }
}

void FaultLogProcessorBase::AddCommonInfo(FaultLogInfo& info)
{
    info.sectionMap[FaultKey::DEVICE_INFO] = Parameter::GetString("const.product.name", "Unknown");
    if (info.sectionMap.find(FaultKey::BUILD_INFO) == info.sectionMap.end()) {
        info.sectionMap[FaultKey::BUILD_INFO] = Parameter::GetString("const.product.software.version", "Unknown");
    }
    info.sectionMap[FaultKey::MODULE_UID] = std::to_string(info.id);
    info.sectionMap[FaultKey::MODULE_PID] = std::to_string(info.pid);
    info.module = RegulateModuleNameIfNeed(info.module);
    info.sectionMap[FaultKey::MODULE_NAME] = info.module;
    info.sectionMap[FaultKey::DEVICE_DEBUGABLE] = Parameter::IsUserMode() ? "No" : "Yes";
    AddPhoneFocusMode(info);
    AddBundleInfo(info);
    AddForegroundInfo(info);

    if (info.reason.empty()) {
        info.reason = info.sectionMap[FaultKey::REASON];
    } else {
        info.sectionMap[FaultKey::REASON] = info.reason;
    }

    if (info.summary.empty()) {
        info.summary = GetSummaryFromSectionMap(info.faultLogType, info.sectionMap);
    } else {
        info.sectionMap[FaultKey::SUMMARY] = info.summary;
    }

    UpdateTerminalThreadStack(info);

    // parse fingerprint by summary or temp log for native crash
    AnalysisFaultlog(info, info.parsedLogInfo);
    info.sectionMap.insert(info.parsedLogInfo.begin(), info.parsedLogInfo.end());
    info.parsedLogInfo.clear();
    // Internal reserved fields, avoid illegal privilege escalation to access files
    info.sectionMap.erase(FaultKey::APPEND_ORIGIN_LOG);
}

void FaultLogProcessorBase::AddBundleInfo(FaultLogInfo& info)
{
    DfxBundleInfo bundleInfo;
    if (info.id < MIN_APP_USERID || !GetDfxBundleInfo(info.module, bundleInfo)) {
        return;
    }

    if (info.module.find("arkwebcore") == std::string::npos) {
        info.id = bundleInfo.uid;
    }

    if (!bundleInfo.versionName.empty()) {
        info.sectionMap[FaultKey::MODULE_VERSION] = bundleInfo.versionName;
        info.sectionMap[FaultKey::VERSION_CODE] = std::to_string(bundleInfo.versionCode);
    }

    info.sectionMap[FaultKey::IS_SYSTEM_APP] = GetIsSystemApp(info.module, info.id) ? "Yes" : "No";
    info.sectionMap[FaultKey::CPU_ABI] = bundleInfo.cpuAbi;
    info.sectionMap[FaultKey::RELEASE_TYPE] = bundleInfo.releaseType;
    info.sectionMap[FaultKey::PRE_INSTALL] = bundleInfo.isPreInstalled ? "Yes" : "No";
}

void FaultLogProcessorBase::AddForegroundInfo(FaultLogInfo& info)
{
    if (!info.sectionMap[FaultKey::FOREGROUND].empty() || info.id < MIN_APP_USERID) {
        return;
    }

    if (UCollectUtil::ProcessStatus::GetInstance().GetProcessState(info.pid) == UCollectUtil::FOREGROUND) {
        info.sectionMap[FaultKey::FOREGROUND] = "Yes";
    } else if (UCollectUtil::ProcessStatus::GetInstance().GetProcessState(info.pid) == UCollectUtil::BACKGROUND) {
        int64_t lastFgTime = static_cast<int64_t>(UCollectUtil::ProcessStatus::GetInstance()
                                                  .GetProcessLastForegroundTime(info.pid));
        info.sectionMap[FaultKey::FOREGROUND] = lastFgTime > info.time ? "Yes" : "No";
    }
}

void FaultLogProcessorBase::UpdateTerminalThreadStack(FaultLogInfo& info)
{
    auto threadStack = GetStrValFromMap(info.sectionMap, "TERMINAL_THREAD_STACK");
    if (threadStack.empty()) {
        return;
    }
    // Replace the '\n' in the string with a line break character
    info.parsedLogInfo["TERMINAL_THREAD_STACK"] = StringUtil::ReplaceStr(threadStack, "\\n", "\n");
}

void FaultLogProcessorBase::PrintFaultLogInfo(const FaultLogInfo& info)
{
    HIVIEW_LOGI("\nSave Faultlog of Process:%{public}d\n"
        "ModuleName:%{public}s\n"
        "Reason:%{public}s\n",
        info.pid, info.module.c_str(), info.reason.c_str());
}

void FaultLogProcessorBase::AddPhoneFocusMode(FaultLogInfo& info)
{
    auto focusMode = system::GetIntParameter("persist.phone_focus.mode.status", 0);
    info.sectionMap[FaultKey::FOCUS_MODE] = std::to_string(focusMode);
}

std::string FaultLogProcessorBase::ReadLogFile(const std::string& logPath) const
{
    std::ifstream logReadFile(logPath);
    if (!logReadFile.is_open()) {
        return "";
    }
    return std::string(std::istreambuf_iterator<char>(logReadFile), std::istreambuf_iterator<char>());
}

void FaultLogProcessorBase::WriteLogFile(const std::string& logPath, const std::string& content) const
{
    std::ofstream logWriteFile(logPath, std::ios::out | std::ios::trunc);
    if (!logWriteFile.is_open()) {
        HIVIEW_LOGE("Failed to open log file: %{public}s", logPath.c_str());
        return;
    }
    logWriteFile << content;
    if (!logWriteFile.good()) {
        HIVIEW_LOGE("Failed to write content to log file: %{public}s", logPath.c_str());
    }
    logWriteFile.close();
}

void FaultLogProcessorBase::GetProcMemInfo(FaultLogInfo& info)
{
    if (!info.sectionMap["START_BOOT_SCAN"].empty()) {
        return;
    }

    std::ifstream meminfoStream("/proc/meminfo");
    if (meminfoStream) {
        constexpr int decimalBase = 10;
        unsigned long long totalMem = 0; // row 1
        unsigned long long freeMem = 0; // row 2
        unsigned long long availMem = 0; // row 3
        std::string meminfoLine;
        std::getline(meminfoStream, meminfoLine);
        totalMem = strtoull(GetDigtStrArr(meminfoLine).front().c_str(), nullptr, decimalBase);
        std::getline(meminfoStream, meminfoLine);
        freeMem = strtoull(GetDigtStrArr(meminfoLine).front().c_str(), nullptr, decimalBase);
        std::getline(meminfoStream, meminfoLine);
        availMem = strtoull(GetDigtStrArr(meminfoLine).front().c_str(), nullptr, decimalBase);
        meminfoStream.close();
        info.sectionMap[FaultKey::DEVICE_MEMINFO] = "Device Memory(kB): Total " + std::to_string(totalMem) +
            ", Free " + std::to_string(freeMem) + ", Available " + std::to_string(availMem);
        info.sectionMap[FaultKey::SYS_TOTAL_MEM] = std::to_string(totalMem);
        info.sectionMap[FaultKey::SYS_FREE_MEM] = std::to_string(freeMem);
        info.sectionMap[FaultKey::SYS_AVAIL_MEM] = std::to_string(availMem);
    } else {
        HIVIEW_LOGE("Fail to open /proc/meminfo");
    }
}

std::list<std::string> FaultLogProcessorBase::GetDigtStrArr(const std::string &target)
{
    std::list<std::string> ret;
    std::string temp = "";
    for (size_t i = 0, len = target.size(); i < len; i++) {
        if (target[i] >= '0' && target[i] <= '9') {
            temp += target[i];
            continue;
        }
        if (temp.size() != 0) {
            ret.push_back(temp);
            temp = "";
        }
    }
    if (temp.size() != 0) {
        ret.push_back(temp);
    }
    ret.push_back("0");
    return ret;
}

void FaultLogProcessorBase::AddPagesHistory(FaultLogInfo& info) const
{
    if (info.id < MIN_APP_USERID) {
        return;
    }
    auto trace = PageHistoryManager::GetInstance().GetPageHistory(info.module, info.pid);
    if (!trace.empty()) {
        info.sectionMap[FaultKey::PAGE_SWITCH_HISTORY] = std::move(trace);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
