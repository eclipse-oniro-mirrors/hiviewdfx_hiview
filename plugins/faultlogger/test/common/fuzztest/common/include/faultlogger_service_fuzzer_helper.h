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

#ifndef FAULTLOGGER_SERVICE_FUZZER_HELPER_H
#define FAULTLOGGER_SERVICE_FUZZER_HELPER_H

#include <memory>
#include <string>
#include "faultlogger.h"
#include "hiview_platform.h"

namespace OHOS {
namespace HiviewDFX {
class HiviewTestContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type __UNUSED)
    {
        return "/data/log/hiview/sys_event_test";
    }
};

inline std::shared_ptr<Faultlogger> CreateFaultloggerInstance()
{
    static std::unique_ptr<HiviewPlatform> platform = std::make_unique<HiviewPlatform>();
    auto plugin = std::make_shared<Faultlogger>();
    plugin->SetName("Faultlogger");
    plugin->SetHandle(nullptr);
    plugin->SetHiviewContext(platform.get());
    plugin->OnLoad();
    return plugin;
}
}
}

#endif
