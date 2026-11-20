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
#include "xperf_audio_listener.h"
#include "xperf_service_log.h"
#include "video_play_state_monitor.h"

namespace OHOS {
namespace HiviewDFX {

void XperfAudioListener::OnPreferredOutputDeviceUpdated(
    const std::vector<std::shared_ptr<AudioStandard::AudioDeviceDescriptor>> &deviceDescriptor)
{
    if (deviceDescriptor.size() == 0) {
        return;
    }
    switch (deviceDescriptor[0]->deviceType_) {
        case AudioStandard::DEVICE_TYPE_SPEAKER:
            if (deviceDescriptor[0]->networkId_ == "LocalDevice" &&
                outputDeviceType_ == AudioStandard::DeviceType::DEVICE_TYPE_BLUETOOTH_A2DP) {
                VideoPlayStateMonitor::GetInstance().OnAudioOutputDeviceChange(B2S);
            }
            break;
        case AudioStandard::DeviceType::DEVICE_TYPE_BLUETOOTH_A2DP:
            VideoPlayStateMonitor::GetInstance().OnAudioOutputDeviceChange(S2B);
            break;
        default:
            break;
    }
    outputDeviceType_ = deviceDescriptor[0]->deviceType_;
}

void XperfAudioListener::OnAudioFocusInfoChange(
    const std::list<std::pair<AudioStandard::AudioInterrupt, AudioStandard::AudioFocuState>>& infoList)
{
}

void XperfAudioListener::OnAudioFocusRequested(const AudioStandard::AudioInterrupt& requestedFocus)
{
    VideoPlayStateMonitor::GetInstance().OnAudioFocusInfoChange(requestedFocus.bundleName, requestedFocus.pid);
}

void XperfAudioListener::OnAudioFocusAbandoned(const AudioStandard::AudioInterrupt &abandonFocus)
{
}
} // namespace HiviewDFX
} // namespace OHOS
