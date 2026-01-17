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
#ifndef FAULTLOG_EVENT_PIPELINE_H
#define FAULTLOG_EVENT_PIPELINE_H

#include "faultlog_event_interface.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogEventPipeline : public FaultLogEventInterface {
public:
    bool AddFaultLog(std::shared_ptr<Event> &event) override;
    ~FaultLogEventPipeline() override = default;

protected:
    virtual FaultLogInfo FillFaultLogInfo(SysEvent& sysEvent);
    virtual void UpdateSysEvent(SysEvent& sysEvent);
    virtual bool ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent) const = 0;
    virtual std::string GetFaultModule(SysEvent& sysEvent) const = 0;

private:
    static void FillTimestampInfo(const SysEvent& sysEvent, FaultLogInfo& info);

protected:
    int32_t faultType_ {0};
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // FAULTLOG_EVENT_PIPELINE_H
