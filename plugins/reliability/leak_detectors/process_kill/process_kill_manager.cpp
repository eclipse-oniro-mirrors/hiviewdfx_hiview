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

#include "process_kill/process_kill_manager.h"
#include "dfr_util.h"

namespace {
    constexpr const char *RELIABILITY_CUSTOMIZE_FAULT = "131072";
    constexpr const char *PROCESS_KILL_TYPE = "2";
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "ProcessKillManager");

ProcessKillManager::ProcessKillManager() {}

ProcessKillManager::~ProcessKillManager() {}

void ProcessKillManager::OnUnorderedEvent(std::map<std::string, std::string> valuePairs)
{
    std::string id = valuePairs["telemetryId"];
    if (id.empty() || valuePairs["fault"] != RELIABILITY_CUSTOMIZE_FAULT ||
        valuePairs["reliabilityType"] != PROCESS_KILL_TYPE) {
        return;
    }

    std::string bundleName = valuePairs["traceAppFilter"];
    std::string reason = valuePairs["reason"];
    std::string status = valuePairs["telemetryStatus"];

    if (bundleName.empty() && reason.empty()) {
        HIVIEW_LOGE("invalid process kill telemetry info, id: %{public}s, bundleName: %{public}s, reason: %{public}s,"
            " status: %{public}s", id.c_str(), bundleName.c_str(), reason.c_str(), status.c_str());
        return;
    }

    HIVIEW_LOGW("telemetry info, id: %{public}s, bundleName: %{public}s, reason: %{public}s, status: %{public}s",
        id.c_str(), bundleName.c_str(), reason.c_str(), status.c_str());

    std::unique_lock<std::shared_mutex> lock(grayscaleMutex_);
    if (status == "off") {
        info_ = {"", "", ""};
    } else if (status == "on") {
        info_ = {id, bundleName, reason};
    } else {
        HIVIEW_LOGE("unknown telemetryStatus, value: %{public}s", status.c_str());
    }
}

ProcessKillTelemetryInfo ProcessKillManager::GetTelemetryInfo()
{
    std::shared_lock<std::shared_mutex> lock(grayscaleMutex_);
    return info_;
}
}
}