/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "panic_error_info_handle.h"
#include "securec.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "parameters.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace PanicErrorInfoHandle {
DEFINE_LOG_LABEL(0xD002D11, "PanicErrorInfoHandle");

using namespace std;

constexpr const char* HISTORY_LOG_PATH = "/data/log/bbox/history.log";
constexpr const char* SYS_FS_PSTORE_PATH = "/sys/fs/pstore/blackbox-ramoops-0";

/* fault category type */
constexpr const char* CATEGORY_SYSTEM_REBOOT = "SYSREBOOT";
constexpr const char* CATEGORY_SYSTEM_POWEROFF = "POWEROFF";
constexpr const char* CATEGORY_SYSTEM_PANIC = "PANIC";
constexpr const char* CATEGORY_SYSTEM_OOPS = "OOPS";
constexpr const char* CATEGORY_SYSTEM_CUSTOM = "CUSTOM";
constexpr const char* CATEGORY_SYSTEM_WATCHDOG = "HWWATCHDOG";
constexpr const char* CATEGORY_SYSTEM_HUNGTASK = "HUNGTASK";
constexpr const char* CATEGORY_SUBSYSTEM_CUSTOM = "CUSTOM";

/* top category type */
constexpr const char* TOP_CATEGORY_SYSTEM_RESET = "System Reset";
constexpr const char* TOP_CATEGORY_FREEZE = "System Freeze";
constexpr const char* TOP_CATEGORY_SYSTEM_POWEROFF = "POWEROFF";
constexpr const char* TOP_CATEGORY_SUBSYSTEM_CRASH = "Subsystem Crash";

/* module type */
constexpr const char* MODULE_SYSTEM = "SYSTEM";

/* fault event type */
constexpr const char* EVENT_SYSREBOOT = "SYSREBOOT";
constexpr const char* EVENT_LONGPRESS = "LONGPRESS";
constexpr const char* EVENT_COMBINATIONKEY = "COMBINATIONKEY";
constexpr const char* EVENT_SUBSYSREBOOT = "SUBSYSREBOOT";
constexpr const char* EVENT_POWEROFF = "POWEROFF";
constexpr const char* EVENT_PANIC = "PANIC";
constexpr const char* EVENT_OOPS = "OOPS";
constexpr const char* EVENT_SYS_WATCHDOG = "SYSWATCHDOG";
constexpr const char* EVENT_HUNGTASK = "HUNGTASK";

struct ErrorInfoToCategory {
    const char *module;
    struct {
        const char *event;
        const char *category;
        const char *topCategory;
    } map;
};

struct ErrorInfoToCategory g_errorInfoCategories[] = {
    {
        MODULE_SYSTEM,
        {EVENT_SYSREBOOT, CATEGORY_SYSTEM_REBOOT, TOP_CATEGORY_SYSTEM_RESET}
    },
    {
        MODULE_SYSTEM,
        {EVENT_LONGPRESS, CATEGORY_SYSTEM_REBOOT, TOP_CATEGORY_SYSTEM_RESET}
    },
    {
        MODULE_SYSTEM,
        {EVENT_COMBINATIONKEY, CATEGORY_SYSTEM_REBOOT, TOP_CATEGORY_SYSTEM_RESET}
    },
    {
        MODULE_SYSTEM,
        {EVENT_SUBSYSREBOOT, CATEGORY_SYSTEM_REBOOT, TOP_CATEGORY_SYSTEM_RESET}
    },
    {
        MODULE_SYSTEM,
        {EVENT_POWEROFF, CATEGORY_SYSTEM_POWEROFF, TOP_CATEGORY_SYSTEM_POWEROFF}
    },
    {
        MODULE_SYSTEM,
        {EVENT_PANIC, CATEGORY_SYSTEM_PANIC, TOP_CATEGORY_SYSTEM_RESET}
    },
    {
        MODULE_SYSTEM,
        {EVENT_OOPS, CATEGORY_SYSTEM_OOPS, TOP_CATEGORY_SYSTEM_RESET}
    },
    {
        MODULE_SYSTEM,
        {EVENT_SYS_WATCHDOG, CATEGORY_SYSTEM_WATCHDOG, TOP_CATEGORY_FREEZE}
    },
    {
        MODULE_SYSTEM,
        {EVENT_HUNGTASK, CATEGORY_SYSTEM_HUNGTASK, TOP_CATEGORY_FREEZE}
    },
};

void RKTransData(std::string bboxTime, std::string bboxSysreset)
{
    string deviceInfo = system::GetParameter("const.product.devicetype", "");
    if (deviceInfo != "default") {
        return;
    }
    ifstream fin(SYS_FS_PSTORE_PATH);
    if (!fin.is_open()) {
        HIVIEW_LOGE("Failed to open file: %{public}s, error=%{public}d", SYS_FS_PSTORE_PATH, errno);
        return;
    }
    ErrorInfo info = {};
    fin.read(reinterpret_cast<char* >(&info), sizeof(ErrorInfo));
    if (!fin) {
        HIVIEW_LOGE("Read error_info failed");
        return;
    }
    SaveHistoryLog(bboxTime, bboxSysreset, &info);
    CopyPstoreFileToHistoryLog(fin);
}

void SaveHistoryLog(string bboxTime, string bboxSysreset, ErrorInfo* info)
{
    ofstream fout;
    fout.open(HISTORY_LOG_PATH, ios::out | ios::app);
    if (!fout.is_open()) {
        HIVIEW_LOGE("Failed to open file: %{public}s, error=%{public}d", HISTORY_LOG_PATH, errno);
        return;
    }
    fout << "[" << GetTopCategory(info->module, info->event) << "],";
    fout << "module[" << info->module << "],";
    fout << "category[" << GetCategory(info->module, info->event) << "],";
    fout << "event[" << info->event << "],";
    fout << "time[" << bboxTime << "],";
    fout << "sysreboot[" << bboxSysreset << "],";
    fout << "errdesc[" << info->errorDesc << "],";
    fout << "logpath[/data/log/bbox]";
    fout << endl;
    HIVIEW_LOGE("GetTopCategory: %{public}s, module: %{public}s",
        GetTopCategory(info->module, info->event), info->module);
    HIVIEW_LOGE("category: %{public}s, event: %{public}s",
        GetCategory(info->module, info->event), info->event);
    HIVIEW_LOGE("time: %{public}s, sysreboot: %{public}s, errorDesc: %{public}s",
        bboxTime.c_str(), bboxSysreset.c_str(), info->errorDesc);
}

void CopyPstoreFileToHistoryLog(ifstream &fin)
{
    uint64_t startTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    string dirPath = "/data/log/bbox/" +
        TimeUtil::TimestampFormatToDate(startTime, "%Y%m%d-%H%M%S") + "/";
    constexpr mode_t defaultLogDirMode = 0770;
    if (!FileUtil::FileExists(dirPath)) {
        FileUtil::ForceCreateDirectory(dirPath);
        FileUtil::ChangeModeDirectory(dirPath, defaultLogDirMode);
    }
    string targetPath = dirPath + "last_kmsg";
    ofstream fout;
    fout.open(targetPath, ios::in | ios::out | ios::trunc);
    if (!fout.is_open()) {
        HIVIEW_LOGE("Failed to open file: %{public}s error=%{public}d", targetPath.c_str(), errno);
        return;
    }
    fout << fin.rdbuf();
    fout << endl;
}

const char *GetTopCategory(const char *module, const char *event)
{
    int i;
    int count = sizeof(g_errorInfoCategories) / sizeof(ErrorInfoToCategory);
    if ((!module || !event)) {
        HIVIEW_LOGE("module: %{public}p, event: %{public}p\n", module, event);
        return TOP_CATEGORY_SUBSYSTEM_CRASH;
    }
    for (i = 0; i < count; i++) {
        if (!strcmp(g_errorInfoCategories[i].module, module) &&
            !strcmp(g_errorInfoCategories[i].map.event, event)) {
                return g_errorInfoCategories[i].map.topCategory;
            }
    }
    if (!strcmp(module, MODULE_SYSTEM)) {
        return TOP_CATEGORY_SYSTEM_RESET;
    }
    return TOP_CATEGORY_SUBSYSTEM_CRASH;
}

const char *GetCategory(const char *module, const char *event)
{
    int i;
    int count = sizeof(g_errorInfoCategories) / sizeof(ErrorInfoToCategory);

    if ((!module || !event)) {
        HIVIEW_LOGE("module: %{public}p, event: %{public}p\n", module, event);
        return CATEGORY_SUBSYSTEM_CUSTOM;
    }
    for (i = 0; i < count; i++) {
        if (!strcmp(g_errorInfoCategories[i].module, module) &&
            !strcmp(g_errorInfoCategories[i].map.event, event)) {
                return g_errorInfoCategories[i].map.category;
            }
    }
    if (!strcmp(module, MODULE_SYSTEM)) {
        return CATEGORY_SYSTEM_CUSTOM;
    }
    return CATEGORY_SUBSYSTEM_CUSTOM;
}
}
}
}
