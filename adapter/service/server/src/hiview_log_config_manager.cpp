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

#include "cjson_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiviewLogConfigManager");
constexpr char CONFIG_FILE_PATH[] = "/system/etc/hiview/log_type.json";
constexpr char FILE_PATH_KEY[] = "LOGPATH";
constexpr char READ_ONLY_KEY[] = "READONLY";
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
    cJSON* jsonRoot = CJsonUtil::ParseJsonRoot(CONFIG_FILE_PATH);
    if (jsonRoot == nullptr) {
        HIVIEW_LOGW("no valid log config file.");
        return nullptr;
    }
    cJSON* jsonType = CJsonUtil::GetItemMember(jsonRoot, type);
    if (jsonType == nullptr) {
        HIVIEW_LOGW("no such type: %{public}s.", type.c_str());
        cJSON_Delete(jsonRoot);
        return nullptr;
    }
    std::string path(CJsonUtil::GetStringMemberValue(jsonType, FILE_PATH_KEY));
    if (path.empty()) {
        HIVIEW_LOGW("no file path tag or path is empty.");
        cJSON_Delete(jsonRoot);
        return nullptr;
    }
    if (path[path.size() - 1] != '/') {
        path.append("/"); // add slash at end of dir for simple use
    }
    auto configInfoPtr = std::make_shared<ConfigInfo>(path);
    CJsonUtil::GetBoolMemberValue(jsonType, READ_ONLY_KEY, configInfoPtr->isReadOnly);
    configInfos.insert(std::make_pair(type, configInfoPtr));
    cJSON_Delete(jsonRoot);
    return configInfoPtr;
}
} // namespace HiviewDFX
} // namespace OHOS