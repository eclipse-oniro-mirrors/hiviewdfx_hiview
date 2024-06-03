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

#include "export_config_manager.h"

#include <regex>

#include "file_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-ExportConfigManager");
void ExportConfigManager::GetModuleNames(std::vector<std::string>& moduleNames) const
{
    if (exportConfigs_.empty()) {
        HIVIEW_LOGW("no module name found.");
        return;
    }
    for (auto& config : exportConfigs_) {
        moduleNames.emplace_back(config.first);
    }
}

void ExportConfigManager::GetExportConfigs(std::vector<std::shared_ptr<ExportConfig>>& exportConfigs) const
{
    for (auto& config : exportConfigs_) {
        exportConfigs.emplace_back(config.second);
    }
}

std::shared_ptr<ExportConfig> ExportConfigManager::GetExportConfig(const std::string& moduleName) const
{
    auto iter = exportConfigs_.find(moduleName);
    if (iter == exportConfigs_.end()) {
        HIVIEW_LOGW("no export config found.");
        return nullptr;
    }
    return iter->second;
}

void ExportConfigManager::Init(const std::string& configDir)
{
    HIVIEW_LOGD("configuration file directory is %{public}s.", configDir.c_str());
    if (!FileUtil::IsLegalPath(configDir) || !FileUtil::FileExists(configDir)) {
        HIVIEW_LOGW("configuration file directory is invalid, dir: %{public}s.", configDir.c_str());
        return;
    }
    std::vector<std::string> configFiles;
    FileUtil::GetDirFiles(configDir, configFiles);
    if (configFiles.empty()) {
        HIVIEW_LOGW("no event export configuration file found.");
        return;
    }
    ParseConfigFiles(configFiles);
}

void ExportConfigManager::ParseConfigFiles(const std::vector<std::string>& configFiles)
{
    for (auto& configFile : configFiles) {
        ParseConfigFile(configFile);
    }
}

void ExportConfigManager::ParseConfigFile(const std::string& configFile)
{
    // module name consists of only lowercase letter and underline.
    // eg. module name parsed from 'hiview_event_export_config.json' is 'hiview'
    std::regex reg { ".*/([a-z_]+)_event_export_config.json$" };
    std::smatch match;
    if (!std::regex_match(configFile, match, reg)) {
        HIVIEW_LOGW("config file name is invalid, file=%{public}s.", configFile.c_str());
        return;
    }
    std::string moduleName = match[1].str();
    auto config = GetExportConfig(moduleName);
    if (config != nullptr) {
        HIVIEW_LOGW("this config file of %{public}s module has been parsed", moduleName.c_str());
        return;
    }
    ExportConfigParser parser(configFile);
    config = parser.Parse();
    if (config == nullptr) {
        HIVIEW_LOGE("failed to parse config file, file=%{public}s.", configFile.c_str());
        return;
    }
    config->moduleName = moduleName;
    exportConfigs_.emplace(moduleName, config);
}
} // HiviewDFX
} // OHOS