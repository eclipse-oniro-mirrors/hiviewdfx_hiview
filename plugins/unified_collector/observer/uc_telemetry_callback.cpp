/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "uc_telemetry_callback.h"

#include <chrono>

#include "ffrt.h"
#include "hisysevent.h"
#include "parameter_ex.h"
#include "time_util.h"
#include "trace_flow_controller.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "common_utils.h"

namespace OHOS::HiviewDFX {
DEFINE_LOG_TAG("UcTelemetryCallback");
using namespace UCollectUtil;
namespace {
constexpr char TELEMETRY_DOMAIN[] = "TELEMETRY";
constexpr char LISTENER_KEY[] = "TelemetryCallback";

void OnSaParamChanged(const char *key, const char *value, void *context)
{
    if (key == nullptr || value == nullptr) {
        HIVIEW_LOGW("invalid input");
        return;
    }
    auto pid = static_cast<pid_t>(StringUtil::StrToInt(value));
    if (pid < 0) {
        HIVIEW_LOGW("pid get error");
        return;
    }
    HIVIEW_LOGI("set new pid %{public}d", pid);
    TraceStateMachine::GetInstance().SetFilterPidInfo(pid);
}

void HandTimeout()
{
    // Avoid died lock
    ffrt::submit([] {
        TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    });
}
}

void UcTelemetryCallback::OnTelemetryStart()
{
    HiSysEventWrite(TELEMETRY_DOMAIN, "TASK_INFO", HiSysEvent::EventType::STATISTIC,
        "ID", telemetryId_,
        "STAGE", "TRACE_BEGIN");
    HIVIEW_LOGI("telemetry start");

    // set app bundle names to kernel
    if (!appFilterName_.empty()) {
        int32_t ret = TraceStateMachine::GetInstance().SetAppFilterInfo(appFilterName_);
        auto pid = CommonUtils::GetPidByName(appFilterName_);
        HIVIEW_LOGI("set app package:%{public}s ret:%{public}d pid:%{public}d", appFilterName_.c_str(), ret, pid);
        if (pid > 0) {
            int32_t pidRet = TraceStateMachine::GetInstance().SetFilterPidInfo(pid);
            HIVIEW_LOGI("app started ,set app pid:%{public}d ret:%{public}d", pid, pidRet);
        }
    }
    if (saParams_.empty()) {
        return;
    }

    // set sa pids to kernel
    for (const auto &param : saParams_) {
        Parameter::WatchParamChange(param.c_str(), OnSaParamChanged, nullptr);
        auto pid = static_cast<pid_t>(Parameter::GetInteger(param, -1));
        if (pid < 0) {
            HIVIEW_LOGE("pid invalid:%{public}s", param.c_str());
            continue;
        }
        if (int32_t ret = TraceStateMachine::GetInstance().SetFilterPidInfo(pid); ret != 0) {
            HIVIEW_LOGE("SetFilterPidInfo failed %{public}s ret:%{public}d", param.c_str(), ret);
        }
    }
}

void UcTelemetryCallback::OnTelemetryFinish()
{
    HIVIEW_LOGI("telemetry finish");
    HiSysEventWrite(TELEMETRY_DOMAIN, "TASK_INFO", HiSysEvent::EventType::STATISTIC,
        "ID", telemetryId_,
        "STAGE", "TRACE_END");
    if (saParams_.empty()) {
        return;
    }
    for (const auto &param : saParams_) {
        Parameter::RemoveParameterWatcherEx(param.c_str(), OnSaParamChanged, nullptr);
    }
    std::lock_guard<ffrt::mutex> lock(timeMutex_);
    isTraceOn_ = false;
}

bool UcTelemetryCallback::UpdateAndCheckTimeOut(int64_t timeCost)
{
    TraceFlowController controller(BusinessName::TELEMETRY);
    int64_t traceOnTime = 0;
    if (!controller.QueryRunningTime(traceOnTime) || traceOnTime < 0) {
        HIVIEW_LOGE("QueryTraceOnTime error");
        return false;
    }
    traceOnTime += timeCost;
    if (!controller.UpdateRunningTime(traceOnTime)) {
        HIVIEW_LOGE("UpdateTraceOnTime error");
        return false;
    }
    if (traceOnTime >= traceDuration_) {
        HIVIEW_LOGI("timeout traceOnTime:%{public}" PRId64 " traceDuration:%{public}" PRId64 "", traceOnTime,
            traceDuration_);
        return false;
    }
    return true;
}

void UcTelemetryCallback::RunTraceOnTimeTask()
{
    while (true) {
        ffrt::this_task::sleep_for(std::chrono::seconds(60)); // 60s: collect period
        std::lock_guard<ffrt::mutex> lock(timeMutex_);
        if (!isTraceOn_) {
            HIVIEW_LOGE("exit RunTraceOnTime task");
            isTaskOn_ = false;
            break;
        }
        auto timeCost = TimeUtil::GetBootTimeMs() - traceOnStartTime_;
        traceOnStartTime_ += timeCost;
        if (!UpdateAndCheckTimeOut(static_cast<int64_t>(timeCost))) {
            HandTimeout();
            HIVIEW_LOGW("telemetry time out, exit RunTraceOnTime task");
            isTaskOn_ = false;
            break;
        }
    }
}

void UcTelemetryCallback::OnTelemetryTraceOn()
{
    HIVIEW_LOGI("trace on");
    std::lock_guard<ffrt::mutex> lock(timeMutex_);
    traceOnStartTime_ = TimeUtil::GetBootTimeMs();
    isTraceOn_ = true;
    if (isTaskOn_) {
        HIVIEW_LOGW("old task still running do not need start again");
        return;
    }

    // start new task
    auto task = [this] { this->RunTraceOnTimeTask(); };
    ffrt::submit(task, {}, {}, ffrt::task_attr().name("telemetry_trace_on").qos(ffrt::qos_default));
    isTaskOn_ = true;
}

void UcTelemetryCallback::OnTelemetryTraceOff()
{
    HIVIEW_LOGI("trace off");
    std::lock_guard<ffrt::mutex> lock(timeMutex_);
    isTraceOn_ = false;
    auto timeCost = TimeUtil::GetBootTimeMs() - traceOnStartTime_;
    traceOnStartTime_ += timeCost;
    if (!UpdateAndCheckTimeOut(static_cast<int64_t>(timeCost))) {
        HandTimeout();
    }
}

void PowerCallback::OnTelemetryStart()
{
    UcTelemetryCallback::OnTelemetryStart();
    PowerStatusManager::GetInstance().AddPowerListener(LISTENER_KEY, powerListener_);
    bool isStatusOn = PowerStatusManager::GetInstance().GetPowerState() == UCollectUtil::SCREEN_ON;
    TraceStateMachine::GetInstance().InitTelemetryStatus(isStatusOn);
}

void PowerCallback::OnTelemetryFinish()
{
    UcTelemetryCallback::OnTelemetryFinish();
    PowerStatusManager::GetInstance().RemovePowerListener(LISTENER_KEY);
}

void PowerTelemetryListener::OnScreenOn()
{
    HIVIEW_LOGI("power on");
    auto ret = TraceStateMachine::GetInstance().PowerTelemetryOn();
    if (!ret.IsSuccess()) {
        HIVIEW_LOGW("PowerTelemetryListener failed");
    }
}

void PowerTelemetryListener::OnScreenOff()
{
    HIVIEW_LOGI("power off");
    auto ret = TraceStateMachine::GetInstance().PowerTelemetryOff();
    if (!ret.IsSuccess()) {
        HIVIEW_LOGW("PowerTelemetryListener failed");
    }
}

void ManualCallback::OnTelemetryStart()
{
    UcTelemetryCallback::OnTelemetryStart();
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
}
}