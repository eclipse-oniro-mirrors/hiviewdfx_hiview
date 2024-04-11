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

#include "hiview_log_config_manager_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "hiview_log_config_manager.h"
#include "hiview_platform.h"

namespace OHOS {
namespace HiviewDFX {
static void HiviewLogConfigManagerFuzzTest(const uint8_t* data, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    HiviewLogConfigManager hiviewLogConfigManager;
    std::string strData = std::string(reinterpret_cast<const char*>(data), size);
    hiviewLogConfigManager.GetConfigInfoByType(strData);
}
} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::HiviewDFX::HiviewLogConfigManagerFuzzTest(data, size);
    return 0;
}

