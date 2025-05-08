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

#ifndef HIVIEW_BASE_EVENT_EXPORT_CACHED_EVENT_H
#define HIVIEW_BASE_EVENT_EXPORT_CACHED_EVENT_H

#include <string>

namespace OHOS {
namespace HiviewDFX {
struct EventVersion {
    // system version
    std::string systemVersion;

    // patch version
    std::string patchVersion;
};

struct CachedEvent {
    // event version
    EventVersion version;

    // event domain
    std::string domain;

    // event name
    std::string name;

    // event json string
    std::string eventStr;

    // transformed uid
    int32_t uid;

    CachedEvent(const EventVersion& version, const std::string& domain, const std::string& name,
        const std::string& eventStr, int32_t uid) : version(version), domain(domain), name(name),
        eventStr(eventStr), uid(uid) {}
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_CACHED_EVENT_H