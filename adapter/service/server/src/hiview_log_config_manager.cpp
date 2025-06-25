/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "hiview_log_config_manager.h"

#include <fstream>

#include "json/json.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiviewLogConfigManager");
constexpr char CONFIG_FILE_PATH[] = "/system/etc/hiview/log_type.json";
constexpr char FILE_PATH_KEY[] = "LOGPATH";
constexpr char READ_ONLY_KEY[] = "READONLY";

Json::Value ParseJsonFile(const std::string& path)
{
    Json::Value jsonRoot;
    std::ifstream fin(path, std::ifstream::binary);
    if (!fin.is_open()) {
        HIVIEW_LOGW("failed to open file, path: %{public}s.", path.c_str());
        return Json::Value();
    }
    Json::CharReaderBuilder builder;
    Json::CharReaderBuilder::strictMode(&builder.settings_);
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, fin, &jsonRoot, &errs)) {
        HIVIEW_LOGE("failed to parse file, path: %{public}s.", path.c_str());
        return Json::Value();
    }
    return jsonRoot;
}

inline bool IsStringMember(const Json::Value& jsonRoot, const std::string& key)
{
    return jsonRoot.isObject() && jsonRoot.isMember(key) && jsonRoot[key].isString();
}

inline bool IsBoolMember(const Json::Value& jsonRoot, const std::string& key)
{
    return jsonRoot.isObject() && jsonRoot.isMember(key) && jsonRoot[key].isBool();
}
}

std::shared_ptr<ConfigInfo> HiviewLogConfigManager::GetConfigInfoByType(const std::string& type)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (configInfos.find(type) != configInfos.end()) {
        return configInfos[type];
    }
    return GetLogConfigFromFile(type);
}

std::shared_ptr<ConfigInfo> HiviewLogConfigManager::GetLogConfigFromFile(const std::string& type)
{
    HIVIEW_LOGI("read log config from file, type: %{public}s", type.c_str());
    Json::Value jsonRoot = ParseJsonFile(CONFIG_FILE_PATH);
    if (jsonRoot.empty() || !jsonRoot.isObject()) {
        HIVIEW_LOGW("no valid log config file.");
        return nullptr;
    }
    if (!jsonRoot.isMember(type)) {
        HIVIEW_LOGW("no such type: %{public}s.", type.c_str());
        return nullptr;
    }
    if (!IsStringMember(jsonRoot[type], FILE_PATH_KEY)) {
        HIVIEW_LOGW("no file path tag.");
        return nullptr;
    }
    std::string path(jsonRoot[type][FILE_PATH_KEY].asString());
    if (path.empty()) {
        HIVIEW_LOGW("path is empty.");
        return nullptr;
    }
    if (path[path.size() - 1] != '/') {
        path.append("/"); // add slash at end of dir for simple use
    }
    auto configInfoPtr = std::make_shared<ConfigInfo>(path);
    if (IsBoolMember(jsonRoot[type], READ_ONLY_KEY)) {
        configInfoPtr->isReadOnly = jsonRoot[type][READ_ONLY_KEY].asBool();
    }
    configInfos.insert(std::make_pair(type, configInfoPtr));
    return configInfoPtr;
}
} // namespace HiviewDFX
} // namespace OHOS