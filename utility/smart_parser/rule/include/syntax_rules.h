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
#ifndef SYNTAX_RULES_H
#define SYNTAX_RULES_H

#include <list>
#include <map>
#include <string>
#include <vector>
namespace OHOS {
namespace HiviewDFX {
constexpr const char *L1_SEG_STATUS = "segmentStatus";
constexpr const char *L1_RULE = "Rule";
constexpr const char *L2_DIR_OR_FILE = "DirOrFile";
constexpr const char *L2_SKIP = "skip";
constexpr const char *L2_SUBCATELOG = "Subcatalog";
constexpr const char *L2_FEATUREID = "featureId";
constexpr const char *L2_RULES = "rules";
constexpr const char *L2_SEGMENT_RULE = "segmentRule";
constexpr const char *L2_SEGMENT_TYPE = "segmentType";
constexpr const char *L2_SEGMENT_START = "startSeg";
constexpr const char *L2_SEGMENT_STACK = "segStack";

constexpr const char *L3_NAMESPACE = "namespace";
constexpr const char *L3_MATCH_KEY = "matchKey";
constexpr const char *L3_NUM = "num";
constexpr const char *L3_PARAM = "v_";
constexpr const char *L3_DEPEND = "depend";
constexpr const char *L3_DESCRIPTOR_LEFT = "@{";
constexpr const char *L3_DESCRIPTOR_RIGHT = "}@";
constexpr const char *L3_VARIABLE_TRACE_BLOCK = "@T@";
constexpr const char *L3_SEEK_LAST = "@L@";
constexpr const char *L3_OR_DESCRIPTOR = "@|@";
constexpr const char *L3_AND_DESCRIPTOR = "@&@";
constexpr const char *L3_REGULAR_DESCRIPTOR = "@R@";

struct FeatureRule {
    int num {0};
    std::string name;
    std::string source;
    std::string depend;
    std::string cmdType;
    std::map<std::string, std::string> param;
};

struct LineFeature {
    int lineCursor{0};
    std::string value;
};

struct FeatureSet {
    std::string fullPath;
    std::string segmentType;
    int skipStep;
    std::list<FeatureRule> rules;
    std::vector<std::string> startSegVec;
    std::vector<std::string> segStackVec;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif /* SYNTAX_RULES_H */