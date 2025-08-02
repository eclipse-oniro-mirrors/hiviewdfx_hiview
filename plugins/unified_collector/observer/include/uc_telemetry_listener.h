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
namespace {
    const int64_t SECONDS_TO_MS = 1000;
    const int64_t MS_TO_US = 1000;
}

struct TelemetryParams {
    int64_t traceDuration = 3600 * SECONDS_TO_MS; //ms
    int64_t beginTime = -1;
    std::string telemetryId;
    std::string appFilterName;
    std::string traceTag;
    std::vector<std::string> saParams;
    TelemetryPolicy tracePolicy;
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
    bool ProcessTraceTag(std::string &traceTag);
    bool CheckTelemetryId(const Event &msg, TelemetryParams &params, std::string &errorMsg);
    bool CheckTraceTags(const Event &msg, TelemetryParams &params, std::string &errorMsg);
    bool CheckTracePolicy(const Event &msg, TelemetryParams &params, std::string &errorMsg);
    bool CheckSwitchValid(const Event &msg, bool &isCloseMsg, std::string &errorMsg);
    bool CheckBeginTime(const Event &msg, TelemetryParams &params, std::string &errorMsg);
    bool InitTelemetryDbData(const Event &msg, bool &isTimeOut, const TelemetryParams &params);
    void GetSaNames(const Event &msg, TelemetryParams &params);

private:
    std::unique_ptr<ffrt::queue> taskQueue_ = nullptr;
    ffrt::task_handle startTaskHandle_;
};

}

#endif //HIVIEWDFX_HIVIEW_UC_TELEMETRY_LISTENER_H
