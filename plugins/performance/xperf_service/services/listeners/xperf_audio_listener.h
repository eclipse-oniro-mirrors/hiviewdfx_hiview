/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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

#ifndef XPERF_AUDIO_LISTENER_H
#define XPERF_AUDIO_LISTENER_H

#include "audio_routing_client_manager.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace HiviewDFX {

class XperfAudioListener : public AudioStandard::AudioPreferredOutputDeviceChangeCallback,
    public AudioStandard::AudioFocusInfoChangeCallback {
private:
    AudioStandard::DeviceType outputDeviceType_ = AudioStandard::DeviceType::DEVICE_TYPE_DEFAULT;

    void OnPreferredOutputDeviceUpdated(
        const std::vector<std::shared_ptr<AudioStandard::AudioDeviceDescriptor>>& deviceDescriptor) override;
    void OnAudioFocusInfoChange(
        const std::list<std::pair<AudioStandard::AudioInterrupt, AudioStandard::AudioFocuState>>& infoList) override;
    void OnAudioFocusRequested(const AudioStandard::AudioInterrupt& requestedFocus) override;
    void OnAudioFocusAbandoned(const AudioStandard::AudioInterrupt& abandonFocus) override;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif