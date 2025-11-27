/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "string_util.h"

#include <climits>
#include <iomanip>
#include <iostream>
#include <regex.h>
#include <sstream>
#include <string>
#include <vector>

namespace OHOS {
namespace HiviewDFX {
namespace StringUtil {
namespace {
std::string ReplaceMatchedStrWithAsterisk(const std::string& source, const std::string& patternStr)
{
    if (source.empty()) {
        return "";
    }
    std::regex pattern(patternStr);
    std::smatch result;
    if (regex_search(source, result, pattern)) {
        const size_t regexSize = 2;
        if (result.size() == regexSize) {
            return StringUtil::ReplaceStr(source, result[1].str(), "******");
        }
    }
    return source;
}
}
using namespace std;
const char INDICATE_VALUE_CHAR = ':';
const char KEY_VALUE_END_CHAR = ';';
constexpr int SKIP_NEXT_INDEX_LENGTH = 2;
std::string ConvertVectorToStr(const std::vector<std::string> &listStr, const std::string &split)
{
    std::string str("");
    for (auto &item : listStr) {
        if (str == "") {
            str = item;
        } else {
            str += split + item;
        }
    }
    return str;
}

string ReplaceStr(const string& str, const string& src, const string& dst)
{
    if (src.empty()) {
        return str;
    }

    string::size_type pos = 0;
    string strTmp = str;
    while ((pos = strTmp.find(src, pos)) != string::npos) {
        strTmp.replace(pos, src.length(), dst);
        pos += dst.length();
    }

    return strTmp;
}

string TrimStr(const string& str, const char cTrim)
{
    string strTmp = str;
    strTmp.erase(0, strTmp.find_first_not_of(cTrim));
    strTmp.erase(strTmp.find_last_not_of(cTrim) + sizeof(char));
    return strTmp;
}

void SplitStr(const string& str, const string& sep, vector<string>& strs,
              bool canEmpty, bool needTrim)
{
    strs.clear();
    string strTmp = needTrim ? TrimStr(str) : str;
    string strPart;
    while (true) {
        string::size_type pos = strTmp.find(sep);
        if (string::npos == pos || sep.empty()) {
            strPart = needTrim ? TrimStr(strTmp) : strTmp;
            if (!strPart.empty() || canEmpty) {
                strs.push_back(strPart);
            }
            break;
        } else {
            strPart = needTrim ? TrimStr(strTmp.substr(0, pos)) : strTmp.substr(0, pos);
            if (!strPart.empty() || canEmpty) {
                strs.push_back(strPart);
            }
            strTmp = strTmp.substr(sep.size() + pos, strTmp.size() - sep.size() - pos);
        }
    }
}

bool StrToInt(const string& str, int& value)
{
    if (str.empty() || (!isdigit(str.front()) && (str.front() != '-'))) {
        return false;
    }

    char* end = nullptr;
    const int base = 10;
    errno = 0;
    auto addr = str.c_str();
    auto result = strtol(addr, &end, base);
    if (end == addr || end[0] != '\0' || errno == ERANGE) {
        return false;
    }

    value = static_cast<int>(result);
    return true;
}

int StrToInt(const string& str)
{
    int id = -1;
    StrToInt(str, id);
    return id;
}

std::list<std::string> SplitStr(const std::string& str, char delimiter)
{
    std::list<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

string GetLeftSubstr(const string& input, const string& split)
{
    size_t pos = input.find(split, 0);
    if (pos == string::npos) {
        return input;
    }
    return input.substr(0, pos);
}

string GetRightSubstr(const string& input, const string& split)
{
    size_t pos = input.find(split, 0);
    if (pos == string::npos) {
        return "none";
    }
    return input.substr(pos + split.size(), input.size() - pos);
}

string GetRleftSubstr(const string& input, const string& split)
{
    size_t pos = input.rfind(split, string::npos);
    if (pos == string::npos) {
        return input;
    }
    return input.substr(0, pos);
}

string GetRrightSubstr(const string& input, const string& split)
{
    size_t pos = input.rfind(split, string::npos);
    if (pos == string::npos) {
        return "none";
    }
    return input.substr(pos + 1, input.size() - pos);
}

string EraseString(const string& input, const string& toErase)
{
    size_t pos = 0;
    string out = input;
    while ((pos = out.find(toErase, pos)) != string::npos) {
        out.erase(pos, toErase.size());
        pos = (pos == 0) ? (0) : (pos - 1);
    }
    return out;
}

string GetMidSubstr(const string& input, const string& begin, const string& end)
{
    string midstr = "";
    size_t beginPos = input.find(begin, 0);
    if (beginPos == string::npos) {
        return midstr;
    }
    beginPos = beginPos + begin.size();
    size_t endPos = input.find(end, beginPos);
    if (endPos == string::npos) {
        return midstr;
    }
    return input.substr(beginPos, endPos - beginPos);
}

string VectorToString(const vector<string>& src, bool reverse, const string& tag)
{
    string str;
    for (auto element : src) {
        if (element.empty()) {
            continue;
        }
        if (reverse) {
            str = element + tag + str;
        } else {
            str = str + element + tag;
        }
    }
    return str;
}

uint64_t StringToUl(const string& flag, int base)
{
    uint64_t ret = strtoul(flag.c_str(), NULL, base);
    if (ret == ULONG_MAX) {
        return 0;
    }
    return ret;
}

double StringToDouble(const string& input)
{
    if (input.empty()) {
        return 0;
    }
    char *e = nullptr;
    errno = 0;
    double temp = std::strtod(input.c_str(), &e);

    if (errno != 0) { // error, overflow or underflow
        return 0;
    }
    return temp;
}

std::string EscapeJsonStringValue(const std::string &value)
{
    std::string escapeValue;
    for (auto it = value.begin(); it != value.end(); it++) {
        switch (*it) {
            case '\\':
                escapeValue.push_back('\\');
                escapeValue.push_back('\\');
                break;
            case '\"':
                escapeValue.push_back('\\');
                escapeValue.push_back('"');
                break;
            case '\b':
                escapeValue.push_back('\\');
                escapeValue.push_back('b');
                break;
            case '\f':
                escapeValue.push_back('\\');
                escapeValue.push_back('f');
                break;
            case '\n':
                escapeValue.push_back('\\');
                escapeValue.push_back('n');
                break;
            case '\r':
                escapeValue.push_back('\\');
                escapeValue.push_back('r');
                break;
            case '\t':
                escapeValue.push_back('\\');
                escapeValue.push_back('t');
                break;
            default:
                escapeValue.push_back(*it);
                break;
        }
    }
    return escapeValue;
}

std::string UnescapeJsonStringValue(const std::string &value)
{
    bool isEscaped = true;
    std::string unescapeValue;
    for (auto it = value.begin(); it != value.end(); it++) {
        if (isEscaped) {
            switch (*it) {
                case '"':
                    unescapeValue.push_back('\"');
                    break;
                case '/':
                    unescapeValue.push_back('/');
                    break;
                case 'b':
                    unescapeValue.push_back('\b');
                    break;
                case 'f':
                    unescapeValue.push_back('\f');
                    break;
                case 'n':
                    unescapeValue.push_back('\n');
                    break;
                case 'r':
                    unescapeValue.push_back('\r');
                    break;
                case 't':
                    unescapeValue.push_back('\t');
                    break;
                case '\\':
                    unescapeValue.push_back('\\');
                    break;
                default:
                    unescapeValue.push_back(*it);
                    break;
            }
            isEscaped = false;
        } else {
            switch (*it) {
                case '\\':
                    isEscaped = true;
                    break;
                default:
                    unescapeValue.push_back(*it);
                    break;
            }
        }
    }
    return unescapeValue;
}

std::string FormatCmdLine(const std::string& cmdLine)
{
    size_t startPos = 0;
    size_t endPos = cmdLine.size();
    for (size_t i = 0; i < cmdLine.size(); i++) {
        if (cmdLine[i] == '/') {
            startPos = i + 1;
        } else if (cmdLine[i] == '\0') {
            endPos = i;
            break;
        }
    }
    return cmdLine.substr(startPos, endPos - startPos);
}

void FormatProcessName(std::string& processName)
{
    processName = FormatCmdLine(processName);
    std::for_each(processName.begin(), processName.end(), [] (char &c) {
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '.') || (c == '-') ||
        (c == '_')) {
            return;
        };
        c = '_';
    });
}

std::string HideSnInfo(const std::string& str)
{
    return ReplaceMatchedStrWithAsterisk(str, R"(_([-=+$a-zA-Z0-9\[\)\>]{12,50})_)");
}

std::string HideDeviceIdInfo(const std::string& str)
{
    return ReplaceMatchedStrWithAsterisk(str, R"(_([A-Za-z0-9]{60,65})_)");
}

bool StartWith(const std::string& str, const std::string& sub)
{
    return str.find(sub) == 0;
}

bool EndWith(const std::string& str, const std::string& sub)
{
    size_t index = str.rfind(sub);
    if (index == std::string::npos) {
        return false;
    }
    return index + sub.size() == str.size();
}

bool IsValidRegex(const std::string& regStr)
{
    // ignore empty regx
    if (regStr.empty()) {
        return true;
    }

    int flags = REG_EXTENDED;
    regex_t reg;
    int status = regcomp(&reg, regStr.c_str(), flags);
    // free regex
    regfree(&reg);
    return (status == REG_OK);
}
} // namespace StringUtil
} // namespace HiviewDFX
} // namespace OHOS
