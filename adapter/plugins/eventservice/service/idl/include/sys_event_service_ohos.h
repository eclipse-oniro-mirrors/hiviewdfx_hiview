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

#ifndef OHOS_HIVIEWDFX_SYS_EVENT_SERVICE_OHOS_H
#define OHOS_HIVIEWDFX_SYS_EVENT_SERVICE_OHOS_H

#include <vector>

#include "iquery_sys_event_callback.h"
#include "isys_event_callback.h"
#include "singleton.h"
#include "sys_event_dao.h"
#include "sys_event_query.h"
#include "sys_event_query_rule.h"
#include "sys_event_rule.h"
#include "sys_event_service.h"
#include "sys_event_service_stub.h"
#include "system_ability.h"

using CallbackObjectOhos = OHOS::sptr<OHOS::IRemoteObject>;
using SysEventCallbackPtrOhos = OHOS::sptr<OHOS::HiviewDFX::ISysEventCallback>;
using SysEventRuleGroupOhos = std::vector<OHOS::HiviewDFX::SysEventRule>;
using QuerySysEventCallbackPtrOhos = OHOS::sptr<OHOS::HiviewDFX::IQuerySysEventCallback>;
using SysEventQueryRuleGroupOhos = std::vector<OHOS::HiviewDFX::SysEventQueryRule>;

namespace OHOS {
namespace HiviewDFX {
class CallbackDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    CallbackDeathRecipient() = default;
    virtual ~CallbackDeathRecipient() = default;
    void OnRemoteDied(const wptr<IRemoteObject> &object) override;
};

class SysEventServiceOhos : public SystemAbility,
                            public SysEventServiceStub,
                            public Singleton<SysEventServiceOhos> {
    DECLARE_SYSTEM_ABILITY(SysEventServiceOhos);
public:
    DISALLOW_COPY_AND_MOVE(SysEventServiceOhos);
    SysEventServiceOhos() : deathRecipient_(new CallbackDeathRecipient()), isDebugMode(false) {};
    virtual ~SysEventServiceOhos() = default;

    static void StartService(OHOS::HiviewDFX::SysEventService* service,
        const OHOS::HiviewDFX::NotifySysEvent notify);
    static OHOS::HiviewDFX::SysEventService* GetSysEventService(
        OHOS::HiviewDFX::SysEventService* service = nullptr);
    void OnSysEvent(std::shared_ptr<OHOS::HiviewDFX::SysEvent>& sysEvent);
    int AddListener(const SysEventRuleGroupOhos& rules, const SysEventCallbackPtrOhos& callback) override;
    void RemoveListener(const SysEventCallbackPtrOhos& callback) override;
    bool QuerySysEvent(int64_t beginTime, int64_t endTime, int32_t maxEvents,
        const SysEventQueryRuleGroupOhos& rules, const QuerySysEventCallbackPtrOhos& callback) override;
    bool SetDebugMode(const SysEventCallbackPtrOhos& callback, bool mode) override;
    void OnRemoteDied(const wptr<IRemoteObject> &remote);

    enum RuleType {
        INVALID_WORD_TYPE = 0,
        FULL_WORD_TYPE,
        PREFIX_WORD_TYPE,
        REGEX_WORD_TYPE
    };

private:
    std::mutex mutex_;
    sptr<CallbackDeathRecipient> deathRecipient_;
    std::map<CallbackObjectOhos, std::pair<int32_t, SysEventRuleGroupOhos>> registeredListeners;
    void QuerySysEventMiddle(int64_t beginTime, int64_t endTime, int32_t maxEvents,
        const SysEventQueryRuleGroupOhos& rules, OHOS::HiviewDFX::EventStore::ResultSet& result);
    int64_t TransSysEvent(OHOS::HiviewDFX::EventStore::ResultSet& result,
        const QuerySysEventCallbackPtrOhos& callback, int64_t& lastRecordTime, int32_t& drops);
    bool IsValidName(const std::string &value, const unsigned int maxSize) const;
    bool CheckDomainEvent(const SysEventQueryRuleGroupOhos& rules) const;
    bool HasAccessPermission() const;

    bool isDebugMode;
    SysEventCallbackPtrOhos debugModeCallback;
    static OHOS::HiviewDFX::NotifySysEvent gISysEventNotify;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_SYS_EVENT_SERVICE_OHOS_H