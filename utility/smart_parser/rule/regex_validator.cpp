/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "syntax_rules.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr size_t ESCAPE_STEP = 2; // 2 : '\' plus the escaped char

constexpr size_t LAZY_QUANTIFIER_STEP = 2; // 2 : quantifier plus the lazy modifier '?'

constexpr int MAX_GROUP_DEPTH = 32; // 32 : max regex group nesting depth

size_t SkipCharClass(const std::string& pattern, size_t start)
{
    size_t next = start + 1;
    while (next < pattern.size() && pattern[next] != ']') {
        next += (pattern[next] == '\\') ? ESCAPE_STEP : 1;
    }
    return next + 1;
}

size_t SkipEscapeSequence(const std::string& pattern, size_t start)
{
    constexpr size_t UNICODE_HEX_LENGTH = 4; // \uHHHH
    constexpr size_t HEX_LENGTH = 2;         // \xHH
    size_t next = start + ESCAPE_STEP;       // skip '\' and the escaped char
    if (next > pattern.size()) {
        return pattern.size();
    }
    const char escaped = pattern[start + 1];
    if (escaped != 'u' && escaped != 'x') {
        return next;
    }
    if (next < pattern.size() && pattern[next] == '{') {
        size_t end = pattern.find('}', next);
        return (end == std::string::npos) ? pattern.size() : (end + 1);
    }
    size_t hexLength = (escaped == 'u') ? UNICODE_HEX_LENGTH : HEX_LENGTH;
    return (next + hexLength < pattern.size()) ? (next + hexLength) : pattern.size();
}

bool HasUnboundedQuantifier(const std::string& pattern, size_t pos, size_t& nextPos)
{
    nextPos = pos;
    if (pos >= pattern.size()) {
        return false;
    }
    const char cur = pattern[pos];
    if (cur == '*' || cur == '+') {
        nextPos += (pos + 1 < pattern.size() && pattern[pos + 1] == '?') ? LAZY_QUANTIFIER_STEP : 1;
        return true;
    }
    if (cur == '{') {
        size_t end = pattern.find('}', pos);
        if (end != std::string::npos) {
            nextPos = end + 1;
            return pattern[end - 1] == ',';
        }
    }
    return false;
}

void MarkQuantifierLevel(size_t* contentHeight, int depth, size_t& maxHeight)
{
    contentHeight[depth] = (contentHeight[depth] > 1) ? contentHeight[depth] : 1;
    maxHeight = (maxHeight > 1) ? maxHeight : 1;
}

size_t HandleGroupClose(const std::string& pattern, size_t index, int& depth,
    size_t* contentHeight, size_t& maxHeight)
{
    size_t nextPos = 0;
    bool unboundedQuant = HasUnboundedQuantifier(pattern, index + 1, nextPos);
    size_t quantHeight = unboundedQuant ? (contentHeight[depth] + 1) : contentHeight[depth];
    contentHeight[depth - 1] = (contentHeight[depth - 1] > quantHeight) ?
        contentHeight[depth - 1] : quantHeight;
    maxHeight = (maxHeight > quantHeight) ? maxHeight : quantHeight;
    depth--;
    return (nextPos > index) ? nextPos : (index + 1);
}
}

size_t GetRegexQuantifierDepth(const std::string& pattern)
{
    size_t contentHeight[MAX_GROUP_DEPTH + 1] = {0};
    int depth = 0;
    size_t maxHeight = 0;
    size_t index = 0;
    while (index < pattern.size()) {
        const char cur = pattern[index];
        if (cur == '\\') {
            index = SkipEscapeSequence(pattern, index);
        } else if (cur == '[') {
            index = SkipCharClass(pattern, index);
        } else if (cur == '(') {
            depth++;
            if (depth > MAX_GROUP_DEPTH) {
                return MAX_GROUP_DEPTH;
            }
            contentHeight[depth] = 0;
            index++;
        } else if (cur == ')') {
            if (depth == 0) {
                return INVALID_REGEX_DEPTH;
            }
            index = HandleGroupClose(pattern, index, depth, contentHeight, maxHeight);
        } else if (cur == '*' || cur == '+') {
            MarkQuantifierLevel(contentHeight, depth, maxHeight);
            index += (index + 1 < pattern.size() && pattern[index + 1] == '?') ?
                LAZY_QUANTIFIER_STEP : 1;
        } else if (cur == '{') {
            size_t nextPos = 0;
            if (HasUnboundedQuantifier(pattern, index, nextPos)) {
                MarkQuantifierLevel(contentHeight, depth, maxHeight);
            }
            index = (nextPos > index) ? nextPos : (index + 1);
        } else {
            index++;
        }
    }
    return (depth != 0) ? INVALID_REGEX_DEPTH : maxHeight;
}
}
}
