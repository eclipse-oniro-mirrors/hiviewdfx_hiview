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
#ifndef FAULTLOG_EVENT_IPC_H
#define FAULTLOG_EVENT_IPC_H

#include "faultlog_event_interface.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogEventIpc : public FaultLogEventInterface {
public:
    bool AddFaultLog(FaultLogInfo info) final;
    ~FaultLogEventIpc() override = default;

protected:
    virtual void DoFaultLogLimit(const std::string& logPath) const {}
    virtual bool ReportEventToAppEvent() const { return false; };
    virtual void ReportEventToHiSysEvent() = 0;
};
} // namespace HiviewDFX
} // namespace OHOS

#define EVENT_PARAM_CTOR(pName, pType, uType, pVal, pSize) \
    { \
        .name = (pName), \
        .t = (pType), \
        .v = {.uType = (pVal)}, \
        .arraySize = (pSize) \
    }

#define EVENT_PARAM_STRING(pName, pVal) EVENT_PARAM_CTOR(pName, HISYSEVENT_STRING, s, pVal, 0)

#define EVENT_PARAM_INT32(pName, pVal) EVENT_PARAM_CTOR(pName, HISYSEVENT_INT32, i32, pVal, 0)

#define EVENT_PARAM_INT64(pName, pVal) EVENT_PARAM_CTOR(pName, HISYSEVENT_INT64, i64, pVal, 0)

#endif // FAULTLOG_EVENT_IPC_H
