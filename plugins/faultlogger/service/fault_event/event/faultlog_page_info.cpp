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
#include "faultlog_page_info.h"

#include "constants.h"
#include "page_history_manager.h"
namespace OHOS {
namespace HiviewDFX {
FaultLogPageInfo::FaultLogPageInfo()
{
    faultType_ = FaultLogType::PROCESS_PAGE_INFO;
}

bool FaultLogPageInfo::ProcessFaultLogEvent(std::shared_ptr<Event>& event, const std::shared_ptr<EventLoop>& workLoop,
    const std::shared_ptr<FaultLogManager>& faultLogManager)
{
    auto sysEvent = Event::DownCastTo<SysEvent>(event);
    int pid = static_cast<int>(sysEvent->GetEventIntValue(FaultKey::MODULE_PID));
    std::string processName = sysEvent->GetEventValue("PROCESS_NAME");
    std::string pageInfo = PageHistoryManager::GetInstance().GetPageHistory(processName, pid);
    sysEvent->SetEventValue("PAGE_INFO", pageInfo);
    return true;
}

std::string FaultLogPageInfo::GetFaultModule(SysEvent& sysEvent) const
{
    return sysEvent.GetEventValue(FaultKey::MODULE_NAME);
}
} // namespace HiviewDFX
} // namespace OHOS