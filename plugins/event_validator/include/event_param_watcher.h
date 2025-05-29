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

#ifndef HIVIEW_PLUGINS_EVENT_PARAM_WATCHER_H
#define HIVIEW_PLUGINS_EVENT_PARAM_WATCHER_H

#include <shared_mutex>
#include <string>

namespace OHOS {
namespace HiviewDFX {
class EventParamWatcher {
public:
    void Init();
    std::string GetTestType();
    void UpdateTestType(const std::string& testType);

private:
    std::string testType_;
    mutable std::shared_mutex testTypeMutex_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_EVENT_PARAM_WATCHER_H
