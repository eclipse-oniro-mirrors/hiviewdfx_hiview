/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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
#include "event_reporter.h"

namespace OHOS {
namespace HiviewDFX {

EventReporter& EventReporter::GetInstance()
{
    static EventReporter instance;
    return instance;
}

void EventReporter::ReportEvent(const std::string& domain, const std::string& event, const std::string& data)
{
    for (auto& func : listeners) {
        func(domain, event, data);
    }
}

void EventReporter::ReportEvent(const std::string& event, const std::string& data)
{
    ReportEvent("PERFORMANCE", event, data);
}

void EventReporter::RegEventListener(
    std::function<void(const std::string&, const std::string&, const std::string&)> listener)
{
    listeners.push_back(std::move(listener));
}
} // namespace HiviewDFX
} // namespace OHOS