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
#ifndef HIVIEW_BASE_APP_EVENT_PUBLISHER_FACTORY_H
#define HIVIEW_BASE_APP_EVENT_PUBLISHER_FACTORY_H

#include <string>
#include <unordered_map>

#include "defines.h"

namespace OHOS {
namespace HiviewDFX {
class DllExport AppEventPublisherFactory {
public:
    static void RegisterPublisher(const std::string& name);
    static void UnregisterPublisher(const std::string& name);
    static bool IsPublisher(const std::string& name);

private:
    static std::shared_ptr<std::unordered_map<std::string, bool>> GetPublisherRegistryMap();
};

class PublisherRegister {
public:
    PublisherRegister(const std::string& name)
    {
        AppEventPublisherFactory::RegisterPublisher(name);
    };
    ~PublisherRegister(){};
};

#define REGISTER_PUBLISHER__(ClassName)                                                        \
class RegisterPublisher##ClassName {                                                           \
    private:                                                                                   \
        static const PublisherRegister g_staticPublisherRegister;                              \
};                                                                                             \
const PublisherRegister RegisterPublisher##ClassName::g_staticPublisherRegister(#ClassName);   \

#define PUBLISHER_ASSERT(ClassName)                                                            \
    class AppEventPublisher;                                                                   \
    static_assert((std::is_base_of<AppEventPublisher, ClassName>::value),                      \
        "Not a AppEventPublisher Class");

#define REGISTER_PUBLISHER(ClassName)                                                          \
    PUBLISHER_ASSERT(ClassName)                                                                \
    REGISTER_PUBLISHER__(ClassName)
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_APP_EVENT_PUBLISHER_FACTORY_H