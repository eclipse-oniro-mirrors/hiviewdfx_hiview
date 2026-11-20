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
#ifndef EVENT_BRIDGE_H
#define EVENT_BRIDGE_H

#include <string>
#include <vector>
#include <functional>

namespace OHOS {
namespace HiviewDFX {

class EventBridge {
public:
    static EventBridge& GetInstance();
    EventBridge(const EventBridge&) = delete;
    void operator=(const EventBridge&) = delete;

    void OnEvent(const std::string& domain, const std::string& event, const std::string& data);
    void OnEvent(const std::string& event, const std::string& data);

    void RegEventListener(std::function<void(const std::string&, const std::string&, const std::string&)> listener);

private:
    EventBridge() = default;
    ~EventBridge() = default;

    std::vector<std::function<void(const std::string&, const std::string&, const std::string&)>> listeners;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif