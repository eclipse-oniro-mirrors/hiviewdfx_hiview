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

const std::string EVENT_NAME_LOAD_COMPLETE = "LOAD_COMPLETE";
 
void LoadCompleteReporter::ReportLoadComplete(const LoadCompleteReport& record)
{
    OHOS::HiviewDFX::SysEventCreator sysEventCreator("PERFORMANCE", EVENT_NAME_LOAD_COMPLETE,
        OHOS::HiviewDFX::SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("LAST_COMPONENT", record.lastComponent);
    sysEventCreator.SetKeyValue("IS_LAUNCH", record.isLaunch);
    sysEventCreator.SetKeyValue("BUNDLE_NAME", record.bundleName);
    sysEventCreator.SetKeyValue("ABILITY_NAME", record.abilityName);

    auto sysEvent = std::make_shared<SysEvent>(EVENT_NAME_LOAD_COMPLETE, nullptr, sysEventCreator);
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