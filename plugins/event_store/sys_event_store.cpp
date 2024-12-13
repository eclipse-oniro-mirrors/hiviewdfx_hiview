/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "sys_event_store.h"

#include <cstdio>
#include <memory>

#include "event.h"
#include "event_export_engine.h"
#include "file_util.h"
#include "focused_event_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "hiview_platform.h"
#include "parameter_ex.h"
#include "plugin_factory.h"
#include "sys_event.h"
#include "sys_event_dao.h"
#include "sys_event_db_mgr.h"
#include "sys_event_sequence_mgr.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
REGISTER(SysEventStore);
DEFINE_LOG_TAG("HiView-SysEventStore");
const std::string PROP_LAST_BACKUP = "persist.hiviewdfx.priv.sysevent.backup_time";
}

SysEventStore::SysEventStore() : hasLoaded_(false)
{
    sysEventDbMgr_ = std::make_unique<SysEventDbMgr>();
}

SysEventStore::~SysEventStore() {}

void SysEventStore::OnLoad()
{
    HIVIEW_LOGI("sys event service load");
    sysEventDbMgr_->StartCheckStoreTask(this->workLoop_);

    lastBackupTime_ = Parameter::GetString(PROP_LAST_BACKUP, "");
    EventExportEngine::GetInstance().Start();
    hasLoaded_ = true;
}

void SysEventStore::OnUnload()
{
    HIVIEW_LOGI("sys event service unload");
    EventExportEngine::GetInstance().Stop();
}

bool SysEventStore::IsNeedBackup(const std::string& dateStr)
{
    if (lastBackupTime_ == dateStr) {
        return false;
    }
    if (lastBackupTime_.empty()) {
        // first time boot, no need to backup
        lastBackupTime_ = dateStr;
        Parameter::SetProperty(PROP_LAST_BACKUP, dateStr);
        HIVIEW_LOGI("first time boot, record backup time: %{public}s.", dateStr.c_str());
        return false;
    }
    return true;
}

std::shared_ptr<SysEvent> SysEventStore::Convert2SysEvent(std::shared_ptr<Event>& event)
{
    if (event == nullptr) {
        HIVIEW_LOGE("event is null");
        return nullptr;
    }
    if (event->messageType_ != Event::MessageType::SYS_EVENT) {
        HIVIEW_LOGE("receive out of sys event type");
        return nullptr;
    }
    std::shared_ptr<SysEvent> sysEvent = Event::DownCastTo<SysEvent>(event);
    if (sysEvent == nullptr) {
        HIVIEW_LOGE("sysevent is null");
    }
    return sysEvent;
}

bool SysEventStore::OnEvent(std::shared_ptr<Event>& event)
{
    if (!hasLoaded_) {
        HIVIEW_LOGE("SysEventService not ready");
        return false;
    }

    std::shared_ptr<SysEvent> sysEvent = Convert2SysEvent(event);
    if (sysEvent != nullptr && sysEvent->preserve_) {
        // add seq to sys event and save it to local file
        int64_t eventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
        sysEvent->SetEventSeq(eventSeq);
        if (FocusedEventUtil::IsFocusedEvent(sysEvent->domain_, sysEvent->eventName_)) {
            HIVIEW_LOGI("event[%{public}s|%{public}s|%{public}" PRId64 "] is valid.",
                sysEvent->domain_.c_str(), sysEvent->eventName_.c_str(), eventSeq);
        }
        EventStore::SysEventSequenceManager::GetInstance().SetSequence(++eventSeq);
        sysEventDbMgr_->SaveToStore(sysEvent);

        if (!Parameter::IsOversea()) {
            std::string dateStr(TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y%m%d"));
            if (IsNeedBackup(dateStr)) {
                EventStore::SysEventDao::Backup();
                lastBackupTime_ = dateStr;
                Parameter::SetProperty(PROP_LAST_BACKUP, dateStr);
            }
        }
    }
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
