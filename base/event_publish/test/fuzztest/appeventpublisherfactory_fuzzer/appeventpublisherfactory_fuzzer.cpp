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

#include "app_event_publisher_factory.h"
#include "appeventpublisherfactory_fuzzer.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FUZZTEST_MAX_STR_LEN = 64;

void FuzzPublisherFactory(const uint8_t* data, size_t size)
{
    if (size < FUZZTEST_MAX_STR_LEN) {
        return;
    }
    std::string name(reinterpret_cast<const char*>(data), FUZZTEST_MAX_STR_LEN);
    data += FUZZTEST_MAX_STR_LEN;
    size -= FUZZTEST_MAX_STR_LEN;

    AppEventPublisherFactory::RegisterPublisher(name);
    AppEventPublisherFactory::IsPublisher(name);
    AppEventPublisherFactory::UnregisterPublisher(name);
    AppEventPublisherFactory::IsPublisher(name);

    // exercise empty-name branches
    AppEventPublisherFactory::RegisterPublisher("");
    AppEventPublisherFactory::IsPublisher("");
    AppEventPublisherFactory::UnregisterPublisher("");
}
} // namespace
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FuzzPublisherFactory(data, size);
    return 0;
}

