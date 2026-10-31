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

#ifndef DFR_NOTIFY_LEAK_COMPRESS_COMMON_H
#define DFR_NOTIFY_LEAK_COMPRESS_COMMON_H

#include <vector>
#include <string>

#include "event.h"
#include "fault_info_base.h"
#include "singleton.h"
#include "notify_leak_info.h"
#include "app_mgr_client.h"
#include "ability_manager_client.h"
#include "ability_info.h"


namespace OHOS {
namespace HiviewDFX {

class NotifyLeakCompressCommon : public DelayedRefSingleton<NotifyLeakCompressCommon> {
    DECLARE_DELAYED_REF_SINGLETON(NotifyLeakCompressCommon);
public:
    void NotifyCommonEvent(CommonEventType eventName);
    void MayAddToCompressList(const std::shared_ptr<NotifyLeakInfo> &notifyInfo);
    bool GetScreenOnStatus();
    bool GetPowerConnectStatus();
    bool GetChargeSleepStatus();
    int GetCommonCompressQueueSize();
    std::string GetLeakFileZipPath(const std::shared_ptr<NotifyLeakInfo> &notifyInfo, bool isTelemetry);
    std::string GetLeakFilePath(const std::shared_ptr<NotifyLeakInfo> &notifyInfo);
    void MayStartCompress();
    bool CouldCompressCheck(const std::shared_ptr<NotifyLeakInfo> &notifyInfo);
    void DoCompressDelay(bool isTelemetry);
    void DoScheduleKill();
    void DoScheduleRecovery(std::vector<AAFwk::AbilityRunningInfo> &Infos);
    bool DoCompressFirstItemUnLocked(bool isTelemetry);

private:
    bool isScreenOn_ { false };
    bool isPowerConnected_ { false };
    bool isChargeSleep_ { false };
    ffrt::mutex compressMutex_;
    time_t lastKillTime = 0;
    std::vector<std::shared_ptr<NotifyLeakInfo>> compressList_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NOTIFY_LEAK_COMPRESS_COMMON_H
