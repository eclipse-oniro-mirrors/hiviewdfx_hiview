/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEWDFX_SYS_EVENT_SERVICE_ADAPTER_H
#define HIVIEWDFX_SYS_EVENT_SERVICE_ADAPTER_H

#include "idl/type/base_types.h"
#include "event_loop.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
class SysEventServiceAdapter {
public:
    static void StartService(SysEventServiceBase* service);
    static void OnSysEvent(std::shared_ptr<SysEvent>& event);
    static void SetWorkLoop(std::shared_ptr<EventLoop> looper);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEWDFX_SYS_EVENT_SERVICE_ADAPTER_H
