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

#ifndef BBOX_DETECTORS_INTERFACE_H
#define BBOX_DETECTORS_INTERFACE_H

#include <memory>

#include "sys_event.h"
#include "bbox_event_recorder.h"

namespace OHOS {
namespace HiviewDFX {
class BBoxDetectorsInterface {
public:
    virtual ~BBoxDetectorsInterface() = default;

    virtual void HandleBBoxEvent(std::shared_ptr<SysEvent> &sysEvent,
                                 std::shared_ptr<BboxEventRecorder> eventRecorder,
                                 bool isLastStartUpShort) = 0;
    virtual void StartBootScan(std::shared_ptr<BboxEventRecorder> eventRecorder) = 0;

    virtual bool InitPanicReport(bool& isLastStartUpShort) = 0;
    virtual bool IsBootCompleted() = 0;
    virtual bool TryToReportRecoveryPanicEvent() = 0;
    virtual void ConfirmReportResult() = 0;
};

extern "C" {
    constexpr const char* const newBBoxDetectorsInterfaceSymbol = "NewBBoxDetectorsInterface";
    typedef BBoxDetectorsInterface* (*NewBBoxDetectorsInterfaceFunc)(void);
    BBoxDetectorsInterface* NewBBoxDetectorsInterface(void);
}
} // namespace HiviewDFX
} // namespace OHOS

#endif // BBOX_DETECTORS_INTERFACE_H
