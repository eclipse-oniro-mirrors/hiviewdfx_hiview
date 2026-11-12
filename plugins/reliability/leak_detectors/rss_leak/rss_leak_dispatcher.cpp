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

#include "rss_leak_dispatcher.h"
#include "dfr_util.h"
#include "i_telemetry.h"
#include "rss_system_telemetry_handler.h"
#include "rss_app_telemetry_handler.h"
#include "rss_random_telemetry_handler.h"
#include <climits>

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002DA0, "RssLeakDispatcher");

RssLeakDispatcher::RssLeakDispatcher() {}
RssLeakDispatcher::~RssLeakDispatcher() {}

RssGrayDetectType RssLeakDispatcher::DetectGrayType(pid_t pid, TelemetryLeakInfo& outInfo)
{
    std::string processName = DfrUtil::GetInstance().GetProcessName(pid);
    // first of all, check apptelemetry
    if (!processName.empty()) {
        uint64_t appAtlt = 0;
        int appThreshold = 0;
        DfrUtil::GetInstance().GetAppTelemetryThresholdAndType(processName, appAtlt, &appThreshold);
        SubLeakType appSubLeak = SUB_LEAK_TYPE_MAX;
        if (appAtlt == AppExecFwk::ATLT_RSS_NATIVE_HEAP) {
            appSubLeak = SUB_NATIVE_HEAP_LEAK;
        } else if (appAtlt == AppExecFwk::ATLT_RSS_ARK_TS) {
            appSubLeak = SUB_ARKTS_LEAK;
        } else if (appAtlt == AppExecFwk::ATLT_RSS_KMP_DUMP) {
            appSubLeak = SUB_KMP_LEAK;
        }
        if (appSubLeak != SUB_LEAK_TYPE_MAX) {
            outInfo.leakType = RSS_LEAK_TYPE;
            outInfo.telemetrySource = APP_TELEMETRY;
            outInfo.subLeakType = appSubLeak;
            outInfo.subleakOverThreshold = appThreshold < 0 ? INT_MAX : appThreshold;
            outInfo.bundleName = processName;
            outInfo.pid = pid;
            HIVIEW_LOGI("detected APP_GRAY, pid: %{public}d, bundleName: %{public}s, subLeakType: %{public}d",
                        pid, processName.c_str(), appSubLeak);
            return APP_GRAY_TYPE;
        }
    }

    std::vector<TelemetryLeakInfo> infos = GetTelemetry().GetTelemetryInfos(RSS_LEAK_TYPE);
    if (infos.empty()) {
        HIVIEW_LOGD("no telemetry info for RSS_LEAK_TYPE, pid: %{public}d", pid);
        return NO_GRAY;
    }

    outInfo = infos.front();
    outInfo.pid = pid;
    // Random vs system gray is decided by the quota's own bundleName
    RssGrayDetectType grayType = outInfo.bundleName.empty() ? RANDOM_GRAY_TYPE : SYSTEM_GRAY_TYPE;
    // Random telemetry only supports SUB_NATIVE_HEAP_LEAK
    if (grayType == RANDOM_GRAY_TYPE && outInfo.subLeakType != SUB_NATIVE_HEAP_LEAK) {
        HIVIEW_LOGW("random RSS_LEAK_TYPE only supports SUB_NATIVE_HEAP_LEAK, got %{public}d, skip",
                    outInfo.subLeakType);
        return NO_GRAY;
    }

    outInfo.bundleName = processName;

    if (grayType == RANDOM_GRAY_TYPE) {
        HIVIEW_LOGI("detected RANDOM_GRAY, pid: %{public}d, bundleName: %{public}s",
                    pid, outInfo.bundleName.c_str());
        return RANDOM_GRAY_TYPE;
    }
    HIVIEW_LOGI("detected SYSTEM_GRAY, pid: %{public}d, bundleName: %{public}s",
                pid, outInfo.bundleName.c_str());
    return SYSTEM_GRAY_TYPE;
}

void RssLeakDispatcher::HandleRssThresholdWarningEvent(pid_t pid)
{
    // only enable on commercial
    if (DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        return;
    }
    if (pid <= 0) {
        HIVIEW_LOGW("invalid pid, skip event");
        return;
    }

    TelemetryLeakInfo telemetryInfo;
    RssGrayDetectType grayType = DetectGrayType(pid, telemetryInfo);
    switch (grayType) {
        case RANDOM_GRAY_TYPE:
            RssRandomTelemetryHandler::GetInstance().Handle(pid, telemetryInfo);
            break;
        case SYSTEM_GRAY_TYPE:
            RssSystemTelemetryHandler::GetInstance().Handle(pid, telemetryInfo);
            break;
        case APP_GRAY_TYPE:
            RssAppTelemetryHandler::GetInstance().Handle(pid, telemetryInfo);
            break;
        default:
            HIVIEW_LOGD("pid %{public}d no gray type matched, skip", pid);
            break;
    }
}

void RssLeakDispatcher::NotifyProcessDied(pid_t pid)
{
    RssSystemTelemetryHandler::GetInstance().NotifyProcessDied(pid);
    RssAppTelemetryHandler::GetInstance().NotifyProcessDied(pid);
    RssRandomTelemetryHandler::GetInstance().NotifyProcessDied(pid);
}

} // namespace HiviewDFX
} // namespace OHOS
