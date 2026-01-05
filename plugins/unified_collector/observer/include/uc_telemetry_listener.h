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
#include "ffrt.h"

namespace OHOS::HiviewDFX {
struct TelemetryParams {
    int64_t traceDuration = 3600 * 1000; //ms
    bool isNewTask = false;
    int64_t beginTime = -1;
    std::string telemetryId;
    std::vector<std::string> appFilterNames;
    std::vector<std::string> saParameters;
    std::vector<std::string> traceTag;
    uint32_t bufferSize = 0;
    TelemetryPolicy tracePolicy = TelemetryPolicy::DEFAULT;
    std::map<std::string, int64_t> flowControlQuotas;
};

class TelemetryListener : public EventListener {
public:
    void OnUnorderedEvent(const Event &msg) override;

    std::string GetListenerName() override
    {
        return "TelemetryListener";
    }

private:
    std::string CheckValidParam(const Event &msg, TelemetryParams &params, bool &isCloseMsg);
    void HandleStart(const TelemetryParams &params);
    void HandleStop();
    void WriteErrorEvent(const std::string &error, const TelemetryParams &params);
    bool ProcessTraceTag(const std::string &traceTag, std::vector<std::string> &traceTags, uint32_t &bufferSize);
    bool CheckTelemetryId(const Event &msg, TelemetryParams &params, std::string &errorMsg);
    bool CheckTraceTags(const Event &msg, TelemetryParams &params, std::string &errorMsg);
    bool CheckTracePolicy(const Event &msg, TelemetryParams &params, std::string &errorMsg);
    bool CheckSwitchValid(const Event &msg, bool &isCloseMsg, std::string &errorMsg);
    bool CheckBeginTime(const Event &msg, TelemetryParams &params, std::string &errorMsg);
    bool CheckTimeOut(const Event &msg, TelemetryParams &params, std::string &errorMsg);

private:
    std::atomic<bool> isCanceled_ = false;
    ffrt::mutex telemetryMutex_;
};

}

#endif //HIVIEWDFX_HIVIEW_UC_TELEMETRY_LISTENER_H
