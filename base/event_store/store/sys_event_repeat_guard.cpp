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

#include "sys_event_repeat_guard.h"

#include "calc_fingerprint.h"
#include "file_util.h"
#include "ffrt.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "setting_observer_manager.h"
#include "string_util.h"
#include "sys_event_repeat_db.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-SysEvent-Repeat-Guard");
namespace {
constexpr time_t TIME_RANGE_COMMERCIAL = 24 * 60 * 60; // 24h
constexpr time_t TIME_RANGE_BETA = 1 * 60 * 60; // 1h
constexpr char HIVIEW_UE_SWITCH[] = "hiview_ue_switch_enable";
constexpr char KEY_ON[] = "1";
constexpr int REGISTER_RETRY_CNT = 100;
constexpr int REGISTER_LOOP_DURATION = 6;

bool GetShaStr(uint8_t* eventData, std::string& hashStr)
{
    EventRawDataInfo<EventRaw::HiSysEventHeader> eventInfo(eventData);
    if (eventInfo.dataSize <= eventInfo.dataPos) {
        HIVIEW_LOGE("invalid event.");
        return false;
    }
    constexpr int buffLen = SHA256_DIGEST_LENGTH * 2 + 1;
    char buff[buffLen] = {0};
    if (CalcFingerprint::CalcBufferSha(
        eventData + eventInfo.dataPos, eventInfo.dataSize - eventInfo.dataPos, buff, buffLen) != 0) {
        HIVIEW_LOGE("fail to calc sha.");
        return false;
    }
    hashStr = buff;
    return true;
}
}

int64_t SysEventRepeatGuard::GetMinValidTime()
{
    static time_t timeRange = Parameter::IsBetaVersion() ? TIME_RANGE_BETA : TIME_RANGE_COMMERCIAL;
    time_t timeNow = time(nullptr);
    time_t timeMin = timeNow > timeRange ? (timeNow - timeRange) : 0;
    return timeMin;
}

 
void SysEventRepeatGuard::Check(SysEvent& event)
{
    if (event.GetEventType() != SysEventCreator::EventType::FAULT) {
        return;
    }
    if (IsEventRepeat(event)) {
        event.SetLog(LOG_NOT_ALLOW_PACK|LOG_REPEAT);
        return;
    }
    event.SetLog(LOG_ALLOW_PACK|LOG_PACKED);
    return;
}

bool SysEventRepeatGuard::GetEventUniqueId(SysEvent& event, std::string& uniqueId)
{
    uniqueId = event.GetEventValue("FINGERPRINT");
    if (!uniqueId.empty()) {
        return true;
    }
    
    uint8_t* eventData = event.AsRawData();
    if (eventData == nullptr) {
        HIVIEW_LOGE("invalid eventData.");
        return false;
    }
    
    if (!GetShaStr(eventData, uniqueId) || uniqueId.empty()) {
        HIVIEW_LOGE("GetShaStr failed.");
        return false;
    }
    return true;
}

bool SysEventRepeatGuard::IsEventRepeat(SysEvent& event)
{
    SysEventHashRecord sysEventHashRecord(event.domain_, event.eventName_);
    if (!GetEventUniqueId(event, sysEventHashRecord.eventHash)) {
        HIVIEW_LOGE("GetShaStr failed.");
        return false;
    }
    auto happentime = SysEventRepeatDb::GetInstance().QueryHappentime(sysEventHashRecord);
    int64_t minValidTime = GetMinValidTime();
    if (happentime > minValidTime) {    // event repeat
        return true;
    }

    sysEventHashRecord.happentime = time(nullptr);
    if (happentime > 0) {   // > 0 means has record
        SysEventRepeatDb::GetInstance().Update(sysEventHashRecord);
    } else {
        SysEventRepeatDb::GetInstance().Insert(sysEventHashRecord);
        SysEventRepeatDb::GetInstance().CheckAndClearDb(minValidTime);
    }
    return false;
}

void SysEventRepeatGuard::UnregisterListeningUeSwitch()
{
    SettingObserverManager::GetInstance()->UnregisterObserver(HIVIEW_UE_SWITCH);
}

void SysEventRepeatGuard::RegisterListeningUeSwitch()
{
    ffrt::submit([] () {
        SettingObserver::ObserverCallback callback =
        [] (const std::string& paramKey) {
            std::string val = SettingObserverManager::GetInstance()->GetStringValue(paramKey);
            HIVIEW_LOGI("value of param key[%{public}s] is %{public}s", paramKey.c_str(), val.c_str());
            if (val == KEY_ON) {
                int64_t curTime = time(nullptr);
                SysEventRepeatDb::GetInstance().Clear(curTime);
            }
        };
        bool success = false;
        int retryCount = REGISTER_RETRY_CNT;
        while (!success && retryCount > 0) {
            success = SettingObserverManager::GetInstance()->RegisterObserver(HIVIEW_UE_SWITCH, callback);
            if (success) {
                break;
            }
            retryCount--;
            ffrt::this_task::sleep_for(std::chrono::seconds(REGISTER_LOOP_DURATION));
        }
        if (!success) {
            HIVIEW_LOGW("failed to regist setting db observer");
        }
        }, {}, {}, ffrt::task_attr().name("repeat_guard_register").qos(ffrt::qos_default));
}
} // HiviewDFX
} // OHOS