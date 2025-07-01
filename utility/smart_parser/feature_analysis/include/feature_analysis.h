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
#ifndef FEATURE_ANALYSIS_H
#define FEATURE_ANALYSIS_H

#include <ctime>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "syntax_rules.h"
namespace OHOS {
namespace HiviewDFX {
enum SeekType {
    FIRST_MATCH = 0,
    LAST_MATCH,
};

class FeatureAnalysis {
    enum ErrorCode {
        DEFAULT = -1,
        SUCCESS = 0,
        BUFFER_ERROR,
        EXTRACT_ERROR,
        COMPOSE_ERROR
    };
public:
    FeatureAnalysis(FeatureSet featureSet, std::map<std::string, std::string> composeRule,
        const std::string& eventType)
        : eventType_(eventType), lineCursor_(0), errorCode_(DEFAULT), line_(""),
          featureSet_(featureSet), composeRule_(composeRule)
    {
        time_t epochTime = time(nullptr);
        if (epochTime > 0) {
            taskId_ = epochTime;
        } else {
            taskId_ = 0;
        }
    };
    ~FeatureAnalysis();
    FeatureAnalysis(const FeatureAnalysis&) = delete;
    FeatureAnalysis& operator=(const FeatureAnalysis&) = delete;

    // interface
    bool AnalysisLog();
    void RawInfoPosition(std::stringstream& buffer);
    bool CheckStartSegment(bool& segmentStart) const;
    int GetErrorCode() const { return errorCode_; };
    std::map<std::string, std::string> GetReasult() const { return eventInfo_; };
    std::vector<std::pair<std::string, LineFeature>> GetParamSeekRecord() {return paramSeekRecord_;};

private:
    void Extract();
    bool IsSourceMatch(const std::string& line, const FeatureRule& rule) const;
    bool ParseElementForParam(const std::string& src, FeatureRule& rule);
    int GetSeekInfo(const std::string& param, std::string& value) const;
    bool CheckVariableParam(FeatureRule& rule) const;
    bool CheckVariable(const FeatureRule& rule, const std::string& leftTag, const std::string& rightTag) const;
    void ReplaceVariable(FeatureRule& rule, const std::string& symbol, const std::string& value) const;
    bool ReplaceVariable(const std::string& src, const std::string& param, const std::string& value,
        std::string& des) const;
    bool CheckDepend(const FeatureRule& rule) const;
    void GetCursorInfo(std::stringstream& buff, const std::string& line);
    LineFeature FormatLineFeature(const std::string& value, const std::string& regex) const;
    void Compose();
    std::string ComposeTrace(const std::string& filePath, const std::string& param,
        const std::vector<std::pair<std::string, LineFeature>>& lineFeatures, const std::string& regex) const;
    std::string ComposeParam(const std::string& param) const;
    std::vector<std::string> SplitParam(const std::string& param) const;
    void ProcessReason(std::map<std::string, std::string>& info);
    bool IsMatchOrExpression(const std::string& line, const std::string& src) const;
    bool IsMatchAndExpression(const std::string& line, const std::string& src) const;
    void SetStackRegex(const std::string& key, const std::string& regex);
    void SetParamRecord(const std::string& key, const LineFeature& value, const int type);

private:
    int taskId_{0};
    std::string eventType_;
    int lineCursor_{0};
    int errorCode_{0};
    const int MAX_SKIP_LINE = 32000; // 32000 max skip line
    static constexpr const char *COMPOSE_PLUS = "+";
    static constexpr const char *COMPOSE_COLON = ":";
    std::string line_;
    std::vector<std::string> deletePath_;
    FeatureSet featureSet_;
    std::map<std::string, std::string> stackRegex_;
    std::vector<std::pair<std::string, LineFeature>> paramSeekRecord_;
    std::map<std::string, std::string> composeRule_;
    std::map<std::string, std::string> eventInfo_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif /* FEATURE_ANALYSIS_H */
