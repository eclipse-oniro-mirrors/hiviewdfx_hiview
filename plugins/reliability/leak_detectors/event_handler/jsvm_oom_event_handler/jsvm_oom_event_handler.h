/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DFR_JSVM_OOM_EVENT_HANDLER_H
#define DFR_JSVM_OOM_EVENT_HANDLER_H

#include "leak_event_handler_base.h"

namespace OHOS {
namespace HiviewDFX {

class JsvmOomEventHandler : public LeakEventHandlerBase {
public:
    JsvmOomEventHandler() = default;
    virtual ~JsvmOomEventHandler() = default;

    std::shared_ptr<LeakEventInfoBase> HandleMessage(SysEvent sysEvent) override;
    ErrCode DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
    ErrCode ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
};

class JsvmOomLeakInfo : public OomLeakInfoBase {
public:
    explicit JsvmOomLeakInfo(const std::string &eventName);
    virtual ~JsvmOomLeakInfo() = default;

    std::string GetFirstFrame();

    void SetDumpJsonHeap(const std::string &heap);
    std::string GetDumpJsonHeap();

private:
    std::string dumpJsonHeap_ { "" };
};

} // HiviewDFX
} // OHOS

#endif // DFR_JSVM_OOM_EVENT_HANDLER_H
