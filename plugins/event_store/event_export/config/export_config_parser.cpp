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

#include "export_config_parser.h"

#include <fstream>

#include "cjson_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventConfigParser");
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
constexpr int32_t INVALID_INT_VAL = -1;
constexpr double INVALID_DOUBLE_VAL = -1.0;
}

ExportConfigParser::ExportConfigParser(const std::string& configFile)
{
    jsonRoot_ = CJsonUtil::ParseJsonRoot(configFile);
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
    auto exportConfig = std::make_shared<ExportConfig>();
    if (jsonRoot_ == nullptr || !cJSON_IsObject(jsonRoot_)) {
        HIVIEW_LOGE("the file format of export config file is not json.");
        return nullptr;
    }
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
    // read task executing cycle
    config->taskCycle = CJsonUtil::GetIntValue(jsonRoot_, TASK_EXECUTING_CYCLE, INVALID_DOUBLE_VAL);
    if (config->taskCycle == INVALID_INT_VAL) {
        HIVIEW_LOGW("taskExecutingCycle configured is invalid.");
        return false;
    }
    // read day count for event file to store
    config->dayCnt = CJsonUtil::GetIntValue(jsonRoot_, FILE_STORED_MAX_DAY_CNT, INVALID_DOUBLE_VAL);
    if (config->dayCnt == INVALID_INT_VAL) {
        HIVIEW_LOGW("fileStoredMaxDayCnt configured is invalid.");
        return false;
    }
    return true;
}
} // HiviewDFX
} // OHOS