/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 
#include "load_complete_reporter.h"
 
#include "hisysevent.h"
#include "hiview_global.h"
#include "sys_event.h"
#include "xperf_service_log.h"
 
namespace OHOS {
namespace HiviewDFX {
 
void LoadCompleteReporter::ReportLoadComplete(const LoadCompleteReport& record)
{
    std::string eventName = "FRAMEWORK_PAGE_ROUTER_EXCEPTION";
    OHOS::HiviewDFX::SysEventCreator sysEventCreator(HiSysEvent::Domain::ACE, eventName,
        OHOS::HiviewDFX::SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("ERROR_TYPE", record.errorType);
    sysEventCreator.SetKeyValue("PACKAGE_NAME", record.packageName);
    sysEventCreator.SetKeyValue("ABILITY_NAME", record.abilityName);
    sysEventCreator.SetKeyValue("PAGE_LOAD_COST", record.pageLoadCost);
 
    auto sysEvent = std::make_shared<SysEvent>(eventName, nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    if (!event) {
        LOGE("[LoadCompleteReporter]ReportLoadComplete dynamic_pointer_cast failed");
        return;
    }
 
    auto& hiviewInstance = OHOS::HiviewDFX::HiviewGlobal::GetInstance();
    if (!hiviewInstance) {
        LOGE("HiviewGlobal::GetInstance failed");
        return;
    }
    if (!hiviewInstance->PostSyncEventToTarget("XperfPlugin", event)) {
        LOGE("hiviewInstance->PostSyncEventToTarget failed");
    }
}
 
} // namespace HiviewDFX
} // namespace OHOS