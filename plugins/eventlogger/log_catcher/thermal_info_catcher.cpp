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
#include "hiview_logger.h"
#include "thermal_mgr_client.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-ThermalInfoCatcher");

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
    logSize_ = GetFdSize(fd) - originSize;
    if (logSize_ <= 0) {
        FileUtil::SaveStringToFd(fd, "thermalLevel is empty!");
    }
    return logSize_;
}
} // namespace HiviewDFX
} // namespace OHOS
