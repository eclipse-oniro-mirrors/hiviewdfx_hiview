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
#include "faultloggerservicedumpbyplugin_fuzzer.h"
#include "constants.h"
#include "export_faultlogger_interface.h"
#include "faultlog_info_inner.h"
#include "faultlogger_fuzzertest_common.h"
#include "faultlogger_service_fuzzer_helper.h"
#include "hiview_global.h"
#include "hiview_platform.h"
#include "sys_event.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr int FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH = 50;
}

void FuzzServiceInterfaceDumpByPlugin(const uint8_t* data, size_t size)
{
    static HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);
    auto service = CreateFaultloggerInstance();

    constexpr int maxLen = 20;
    int32_t fd;
    if (size <= (sizeof(fd) + maxLen)) {
        return;
    }

    STREAM_TO_VALUEINFO(data, fd);
    std::vector<std::string> args;
    char arg[maxLen] = {0};
    errno_t err = strncpy_s(reinterpret_cast<char*>(arg), sizeof(arg),
        reinterpret_cast<const char*>(data), maxLen - 1);
    if (err != EOK) {
        return;
    }
    args.push_back(arg);
    service->Dump(fd, args);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzServiceInterfaceDumpByPlugin(data, size);
    return 0;
}
