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
#include "thermal_info_catcher.h"

#include "file_util.h"
#include "freeze_common.h"
#include "thermal_mgr_client.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr int TEMP_EVENT_LIMIT = 5;
}

ThermalInfoCatcher::ThermalInfoCatcher() : EventLogCatcher()
{
    name_ = "ThermalInfoCatcher";
}

bool ThermalInfoCatcher::Initialize(const std::string& strParam1, int intParam1, int intParam2)
{
    // this catcher do not need parameters, just return true
    description_ = "ThermalInfoCatcher --";
    return true;
}

int ThermalInfoCatcher::Catch(int fd, int jsonFd)
{
    int originSize = GetFdSize(fd);
    PowerMgr::ThermalLevel temp = PowerMgr::ThermalMgrClient::GetInstance().GetThermalLevel();
    int tempNum = static_cast<int>(temp);
    FileUtil::SaveStringToFd(fd, "\nThermalLevel info: " + std::to_string(tempNum) + "\n");
    if (tempNum >= TEMP_EVENT_LIMIT && event_) {
        event_->SetEventValue(FreezeCommon::HOST_RESOURCE_WARNING, "Yes");
    }

    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}

void ThermalInfoCatcher::SetEvent(std::shared_ptr<SysEvent> event)
{
    event_ = event;
}
} // namespace HiviewDFX
} // namespace OHOS
