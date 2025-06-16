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
#include "extract_rule.h"

#include <fstream>
#include <regex>

#include "file_util.h"
#include "log_util.h"
#include "hiview_logger.h"
#include "string_util.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("ExtractRule");
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

std::string GetStringValue(const cJSON *json, const std::string& key)
{
    if (!cJSON_IsObject(json)) {
        return "";
    }
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key.c_str());
    return GetStringValueFromItem(item);
}

void ExtractRule::ParseExtractRule(const string& eventType, const string& config, const string& path)
{
    std::string content;
    if (!FileUtil::LoadStringFromFile(config, content)) {
        HIVIEW_LOGW("Failed to open file, path: %{public}s.", config.c_str());
        return;
    }
    cJSON *root = cJSON_Parse(content.c_str());
    if (!root) {
        HIVIEW_LOGE("Json parse fail in %{public}s.", config.c_str());
        return;
    }

    ParseSegStatusCfg(root);
    ParseRule(eventType, root, path);
    cJSON_Delete(root);
    return;
}

void ExtractRule::ParseSegStatusCfg(const cJSON *json)
{
    cJSON *arrayObj = cJSON_GetObjectItem(json, L1_SEG_STATUS.c_str());
    if (!arrayObj) {
        HIVIEW_LOGE("failed to get json number %{public}s.", L1_SEG_STATUS.c_str());
        return;
    }

    int arrayObjSize = cJSON_GetArraySize(arrayObj);
    if (arrayObjSize > JSON_ARRAY_THRESHOLD) {
        arrayObjSize = JSON_ARRAY_THRESHOLD;
        HIVIEW_LOGI("json array has been resized to threshold value.");
    }
    for (int i = 0; i < arrayObjSize; i++) {
        cJSON *obj = cJSON_GetArrayItem(arrayObj, i);
        if (!obj) {
            HIVIEW_LOGE("get arrayItem from json fail.");
            continue;
        }

        string name = GetStringValue(obj, "namespace");
        vector<string> cfg;
        cfg.emplace_back(GetStringValue(obj, "matchKey"));
        cfg.emplace_back(GetStringValue(obj, "desc"));
        if (!name.empty() && !cfg[0].empty()) {
            segStatusCfgMap_.emplace(make_pair(name, cfg));
        }
    }
}

/*
 * parse and store into feature set
 */
void ExtractRule::ParseRule(const string& eventType, const cJSON *json, const string& path)
{
    cJSON *item = nullptr;
    cJSON_ArrayForEach(item, json) {
        std::string key = item->string;
        if (key.find("Rule") == std::string::npos) {
            continue;
        }
        string dirOrFile = GetStringValue(item, L2_DIR_OR_FILE);
        if (dirOrFile.empty()) {
            continue;
        }
        string subcatalog = GetStringValue(item, L2_SUBCATELOG);
        vector<string> featureIds = SplitFeatureId(item);
        FeatureSet fsets{};
        for (const auto& featureId : featureIds) {
            if (!IsMatchId(eventType, featureId) || !IsMatchPath(path, dirOrFile, subcatalog, fsets.fullPath)) {
                continue;
            }
            cJSON *skipItem = cJSON_GetObjectItemCaseSensitive(item, L2_SKIP.c_str());
            fsets.skipStep = cJSON_IsNumber(skipItem) ? static_cast<int>(cJSON_GetNumberValue(skipItem)) : 0;
            fsets.segmentType = GetStringValue(item, L2_SEGMENT_TYPE);
            fsets.startSegVec = GetJsonArray(item, L2_SEGMENT_START);
            fsets.segStackVec = GetJsonArray(item, L2_SEGMENT_STACK);
            // 1st: parse feature
            ParseRule(item, fsets.rules);
            featureSets_.emplace(pair<string, FeatureSet>(featureId, fsets));
            featureIds_.emplace_back(featureId);
            HIVIEW_LOGI("ParseFile eventId %{public}s, FeatureId %{public}s.", eventType.c_str(), featureId.c_str());
        }
    }
    HIVIEW_LOGD("ParseFile end.");
    return;
}

vector<string> ExtractRule::GetFeatureId()
{
    return featureIds_;
}

bool ExtractRule::IsMatchId(const string& eventType, const string& featureId) const
{
    string firstMatch = StringUtil::GetRightSubstr(featureId, "_");
    if (StringUtil::GetRleftSubstr(firstMatch, "_") == eventType) {
        return true;
    }
    return false;
}

std::vector<std::string> ExtractRule::GetJsonArray(const cJSON *json, const string& param)
{
    cJSON* paramItem = cJSON_GetObjectItem(json, param.c_str());
    if (!json || !paramItem || paramItem->type != cJSON_Array) {
        HIVIEW_LOGE("failed to get json array number %{public}s.\n", param.c_str());
        return {};
    }

    int jsonSize = cJSON_GetArraySize(paramItem);
    if (jsonSize > JSON_ARRAY_THRESHOLD) {
        jsonSize = JSON_ARRAY_THRESHOLD;
        HIVIEW_LOGI("json array has been resized to threshold value.");
    }
    std::vector<std::string> result;
    for (int i = 0; i < jsonSize; i++) {
        cJSON *item = cJSON_GetArrayItem(paramItem, i);
        result.push_back(GetStringValueFromItem(item));
    }
    return result;
}

/**
 * sourcefile: the full path
 * name: static path
 * pattern: dynamic path
 */
bool ExtractRule::IsMatchPath(const string& sourceFile, const string& name, const string& pattern,
    string& desPath) const
{
    HIVIEW_LOGI("sourceFile is %{public}s, name is %{public}s, pattern is %{public}s.\n",
        sourceFile.c_str(), name.c_str(), pattern.c_str());
    desPath = sourceFile;

    if (LogUtil::IsTestModel(sourceFile, name, pattern, desPath)) {
        HIVIEW_LOGI("this is test model, desPath is %{public}s.\n", desPath.c_str());
        return true;
    }

    if (pattern.empty()) {
        desPath = name;
        return LogUtil::FileExist(desPath);
    }

    std::vector<std::string> paths;
    StringUtil::SplitStr(pattern, "@|@", paths, false, false);

    for (auto path : paths) {
        std::vector<std::string> parts;
        StringUtil::SplitStr(path, "/", parts, false, false);
        std::string out = (name.back() == '/') ? name : (name + "/");
        for (auto& part : parts) {
            if (regex_match(sourceFile, regex(out + part))) {
                out = ((*(sourceFile.rbegin())) == '/') ? sourceFile : (sourceFile + "/");
            } else {
                out += part + "/";
            }
        }
        desPath = out.substr(0, out.size() - 1);
        if (LogUtil::FileExist(desPath)) {
            HIVIEW_LOGI("desPath is %{public}s.\n", desPath.c_str());
            return true;
        }
    }
    return false;
}

vector<string> ExtractRule::SplitFeatureId(const cJSON *object) const
{
    cJSON *item = cJSON_GetObjectItem(object, L2_FEATUREID.c_str());
    if (!item) {
        HIVIEW_LOGE("failed to get json number %{public}s.", L1_SEG_STATUS.c_str());
        return {};
    }

    vector<string> result;
    StringUtil::SplitStr(GetStringValueFromItem(item), ",", result, false, false);
    return result;
}

void ExtractRule::ParseRule(const cJSON *object, list<FeatureRule>& features) const
{
    cJSON *l2Rule = cJSON_GetObjectItem(object, L2_RULES.c_str());
    if (!l2Rule) {
        return;
    }
    ParseRuleParam(l2Rule, features, L2_RULES);

    cJSON *l2SegRule = cJSON_GetObjectItem(object, L2_SEGMENT_RULE.c_str());
    if (!l2SegRule) {
        return;
    }
    ParseRuleParam(l2SegRule, features, L2_SEGMENT_RULE);
}

void ExtractRule::ParseRuleParam(const cJSON *object, list<FeatureRule>& features, const string& type) const
{
    int objectSize = cJSON_GetArraySize(object);
    if (objectSize > JSON_ARRAY_THRESHOLD) {
        objectSize = JSON_ARRAY_THRESHOLD;
        HIVIEW_LOGI("json array has been resized to threshold value.");
    }
    for (int i = 0; i < objectSize; i++) {
        cJSON *item = cJSON_GetArrayItem(object, i);
        FeatureRule command{};
        command.cmdType = type;
        command.name = GetStringValue(item, L3_NAMESPACE);
        command.source = GetStringValue(item, L3_MATCH_KEY);
        command.depend = GetStringValue(item, L3_DEPEND);
        GetExtractParam(item, command.param, L3_PARAM);
        cJSON *num = cJSON_GetObjectItem(item, L3_NUM.c_str());
        command.num = cJSON_IsNumber(num) ? static_cast<int>(cJSON_GetNumberValue(num)) : 0;
        if (command.num > 0 && type == L2_RULES) {
            HIVIEW_LOGE("rule command can't have num.\n");
            continue;
        }
        features.emplace_back(command);
    }
}

void ExtractRule::GetExtractParam(const cJSON *rules,
    std::map<std::string, std::string>& param, const std::string& preKey) const
{
    cJSON *item = nullptr;
    cJSON_ArrayForEach(item, rules) {
        std::string key = item->string;
        auto pos = key.find(preKey);
        if (pos == 0) {
            param.emplace(key, GetStringValueFromItem(item));
        }
    }
}
} // namespace HiviewDFX
} // namespace OHOS
