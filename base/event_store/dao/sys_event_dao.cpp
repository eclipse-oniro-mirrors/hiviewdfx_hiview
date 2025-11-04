/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "sys_event_dao.h"

#include <cinttypes>

#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "sys_event_backup.h"
#include "sys_event_database.h"
#include "sys_event_query_wrapper.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
DEFINE_LOG_TAG("HiView-SysEventDao");
constexpr char BACKUP_DIR[] = "/log/hiview/backup/";
constexpr char LOG_HIVIEW_DIR[] = "/log/hiview/";
constexpr char DIRTY_EVENT_CLEAR_FLAG_PATH[] = "/log/hiview/dirty_event_clear_flag";
constexpr char DIRTY_EVENT_CLEARED_PROP[] = "sys.hiview.diag.dirty_event_cleared";
static std::mutex g_mutex;

std::shared_ptr<SysEventQuery> SysEventDao::BuildQuery(const std::string& domain,
    const std::vector<std::string>& names)
{
    HIVIEW_LOGD("squery domain=%{public}s, names.size=%{public}zu", domain.c_str(), names.size());
    if (domain.empty() || names.empty()) {
        return nullptr;
    }
    return std::make_shared<SysEventQueryWrapper>(domain, names);
}

std::shared_ptr<SysEventQuery> SysEventDao::BuildQuery(const std::string& domain,
    const std::vector<std::string>& names, uint32_t type, int64_t toSeq, int64_t fromSeq)
{
    return BuildQuery(domain, names, type, QueryExtraInfo { toSeq, fromSeq, DEFAULT_REPORT_INTERVAL });
}

std::shared_ptr<SysEventQuery> SysEventDao::BuildQuery(const std::string& domain,
    const std::vector<std::string>& names, uint32_t type, QueryExtraInfo info)
{
    HIVIEW_LOGD("query domain=%{public}s, names.size=%{public}zu, type=%{public}u, toSeq=%{public}" PRId64
        ",  fromSeq=%{public}" PRId64 ", reportInterval=%{public}" PRId16 ".", domain.c_str(), names.size(), type,
        info.toSeq, info.fromSeq, info.reportInterval);
    return std::make_shared<SysEventQueryWrapper>(domain, names, type, info);
}

int SysEventDao::Insert(std::shared_ptr<SysEvent> sysEvent)
{
    return SysEventDatabase::GetInstance().Insert(sysEvent);
}

void SysEventDao::CheckRepeat(SysEvent& event)
{
    SysEventDatabase::GetInstance().CheckRepeat(event);
}

void SysEventDao::Backup()
{
    // If there are two tasks at the same time, backup only once.
    if (g_mutex.try_lock()) {
        SysEventBackup backup(BACKUP_DIR);
        backup.Backup();
        g_mutex.unlock();
    } else {
        // Waiting for another backup task to complete.
        g_mutex.lock();
        g_mutex.unlock();
    }
}

void SysEventDao::Restore()
{
    SysEventBackup backup(BACKUP_DIR);
    backup.Restore(GetDatabaseDir());
}

void SysEventDao::ClearDirtyEventFiles()
{
    if (!FileUtil::FileExists(LOG_HIVIEW_DIR)) {
        int writeEventRet = HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, "DIRTY_EVENT_CLEAR_RESULT",
            HiSysEvent::EventType::STATISTIC, "CLEAR_RESULT", "no hiview dir");
        HIVIEW_LOGW("no hiview dir in log partition, writeEventRet: %{public}d.", writeEventRet);
        return;
    }
    if (FileUtil::FileExists(DIRTY_EVENT_CLEAR_FLAG_PATH)) {
        HIVIEW_LOGI("already cleared.");
        Parameter::SetProperty(DIRTY_EVENT_CLEARED_PROP, "true");
        return;
    }
    SysEventBackup backup(BACKUP_DIR);
    std::string clearResult = backup.ClearDirtyEventFiles(GetDatabaseDir());
    int createFlagRet = FileUtil::CreateFile(DIRTY_EVENT_CLEAR_FLAG_PATH);
    Parameter::SetProperty(DIRTY_EVENT_CLEARED_PROP, "true");
    int writeEventRet = HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, "DIRTY_EVENT_CLEAR_RESULT",
        HiSysEvent::EventType::STATISTIC, "CLEAR_RESULT", clearResult, "FLAG_CREATE_RESULT", createFlagRet);
    HIVIEW_LOGI("clear event, createFlagRet: %{public}d, writeEventRet: %{public}d", createFlagRet, writeEventRet);
}

void SysEventDao::Clear()
{
    SysEventDatabase::GetInstance().Clear();
}

std::string SysEventDao::GetDatabaseDir()
{
    return SysEventDatabase::GetInstance().GetDatabaseDir();
}
} // EventStore
} // namespace HiviewDFX
} // namespace OHOS
