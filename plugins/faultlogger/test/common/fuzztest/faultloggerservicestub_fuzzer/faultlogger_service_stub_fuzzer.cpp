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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "faultlogger.h"
#include "faultlogger_service_ohos.h"
#include "hiviewfaultlogger_ipc_interface_code.h"
#include "faultlogger_service_stub_fuzzer.h"
#include "constants.h"
#include "export_faultlogger_interface.h"
#include "faultlogger_fuzzertest_common.h"
#include "hiview_global.h"
#include "hiview_platform.h"
#include "sys_event.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t DATA_MIN_SIZE = 2;
}

void FuzzFaultloggerServiceStubInterface(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < DATA_MIN_SIZE) {
        return;
    }

    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService();

    MessageParcel parcel;
    MessageParcel reply;
    MessageOption option;

    parcel.WriteInterfaceToken(serviceOhos.GetDescriptor());
    parcel.WriteBuffer(data, size);

    parcel.RewindRead(0);
    serviceOhos.OnRemoteRequest(
        static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::ADD_FAULTLOG), parcel, reply, option);

    parcel.RewindRead(0);
    serviceOhos.OnRemoteRequest(
        static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::QUERY_SELF_FAULTLOG), parcel, reply, option);

    parcel.RewindRead(0);
    serviceOhos.OnRemoteRequest(
        static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::ENABLE_GWP_ASAN_GRAYSALE), parcel, reply, option);

    parcel.RewindRead(0);
    serviceOhos.OnRemoteRequest(
        static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::DISABLE_GWP_ASAN_GRAYSALE), parcel, reply, option);

    parcel.RewindRead(0);
    serviceOhos.OnRemoteRequest(
        static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::GET_GWP_ASAN_GRAYSALE), parcel, reply, option);

    parcel.RewindRead(0);
    serviceOhos.OnRemoteRequest(
        static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::ENABLE_GWP_ASAN_INNER), parcel, reply, option);

    parcel.RewindRead(0);
    serviceOhos.HandleOtherRemoteRequest(
        static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::ENABLE_GWP_ASAN_INNER), parcel, reply, option);

    parcel.RewindRead(0);
    serviceOhos.HandleEnableGwpAsanGrayscale(parcel, reply);

    parcel.RewindRead(0);
    serviceOhos.HandleDisableGwpAsanGrayscale();

    parcel.RewindRead(0);
    serviceOhos.HandleGetGwpAsanGrayscaleState(reply);

    parcel.RewindRead(0);
    serviceOhos.HandleEnableGwpAsanInner(parcel, reply);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFaultloggerServiceStubInterface(data, size);
    return 0;
}
