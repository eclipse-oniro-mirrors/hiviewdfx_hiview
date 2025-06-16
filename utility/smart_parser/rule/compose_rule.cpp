/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "compose_rule.h"

#include <algorithm>
#include <fstream>
#include <memory>

#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("ComposeRule");

void ComposeRule::ParseComposeRule(const string& config, const string& type, vector<string> featureIds)
{
    std::string content;
    if (!FileUtil::LoadStringFromFile(config, content)) {
        HIVIEW_LOGW("Failed to open file, path: %{public}s.", config.c_str());
        return;
    }
    cJSON *root = cJSON_Parse(content.c_str());
    if (!root) {
        HIVIEW_LOGE("cJson parse fail in %{public}s.", config.c_str());
        return;
    }
    cJSON *rootType = cJSON_GetObjectItem(root, type.c_str());
    if (!rootType) {
        HIVIEW_LOGE("cJson parse fail, %{public}s don't exist in %{public}s.", type.c_str(), config.c_str());
        cJSON_Delete(root);
        return;
    }

    ParseRule(rootType, type, featureIds);
    HIVIEW_LOGI("ComposeRule ParseFile end.");
    cJSON_Delete(root);
    return;
}

std::list<std::pair<std::string, std::map<std::string, std::string>>> ComposeRule::GetComposeRule() const
{
    return composeRules_;
}

void ComposeRule::ParseRule(const cJSON *json, const string& type, vector<string>& featureIds)
{
    sort(featureIds.begin(), featureIds.end(), ComparePrio);
    for (const auto& featureId : featureIds) {
        map<string, string> composeParams = GetMapFromJson(json, featureId);
        composeRules_.emplace_back(pair<string, map<string, string>>(featureId, composeParams));
    }
}

bool ComposeRule::ComparePrio(const string& featureIdOne, const string& featureIdTwo)
{
    return StringUtil::GetRightSubstr(featureIdOne, "_") < StringUtil::GetRightSubstr(featureIdTwo, "_");
}

static std::string GetStringValueFromItem(const cJSON *json)
{
    if (!json) {
        return "";
    }
    std::string ret{};
    if (cJSON_IsString(json)) {
        ret = json->valuestring;
    } else if (cJSON_IsNumber(json)) {
        ret = std::to_string(json->valuedouble);
    } else if (cJSON_IsBool(json)) {
        ret = cJSON_IsTrue(json) ? "true" : "false";
    } else {
        ret = "";
    }
    return ret;
}

std::map<std::string, std::string> ComposeRule::GetMapFromJson(const cJSON *json, const string& featureId)
{
    cJSON* jsonFeatureId = cJSON_GetObjectItem(json, featureId.c_str());
    if (!jsonFeatureId) {
        HIVIEW_LOGE("ComposeRule don't have %{public}s featureId.", featureId.c_str());
        return {};
    }
    std::map<std::string, std::string> result;
    cJSON *item;
    cJSON_ArrayForEach(item, jsonFeatureId) {
        std::string key = item->string;
        result.emplace(pair<std::string, std::string>(key, GetStringValueFromItem(item)));
    }
    return result;
}
} // namespace HiviewDFX
} // namespace OHOS
