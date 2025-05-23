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
#ifndef HIVIEWDFX_HIVIEW_UC_TELEMETRY_CALLBACK_H
#define HIVIEWDFX_HIVIEW_UC_TELEMETRY_CALLBACK_H
#include "plugin.h"
#include "trace_common.h"
#include "uc_telemetry_listener.h"

namespace OHOS::HiviewDFX {

class UcTelemetryCallback : public TelemetryCallback {
public:
    explicit UcTelemetryCallback(const TelemetryParams &params)
        : traceDuration_(params.traceDuration),
          telemetryId_(params.telemetryId),
          appFilterName_(params.appFilterName),
          saParams_(params.saParams) {}

    void OnTelemetryStart() override;
    void OnTelemetryFinish() override;
    void OnTelemetryTraceOn() override;
    void OnTelemetryTraceOff() override;

protected:
    uint64_t traceOnStartTime_ = 0;
    std::mutex timeMutex_;
    bool isTraceOn_ = false;
    bool isTaskOn_ = false;
    int64_t traceDuration_;
    std::string telemetryId_;
    std::string appFilterName_;
    std::vector<std::string> saParams_;

    void RunTraceOnTimeTask();
    bool UpdateAndCheckTimeOut(int64_t timeCost);
};

class ManualCallback : public UcTelemetryCallback {
public:
    explicit ManualCallback(const TelemetryParams &params) : UcTelemetryCallback(params) {}
    void OnTelemetryStart() override;
};

class PowerTelemetryListener : public PowerListener {
public:
    void OnScreenOn() override;
    void OnScreenOff() override;
};

class PowerCallback : public UcTelemetryCallback {
public:
    explicit PowerCallback(const TelemetryParams &params) : UcTelemetryCallback(params)
    {
        powerListener_ = std::make_shared<PowerTelemetryListener>();
    }
    void OnTelemetryStart() override;
    void OnTelemetryFinish() override;

private:
    std::shared_ptr<PowerListener> powerListener_;
};
}
#endif //HIVIEWDFX_HIVIEW_UC_TELEMETRY_CALLBACK_H
