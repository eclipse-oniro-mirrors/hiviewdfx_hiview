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
#ifndef HIVIEW_BASE_OBJ_SYS_EVENT_H
#define HIVIEW_BASE_OBJ_SYS_EVENT_H

#include <iremote_object.h>
#include "refbase.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {

class ObjSysEvent : public SysEvent {
public:
    ObjSysEvent(const std::string& sender, PipelineEventProducer* handler, SysEventCreator& sysEventCreator);
    ~ObjSysEvent();

public:
    void SetEventObjValue(const sptr<IRemoteObject> obj);
    sptr<IRemoteObject> GetEventObjValue();

private:
    sptr<IRemoteObject> obj_;
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_OBJ_SYS_EVENT_H
    