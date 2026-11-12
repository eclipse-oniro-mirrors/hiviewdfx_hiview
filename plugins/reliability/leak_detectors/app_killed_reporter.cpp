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

#include "app_killed_reporter.h"

#include <vector>
#include <string>
#include "fault_common.h"
#include "dfr_util.h"
#include "app_event_handler.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "AppKilledReporter");

void AppKilledReporter::Report(const AKReporter &akReporter)
{
    AppEventHandler::AppKilledInfo info;
    info.uid = akReporter.uid;
    info.bundleName = akReporter.bundleName;
    info.reason = akReporter.reason;
    info.isForeground = akReporter.isForeground;
    info.appRunningUniqueId = akReporter.appRunningUniqueId;
    info.bundleVersion = akReporter.version;
    info.callingPid = akReporter.callingPid;
    info.callingProcessName = akReporter.callingProcessName;
    info.lastExitDetailInfo.pid = akReporter.lastExitDetailInfo.pid;
    info.lastExitDetailInfo.uid = akReporter.lastExitDetailInfo.uid;
    info.lastExitDetailInfo.rss = akReporter.lastExitDetailInfo.rss;
    info.lastExitDetailInfo.pss = akReporter.lastExitDetailInfo.pss;
    info.lastExitDetailInfo.processState = akReporter.lastExitDetailInfo.processState;
    info.lastExitDetailInfo.timestamp = akReporter.lastExitDetailInfo.timestamp;
    info.lastExitDetailInfo.processName = akReporter.lastExitDetailInfo.processName;
    info.lastExitDetailInfo.exitMsg = akReporter.lastExitDetailInfo.exitMsg;
    info.lastExitDetailInfo.killReason = akReporter.lastExitDetailInfo.killReason;

    HIVIEW_LOGI("ready to postAppEvent, uid: %{public}d, bundleName: %{public}s, reason: %{public}s, isForeground: "
        "%{public}d, appRunningUniqueId: %{public}s, bundleVersion: %{public}s", info.uid, info.bundleName.c_str(),
        info.reason.c_str(), info.isForeground, info.appRunningUniqueId.c_str(), info.bundleVersion.c_str());

    bool ret = DfrUtil::GetInstance().PostAppEvent(info);
    if (!ret) {
        HIVIEW_LOGE("post app event failed");
    } else {
        HIVIEW_LOGI("post app event success");
    }
}
}  // HiviewDFX
}  // OHOS
