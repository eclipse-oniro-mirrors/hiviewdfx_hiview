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

#ifndef HIVIEW_BASE_EVENT_STORE_UTILITY_EVENT_DOC_WRITER_H
#define HIVIEW_BASE_EVENT_STORE_UTILITY_EVENT_DOC_WRITER_H

#include <memory>
#include <string>

#include "base_def.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
class EventDocWriter {
public:
    EventDocWriter(const std::string& path): docPath_(path) {}
    virtual ~EventDocWriter() {}
    virtual int Write(const std::shared_ptr<SysEvent>& sysEvent) = 0;

protected:
    std::string docPath_;
}; // EventDocWriter
} // EventStore
} // HiviewDFX
} // OHOS
#endif // HIVIEW_BASE_EVENT_STORE_UTILITY_EVENT_DOC_WRITER_H
