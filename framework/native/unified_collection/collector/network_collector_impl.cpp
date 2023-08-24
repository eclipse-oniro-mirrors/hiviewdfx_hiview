/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "network_collector.h"
#include "wifi_device.h"
#include "logger.h"

using std::unique_ptr;

DEFINE_LOG_TAG("UCollectUtil");

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class NetworkCollectorImpl : public NetworkCollector {
public:
    NetworkCollectorImpl() = default;
    virtual ~NetworkCollectorImpl() = default;

public:
    virtual CollectResult<NetworkRate> CollectRate() override;
    virtual CollectResult<NetworkPackets> CollectSysPackets() override;
};

std::shared_ptr<NetworkCollector> NetworkCollector::Create()
{
    return std::make_shared<NetworkCollectorImpl>();
}

CollectResult<NetworkRate> NetworkCollectorImpl::CollectRate()
{
    std::shared_ptr<Wifi::WifiDevice> wifiDevicePtr = Wifi::WifiDevice::GetInstance(OHOS::WIFI_DEVICE_SYS_ABILITY_ID);
    CollectResult<NetworkRate> result;
    bool isActive = false;
    int ret = wifiDevicePtr->IsWifiActive(isActive);
    if (isActive) {
        Wifi::WifiLinkedInfo linkInfo;
        ret = wifiDevicePtr->GetLinkedInfo(linkInfo);
        if (ret != Wifi::WIFI_OPT_SUCCESS) {
            HIVIEW_LOGE("GetLinkedInfo failed");
            result.retCode = UcError::UNSUPPORT;
            return result;
        }
        NetworkRate& networkRate = result.data;
        networkRate.rssi = linkInfo.rssi;
        HIVIEW_LOGD("rssi = %d", networkRate.rssi);
        networkRate.txBitRate = linkInfo.txLinkSpeed;
        HIVIEW_LOGD("txBitRate = %d", networkRate.txBitRate);
        networkRate.rxBitRate = linkInfo.rxLinkSpeed;
        HIVIEW_LOGD("rxBitRate = %d", networkRate.rxBitRate);
        result.retCode = UcError::SUCCESS;
    } else {
        HIVIEW_LOGE("IsWifiActive failed");
        result.retCode = UcError::UNSUPPORT;
    }
    return result;
}

CollectResult<NetworkPackets> NetworkCollectorImpl::CollectSysPackets()
{
    std::shared_ptr<Wifi::WifiDevice> wifiDevicePtr = Wifi::WifiDevice::GetInstance(OHOS::WIFI_DEVICE_SYS_ABILITY_ID);
    CollectResult<NetworkPackets> result;
    bool isActive = false;
    int ret = wifiDevicePtr->IsWifiActive(isActive);
    if (isActive) {
        Wifi::WifiLinkedInfo linkInfo;
        ret = wifiDevicePtr->GetLinkedInfo(linkInfo);
        if (ret != Wifi::WIFI_OPT_SUCCESS) {
            HIVIEW_LOGE("GetLinkedInfo failed");
            result.retCode = UcError::UNSUPPORT;
            return result;
        }
        NetworkPackets& networkPackets = result.data;
        networkPackets.currentSpeed = linkInfo.linkSpeed;
        HIVIEW_LOGD("currentSpeed = %d", networkPackets.currentSpeed);
        networkPackets.currentTxBytes = linkInfo.lastTxPackets;
        HIVIEW_LOGD("currentTxBytes = %d", networkPackets.currentTxBytes);
        networkPackets.currentRxBytes = linkInfo.lastRxPackets;
        HIVIEW_LOGD("currentRxBytes = %d", networkPackets.currentRxBytes);
        result.retCode = UcError::SUCCESS;
    } else {
        HIVIEW_LOGE("IsWifiActive failed");
        result.retCode = UcError::UNSUPPORT;
    }
    return result;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS