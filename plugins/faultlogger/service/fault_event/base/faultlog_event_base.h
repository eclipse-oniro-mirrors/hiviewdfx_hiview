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
#ifndef FAULTLOG_EVENT_BASE_H
#define FAULTLOG_EVENT_BASE_H

#include "faultlog_event_interface.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogEventBase : public FaultLogEventInterface {
public:
    ~FaultLogEventBase() override = default;
    bool ProcessFaultLogEvent(std::shared_ptr<Event>& event, const std::shared_ptr<EventLoop>& workLoop,
        const std::shared_ptr<FaultLogManager>& faultLogManager) override;
protected:
    virtual bool ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent, const FaultLogInfo& info) const {return false;}
    virtual std::string GetFaultModule(SysEvent& sysEvent) const = 0;
    virtual void FillSpecificFaultLogInfo(SysEvent& sysEvent, FaultLogInfo& info) const {}
private:
    FaultLogInfo FillFaultLogInfo(SysEvent& sysEvent) const;
    void FillCommonFaultLogInfo(SysEvent& sysEvent, FaultLogInfo& info) const;
    void UpdateSysEvent(SysEvent& sysEvent, FaultLogInfo& info);
    void FillTimestampInfo(const SysEvent& sysEvent, FaultLogInfo& info) const;
protected:
    int32_t faultType_ {0};
    std::shared_ptr<EventLoop> workLoop_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
