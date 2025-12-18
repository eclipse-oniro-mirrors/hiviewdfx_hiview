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
#ifndef FAULTLOG_EVENT_INTERFACE_H
#define FAULTLOG_EVENT_INTERFACE_H

#include "faultlog_info_inner.h"
#include "faultlog_manager.h"
#include "event.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogEventInterface {
public:
    virtual bool AddFaultLog(FaultLogInfo info);
    virtual bool AddFaultLog(std::shared_ptr<Event> &event) { return false;}
    virtual ~FaultLogEventInterface() = default;

protected:
    virtual bool NeedSkip() const {return false;};
    virtual void UpdateFaultLogInfo() = 0;
    virtual bool UpdateCommonInfo();

private:
    void Analysis();
    bool IsFaultTypeSupport() const;
    bool VerifyModule();
    void SaveFaultLogToFile();

protected:
    FaultLogInfo info_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
