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
 
#include "content_reader_factory.h"

namespace OHOS {
namespace HiviewDFX {
void ContentReaderFactory::Register(int8_t version, std::shared_ptr<ContentReader> reader)
{
    readerMap_.insert(std::pair<int8_t, std::shared_ptr<ContentReader>>(version, reader));
}

std::shared_ptr<ContentReader> ContentReaderFactory::Get(int8_t version)
{
    auto itr = readerMap_.find(version);
    if (itr != readerMap_.end()) {
        return itr->second;
    }
    return nullptr;
}
} // HiviewDFX
} // OHOS