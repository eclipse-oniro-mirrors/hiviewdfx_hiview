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

#include "export_config_parser.h"

#include <fstream>

#include "cjson_util.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportConfig");
namespace {
constexpr char EXPORT_SWITCH_PARAM_KEY[] = "exportSwitchParam";
constexpr char SYS_UPGRADE_PARAM_KEY[] = "sysUpgradeParam";
constexpr char SETTING_PARAM_NAME_KEY[] = "name";
constexpr char SETTING_PARAM_ENABLED_KEY[] = "enabledValue";
constexpr char EXPORT_DIR[] = "exportDir";
constexpr char EXPORT_DIR_MAX_CAPACITY[] = "exportDirMaxCapacity";
constexpr char EXPORT_SINGLE_FILE_MAX_SIZE[] = "exportSingleFileMaxSize";
constexpr char TASK_EXECUTING_CYCLE[] = "taskExecutingCycle";
constexpr char EXPORT_EVENT_LIST_CONFIG_PATHS[] = "exportEventListConfigPaths";
constexpr char FILE_STORED_MAX_DAY_CNT[] = "fileStoredMaxDayCnt";
constexpr char EXPORT_TASK_TYPE[] = "exportTaskType";
constexpr char INHERITED_MODULE[] = "inheritedModule";
constexpr int32_t INVALID_INT_VAL = -1;

bool ParseIntFromCfg(cJSON* json, std::string& areaTag, std::string& versionTag, int64_t& val)
{
    if (!cJSON_IsObject(json) || !cJSON_HasObjectItem(json, areaTag.c_str())) {
        return false;
    }
    auto areaJson = cJSON_GetObjectItem(json, areaTag.c_str());
    if (!cJSON_IsObject(areaJson) || !cJSON_HasObjectItem(areaJson, versionTag.c_str())) {
        return false;
    }
    auto versionJson = cJSON_GetObjectItem(areaJson, versionTag.c_str());
    if (!cJSON_IsNumber(versionJson)) {
        return false;
    }
    double num = cJSON_GetNumberValue(versionJson);
    if (num < static_cast<double>(std::numeric_limits<int64_t>::lowest()) ||
        num > static_cast<double>(std::numeric_limits<int64_t>::max())) {
        return false;
    }
    val = static_cast<int64_t>(num);
    return true;
}

void RebuildExportDir(std::shared_ptr<ExportConfig> config, bool appendTaskType)
{
    config->exportDir = FileUtil::IncludeTrailingPathDelimiter(config->exportDir);
    config->exportDir = FileUtil::IncludeTrailingPathDelimiter(config->exportDir.append("sys_event_export"));
    if (!appendTaskType) {
        HIVIEW_LOGI("no need to rebuild export dir: %{public}s", config->exportDir.c_str());
        return;
    }
    std::string dirSuffix("0");
    if (config->taskType > ALL_EVENT_TASK_TYPE) {
        dirSuffix = std::to_string(config->taskType);
    }
    config->exportDir = FileUtil::IncludeTrailingPathDelimiter(config->exportDir.append(dirSuffix));
    HIVIEW_LOGI("rebuild export dir to %{public}s", config->exportDir.c_str());
}
}

ExportConfigParser::ExportConfigParser(const std::string& configFile, const std::string& moduleName)
{
    HIVIEW_LOGI("cfg file is %{public}s", configFile.c_str());
    jsonRoot_ = CJsonUtil::ParseJsonRoot(configFile);
    moduleName_ = moduleName;
}

ExportConfigParser::~ExportConfigParser()
{
    if (jsonRoot_ == nullptr) {
        return;
    }
    cJSON_Delete(jsonRoot_);
}

std::shared_ptr<ExportConfig> ExportConfigParser::Parse()
{
    if (jsonRoot_ == nullptr || !cJSON_IsObject(jsonRoot_)) {
        HIVIEW_LOGE("the file format of export config file is not json.");
        return nullptr;
    }
    auto exportConfig = std::make_shared<ExportConfig>();
    exportConfig->moduleName = moduleName_;
    // read event export config files
    CJsonUtil::GetStringArray(jsonRoot_, EXPORT_EVENT_LIST_CONFIG_PATHS, exportConfig->eventsConfigFiles);
    // parse export switch setting parameter
    if (!ParseSettingDbParam(exportConfig->exportSwitchParam, EXPORT_SWITCH_PARAM_KEY)) {
        HIVIEW_LOGE("failed to parse export switch parameter.");
        return nullptr;
    }
    // parse system upgrade setting parameter
    if (!ParseSettingDbParam(exportConfig->sysUpgradeParam, SYS_UPGRADE_PARAM_KEY)) {
        HIVIEW_LOGI("failed to parse system upgrade parameter.");
    }
    // parse residual content of the config file
    if (!ParseResidualContent(exportConfig)) {
        HIVIEW_LOGE("failed to parse residual content.");
        return nullptr;
    }
    return exportConfig;
}

bool ExportConfigParser::ParseSettingDbParam(SettingDbParam& settingDbParam, const std::string& paramKey)
{
    cJSON* settingDbParamJson = cJSON_GetObjectItem(jsonRoot_, paramKey.c_str());
    if (settingDbParamJson == nullptr || !cJSON_IsObject(settingDbParamJson)) {
        HIVIEW_LOGW("settingDbParam configured is invalid.");
        return false;
    }
    settingDbParam.name = CJsonUtil::GetStringValue(settingDbParamJson, SETTING_PARAM_NAME_KEY);
    if (settingDbParam.name.empty()) {
        HIVIEW_LOGW("name of setting db parameter configured is invalid.");
        return false;
    }
    settingDbParam.enabledVal = CJsonUtil::GetStringValue(settingDbParamJson, SETTING_PARAM_ENABLED_KEY);
    if (settingDbParam.enabledVal.empty()) {
        HIVIEW_LOGW("enabled value of setting db parameter configured is invalid.");
        return false;
    }
    return true;
}

bool ExportConfigParser::ParseResidualContent(std::shared_ptr<ExportConfig> config)
{
    // read export diectory
    config->exportDir = CJsonUtil::GetStringValue(jsonRoot_, EXPORT_DIR);
    if (config->exportDir.empty()) {
        HIVIEW_LOGW("exportDirectory configured is invalid.");
        return false;
    }
    // read maximum capacity of the export diectory
    config->maxCapcity = CJsonUtil::GetIntValue(jsonRoot_, EXPORT_DIR_MAX_CAPACITY, INVALID_INT_VAL);
    if (config->maxCapcity == INVALID_INT_VAL) {
        HIVIEW_LOGW("exportDirMaxCapacity configured is invalid.");
        return false;
    }
    // read maximum size of the export single event file
    config->maxSize = CJsonUtil::GetIntValue(jsonRoot_, EXPORT_SINGLE_FILE_MAX_SIZE, INVALID_INT_VAL);
    if (config->maxSize == INVALID_INT_VAL) {
        HIVIEW_LOGW("exportSingleFileMaxSize configured is invalid.");
        return false;
    }
    config->dayCnt = CJsonUtil::GetIntValue(jsonRoot_, FILE_STORED_MAX_DAY_CNT, INVALID_INT_VAL);
    if (config->dayCnt == INVALID_INT_VAL) {
        HIVIEW_LOGW("fileStoredMaxDayCnt configured is invalid.");
        return false;
    }
    config->inheritedModule = CJsonUtil::GetStringValue(jsonRoot_, INHERITED_MODULE);
    if (!ParseTaskType(config) || !ParseTaskExecutingCycle(config)) {
        return false;
    }
    return true;
}

bool ExportConfigParser::ParseTaskType(std::shared_ptr<ExportConfig> config)
{
    auto taskTypeJson = cJSON_GetObjectItem(jsonRoot_, EXPORT_TASK_TYPE);
    if (taskTypeJson == nullptr) {
        // old cfg file
        HIVIEW_LOGI("task type isn't configured for module: %{public}s", config->moduleName.c_str());
        config->taskType = ALL_EVENT_TASK_TYPE;
        RebuildExportDir(config, false);
        return true;
    }
    if (!cJSON_IsObject(taskTypeJson)) {
        return false;
    }
    std::string areaTag(Parameter::IsOversea() ? "oversea" : "domestic");
    std::string versionTag(Parameter::IsBetaVersion() ? "beta" : "commercial");
    int64_t taskType = INVALID_TASK_TYPE;
    if (!ParseIntFromCfg(taskTypeJson, areaTag, versionTag, taskType)) {
        HIVIEW_LOGE("failed to parse task type");
        config->taskType = INVALID_TASK_TYPE;
        return false;
    }
    config->taskType = taskType;
    HIVIEW_LOGI("task type is configured as object for module: %{public}s, value is %{public}" PRId16 "",
        config->moduleName.c_str(), config->taskType);
    config->needPostEvent = true;
    RebuildExportDir(config, true);
    return true;
}

bool ExportConfigParser::ParseTaskExecutingCycle(std::shared_ptr<ExportConfig> config)
{
    auto taskCycleJson = cJSON_GetObjectItem(jsonRoot_, TASK_EXECUTING_CYCLE);
    if (taskCycleJson == nullptr) {
        config->taskCycle = 0;
        return false;
    }
    if (cJSON_IsNumber(taskCycleJson)) {
        config->taskCycle = CJsonUtil::GetIntValue(jsonRoot_, TASK_EXECUTING_CYCLE);
        HIVIEW_LOGI("task cycle is configured as number for module: %{public}s, value is %{public}" PRId64 "",
            config->moduleName.c_str(), config->taskCycle);
        return true;
    }
    if (!cJSON_IsObject(taskCycleJson)) {
        return false;
    }
    std::string areaTag(Parameter::IsOversea() ? "oversea" : "domestic");
    std::string versionTag(Parameter::IsBetaVersion() ? "beta" : "commercial");
    if (!ParseIntFromCfg(taskCycleJson, areaTag, versionTag, config->taskCycle)) {
        HIVIEW_LOGE("failed to parse task type");
        config->taskCycle = 0;
        return false;
    }
    HIVIEW_LOGI("task cycle is configured as object for module: %{public}s, value is %{public}" PRId64 "",
        config->moduleName.c_str(), config->taskCycle);
    return true;
}
} // HiviewDFX
} // OHOS