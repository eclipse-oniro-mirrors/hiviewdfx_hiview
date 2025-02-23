/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef HIVIEWDFX_HIVIEW_UC_TELEMETRY_LISTENER_H
#define HIVIEWDFX_HIVIEW_UC_TELEMETRY_LISTENER_H

#include <map>

#include "event.h"
#include "plugin.h"
#include "trace_flow_controller.h"
#include "trace_state_machine.h"

namespace OHOS::HiviewDFX {
class TelemetryListener : public EventListener {
public:
    explicit TelemetryListener(std::shared_ptr<Plugin> myPlugin) : myPlugin_(myPlugin) {}
    void OnUnorderedEvent(const Event &msg) override;

    std::string GetListenerName() override
    {
        return "TelemetryListener";
    }

private:
    std::weak_ptr<Plugin> myPlugin_;
    uint64_t openTime_ = 0;
    int32_t traceDuration_ = 3600; // seconds
    std::string traceTags_;

    TelemetryFlow InitFlowControlQuotas(const Event &msg);
    void SendStartEvent();
    void SendStopEvent();
};

}

#endif //HIVIEWDFX_HIVIEW_UC_TELEMETRY_LISTENER_H
