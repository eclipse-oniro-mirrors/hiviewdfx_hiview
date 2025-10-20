/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportConfig");
namespace {
constexpr char EXPORT_CFG_FILE_NAME_SUFFIX[] = "_event_export_config.json";

std::string GetExportConfigDir()
{
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        return "";
    }
    std::string configDir = context->GetHiViewDirectory(HiviewContext::DirectoryType::CONFIG_DIRECTORY);
    return FileUtil::IncludeTrailingPathDelimiter(configDir.append("sys_event_export"));
}
}

ExportConfigManager& ExportConfigManager::GetInstance()
{
    static ExportConfigManager instance;
    return instance;
}

ExportConfigManager::ExportConfigManager()
{
    Init();
}

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

void ExportConfigManager::GetPeriodicExportConfigs(std::vector<std::shared_ptr<ExportConfig>>& configs) const
{
    for (auto& config : exportConfigs_) {
        if (config.second->taskCycle <= 0) {
            continue;
        }
        configs.emplace_back(config.second);
    }
}

void ExportConfigManager::GetTriggerExportConfigs(std::vector<std::shared_ptr<ExportConfig>>& configs) const
{
    for (auto& config : exportConfigs_) {
        if (config.second->taskTriggerCycle <= 0) {
            continue;
        }
        configs.emplace_back(config.second);
    }
}

std::shared_ptr<ExportConfig> ExportConfigManager::GetExportConfig(const std::string& moduleName) const
{
    auto iter = exportConfigs_.find(moduleName);
    if (iter == exportConfigs_.end()) {
        return nullptr;
    }
    return iter->second;
}

void ExportConfigManager::Init()
{
    std::string configDir = GetExportConfigDir();
    if (configDir.empty()) {
        return;
    }
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
    std::string cfgFileName = FileUtil::ExtractFileName(configFile);
    if (!StringUtil::EndWith(cfgFileName, EXPORT_CFG_FILE_NAME_SUFFIX)) {
        HIVIEW_LOGW("config file name is invalid, file=%{public}s.", cfgFileName.c_str());
        return;
    }
    std::string moduleName = StringUtil::EraseString(cfgFileName, EXPORT_CFG_FILE_NAME_SUFFIX);
    auto config = GetExportConfig(moduleName);
    if (config != nullptr) {
        HIVIEW_LOGW("this config file of %{public}s module has been parsed", moduleName.c_str());
        return;
    }
    ExportConfigParser parser(configFile, moduleName);
    config = parser.Parse();
    if (config == nullptr || config->taskType == INVALID_TASK_TYPE) {
        HIVIEW_LOGE("failed to parse or parsed config is invalid from file: %{public}s", configFile.c_str());
        return;
    }
    exportConfigs_.emplace(moduleName, config);
}
} // HiviewDFX
} // OHOS