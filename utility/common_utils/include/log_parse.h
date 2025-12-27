/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef LOG_PARSE_H
#define LOG_PARSE_H

#include <map>
#include <set>
#include <stack>
#include <string>

#include "common_defines.h"
namespace OHOS {
namespace HiviewDFX {
class LogParse {
public:
    explicit LogParse(const std::string& eventName);
    ~LogParse() = default;
    LogParse(const LogParse&) = delete;
    LogParse& operator=(const LogParse&) = delete;
    bool IsIgnoreLibrary(const std::string& val) const;
    void SetFrame(std::stack<std::string>& stack, std::map<std::string, std::string>& eventInfo) const;
    std::string GetFilterTrace(const std::string& info, std::vector<std::string>& trace,
        std::string eventType) const;
    std::stack<std::string> GetStackTop(const std::vector<std::string>& validStack, const size_t num) const;

private:
    std::vector<std::string> GetValidStack(size_t num, std::stack<std::string>& inStack) const;
    std::vector<std::string> StackToPart(std::stack<std::string>& inStack, size_t num) const;
    void MatchIgnoreLibrary(std::stack<std::string> inStack, std::stack<std::string>& outStack, size_t num) const;
    std::string GetValidBlock(std::stack<std::string> inStack, std::vector<std::string>& lastPart) const;

private:
    static const int STACK_LEN_MAX = 30;
    std::map<std::string, std::set<std::string>> ignoreList_;
};
}
}
#endif
