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
#ifndef HIVIEW_PLUGIN_BUNDLE_CONFIG_H
#define HIVIEW_PLUGIN_BUNDLE_CONFIG_H

#include <string>
#include <vector>

namespace OHOS {
namespace HiviewDFX {
class PluginBundleConfig {
public:
    PluginBundleConfig (const std::string& configPath);
    ~PluginBundleConfig () = default;

    inline std::vector<std::string> GetBundleNames()
    {
        return bundleNames_;
    }

private:
    void Parse(const std::string& configPath);

private:
    std::vector<std::string> bundleNames_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGIN_BUNDLE_CONFIG_H
