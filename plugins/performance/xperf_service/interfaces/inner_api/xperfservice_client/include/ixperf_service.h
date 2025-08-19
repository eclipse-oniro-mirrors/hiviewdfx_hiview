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
#ifndef OHOS_HIVIEWDFX_IXPERFSERVICE_H
#define OHOS_HIVIEWDFX_IXPERFSERVICE_H

#include <cstdint>
#include <iremote_broker.h>
#include <string_ex.h>
#include "hilog/log.h"

namespace OHOS {
namespace HiviewDFX {

constexpr int32_t XPERF_SA_ID = 8600;

enum DomainId {
    XPERF = 0,
    NETWORK = 1,
    APP = 2,
    AUDIO = 3,
    AVCODEC = 4,
    RS = 5,
};

enum XperfServiceIpcCode {
    NOTIFY_TO_XPERF = 1,
    REGISTER_VIDEO_JANK,
    UNREGISTER_VIDEO_JANK,
    REGISTER_AUDIO_JANK,
    UNREGISTER_AUDIO_JANK,
};

enum VideoJankIpcCode {
    NOTIFY_VIDEO_JANK = 1,
};

enum AudioJankIpcCode {
    NOTIFY_AUDIO_JANK = 1,
};

typedef enum {
    XPERF_SERVICE_ERR = (-1),
    XPERF_SERVICE_OK = 0,
} XperfServiceBasicErr;

enum XperfServiceErrNo {
    /* Generic error code */
    XPERF_SERVICE_FRAMEWORK_ERR_BASE = (-10000),
    XPERF_SERVICE_DESCRIPTOR_ERR,
    XPERF_SERVICE_GET_PROXY_FAIL,
    XPERF_SERVICE_IPC_WRITE_FAIL,
    XPERF_SERVICE_IPC_READ_FAIL,
    XPERF_SERVICE_IPC_SEND_FAIL,
    XPERF_SERVICE_FUNC_NOT_EXIST,

    XPERF_SERVICE_COMMOM_ERR_BASE = (-9900),
    XPERF_SERVICE_TIMOUT,
    XPERF_SERVICE_INVALID_PARAM,
};

class IVideoJankCallback : public IRemoteBroker {
public:
    IVideoJankCallback() = default;
    ~IVideoJankCallback() override = default;
    virtual void OnVideoJankEvent(const std::string& msg) = 0;
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.HiviewDFX.IVideoJankCallback");
    static constexpr HiLogLabel LABEL = {LOG_CORE, 0xD002D66, "XPERF_SERVICE"};
};

class IAudioJankCallback : public IRemoteBroker {
public:
    IAudioJankCallback() = default;
    ~IAudioJankCallback() override = default;
    virtual void OnAudioJankEvent(const std::string& msg) = 0;
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.HiviewDFX.IAudioJankCallback");
    static constexpr HiLogLabel LABEL = {LOG_CORE, 0xD002D66, "XPERF_SERVICE"};
};

class IXperfService : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.HiviewDFX.IXperfService");

    virtual ErrCode NotifyToXperf(int32_t domainId, int32_t eventId, const std::string& msg) = 0;
    virtual int32_t RegisterVideoJank(const std::string& caller, const sptr<IVideoJankCallback>& cb) = 0;
    virtual int32_t UnregisterVideoJank(const std::string& caller) = 0;
    virtual int32_t RegisterAudioJank(const std::string& caller, const sptr<IAudioJankCallback>& cb) = 0;
    virtual int32_t UnregisterAudioJank(const std::string& caller) = 0;

protected:
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD002D66, "XPERF_SERVICE"};
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // OHOS_HIVIEWDFX_IXPERFSERVICE_H

