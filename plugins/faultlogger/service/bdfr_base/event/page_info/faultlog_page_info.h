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
#ifndef FAULTLOG_PAGE_INFO_H
#define FAULTLOG_PAGE_INFO_H

#include "faultlog_event_pipeline.h"
#include "event.h"
#include "sys_event.h"
#include "faultlog_manager.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogPageInfo final : public FaultLogEventPipeline {
public:
    FaultLogPageInfo();
    bool AddFaultLog(std::shared_ptr<Event> &event) final;

private:
    void UpdateFaultLogInfo() override { return; }
    bool ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent) const override { return false; };
    std::string GetFaultModule(SysEvent& sysEvent) const override;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
