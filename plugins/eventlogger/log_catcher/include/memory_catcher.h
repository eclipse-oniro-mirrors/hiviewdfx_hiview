/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef EVENT_LOGGER_MEMORY_CATCHER
#define EVENT_LOGGER_MEMORY_CATCHER

#include <string>

#include "event_log_catcher.h"
namespace OHOS {
namespace HiviewDFX {
class MemoryCatcher : public EventLogCatcher {
public:
    explicit MemoryCatcher();
    ~MemoryCatcher() override{};
    bool Initialize(const std::string& strParam1, int intParam1, int intParam2) override;
    int Catch(int fd, int jsonFd) override;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_MEMORY_CATCHER