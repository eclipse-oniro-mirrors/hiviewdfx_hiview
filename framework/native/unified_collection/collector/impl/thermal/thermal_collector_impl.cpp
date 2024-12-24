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

#include "thermal_collector_impl.h"

#include <string>
#include <map>

#include "common_util.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "thermal_decorator.h"
#ifdef THERMAL_MANAGER_ENABLE
#include "thermal_mgr_client.h"
#endif

using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("ThermalCollector");
const std::string THERMAL_PATH = "/sys/class/thermal/";

std::string GetThermalType(const std::string& thermalZone)
{
    std::string tmp;
    if (!FileUtil::LoadStringFromFile(thermalZone + "/type", tmp)) {
        HIVIEW_LOGW("read node failed");
        return "";
    }
    std::stringstream ss(tmp); // tmp str load from node may have a newline character at the end
    std::string type;
    ss >> type; // type str without newline character
    return type;
}

int32_t GetThermalValue(const std::string& thermalZone)
{
    return CommonUtil::ReadNodeWithOnlyNumber(thermalZone + "/temp");
}

std::string GetZoneTypeStr(ThermalZone thermalZone)
{
    const std::map<ThermalZone, std::string> thermalZoneMap = {
        {SHELL_FRONT, "shell_front"},
        {SHELL_FRAME, "shell_frame"},
        {SHELL_BACK, "shell_back"},
        {SOC_THERMAL, "soc_thermal"},
        {SYSTEM, "system_h"}
    };
    auto it = thermalZoneMap.find(thermalZone);
    return it != thermalZoneMap.end() ? it->second : "";
}

std::string GetZonePathByType(const std::vector<std::string>& thermalZones, const std::string& zoneTypeStr)
{
    for (const auto& zone : thermalZones) {
        if (zoneTypeStr == GetThermalType(zone)) {
            return zone;
        }
    }
    return "";
}
}

std::shared_ptr<ThermalCollector> ThermalCollector::Create()
{
    return std::make_shared<ThermalDecorator>(std::make_shared<ThermalCollectorImpl>());
}

CollectResult<int32_t> ThermalCollectorImpl::CollectDevThermal(ThermalZone thermalZone)
{
    CollectResult<int32_t> result;
    std::string zoneTypeStr = GetZoneTypeStr(thermalZone);
    if (zoneTypeStr.empty()) {
        return result;
    }
    std::vector<std::string> thermalZones;
    FileUtil::GetDirFiles(THERMAL_PATH, thermalZones);
    std::string zonePath = GetZonePathByType(thermalZones, zoneTypeStr);
    if (zonePath.empty()) {
        return result;
    }
    result.data = GetThermalValue(zonePath);
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<uint32_t> ThermalCollectorImpl::CollectThermaLevel()
{
    CollectResult<uint32_t> result;
#ifdef THERMAL_MANAGER_ENABLE
    auto& thermalMgrClient = PowerMgr::ThermalMgrClient::GetInstance();
    PowerMgr::ThermalLevel thermalLevel = thermalMgrClient.GetThermalLevel();
    result.retCode = UcError::SUCCESS;
    result.data = static_cast<uint32_t>(thermalLevel);
#endif
    return result;
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
