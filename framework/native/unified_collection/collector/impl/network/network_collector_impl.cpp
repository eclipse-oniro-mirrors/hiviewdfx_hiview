/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "network_collector_impl.h"

#include "hiview_logger.h"
#include "network_decorator.h"
#ifdef COMMUNICATION_WIFI_ENABLE
#include "wifi_device.h"
#endif

using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil");

std::shared_ptr<NetworkCollector> NetworkCollector::Create()
{
    return std::make_shared<NetworkDecorator>(std::make_shared<NetworkCollectorImpl>());
}

#ifdef COMMUNICATION_WIFI_ENABLE
bool GetNetworkInfo(Wifi::WifiLinkedInfo& linkInfo)
{
    std::shared_ptr<Wifi::WifiDevice> wifiDevicePtr = Wifi::WifiDevice::GetInstance(OHOS::WIFI_DEVICE_SYS_ABILITY_ID);
    if (wifiDevicePtr == nullptr) {
        return false;
    }
    bool isActive = false;
    wifiDevicePtr->IsWifiActive(isActive);
    if (!isActive) {
        return false;
    }
    int ret = wifiDevicePtr->GetLinkedInfo(linkInfo);
    if (ret != Wifi::WIFI_OPT_SUCCESS) {
        HIVIEW_LOGE("GetLinkedInfo failed");
        return false;
    } else {
        return true;
    }
}
#endif

CollectResult<NetworkRate> NetworkCollectorImpl::CollectRate()
{
    CollectResult<NetworkRate> result;
#ifdef COMMUNICATION_WIFI_ENABLE
    Wifi::WifiLinkedInfo linkInfo;
    if (GetNetworkInfo(linkInfo)) {
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
#endif
    return result;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
