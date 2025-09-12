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

#ifndef HIVIEW_BASE_EVENT_EXPORT_CONFIG_MGR_H
#define HIVIEW_BASE_EVENT_EXPORT_CONFIG_MGR_H

#include <map>
#include <memory>
#include <string>

#include "export_config_parser.h"

namespace OHOS {
namespace HiviewDFX {
class ExportConfigManager {
public:
    ExportConfigManager(const std::string& configDir)
    {
        Init(configDir);
    }

public:
    void GetModuleNames(std::vector<std::string>& moduelNames) const;
    void GetExportConfigs(std::vector<std::shared_ptr<ExportConfig>>& exportConfigs) const;
    std::shared_ptr<ExportConfig> GetExportConfig(const std::string& moduleName) const;

private:
    void Init(const std::string& configDir);
    void ParseConfigFiles(const std::vector<std::string>& configFiles);
    void ParseConfigFile(const std::string& configFile);

private:
    std::map<std::string, std::shared_ptr<ExportConfig>> exportConfigs_;
};
} // HiviewDFX
} // OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_CONFIG_MGR_H
