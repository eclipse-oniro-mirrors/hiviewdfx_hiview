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
#include "log_parse.h"

#include "string_util.h"
#include "tbox.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {

LogParse::LogParse(const std::string& eventName)
{
    // some stack function is invalid, so it should be ignored
    std::map<std::string, std::set<std::string>> ignoreList = {
        {"Level1", {
            "libc.so",
            "libc++.so",
            "libc++_shared.so",
            "ld-musl-aarch64.so",
            "libc_fdleak_debug.so",
            "unknown",
            "watchdog",
            "kthread",
            "rdr_system_error",
            "[Unknown]",
            "[shmm]",
            "Not mapped"}
        },
        {"Level2", {
            "libart.so",
            "__switch_to",
            "dump_backtrace",
            "show_stack",
            "dump_stack"}
        },
        {"Level3", {
            "panic"}
        }
    };

    ignoreList_ = std::move(ignoreList);
    if (eventName == "APP_FREEZE" || eventName == "SYS_FREEZE") {
        ignoreList_["Level1"].emplace("libeventhandler.z.so");
        ignoreList_["Level1"].emplace("libipc_common.z.so");
        ignoreList_["Level1"].emplace("libipc_core.z.so");
        ignoreList_["Level1"].emplace("libipc_single.z.so");
        ignoreList_["Level1"].emplace("stub.an");
    }
}

bool LogParse::IsIgnoreLibrary(const string& val) const
{
    for (const auto &list : ignoreList_) {
        for (const auto &str : list.second) {
            if (val.find(str, 0) != string::npos) {
                return true;
            }
        }
    }
    return false;
}

stack<string> LogParse::GetStackTop(const vector<string>& validStack, const size_t num) const
{
    size_t len = validStack.size();
    stack<string> stackTop;
    for (size_t i = 0; i < len; i++) {
        if (i == 0 || len - i < num) {
            stackTop.push(validStack.at(i));
        }
    }
    return stackTop;
}

vector<string> LogParse::StackToPart(stack<string>& inStack, size_t num) const
{
    stack<string> partStack;
    while (!inStack.empty()) {
        string topStr = inStack.top();
        StringUtil::EraseString(topStr, "\t");
        partStack.push(topStr);
        inStack.pop();
    }
    vector<string> validPart;
    if (!partStack.empty()) {
        validPart = GetValidStack(num, partStack);
    }
    return validPart;
}

string LogParse::GetValidBlock(stack<string> inStack, vector<string>& lastPart) const
{
    vector<string> validStack;

    lastPart = StackToPart(inStack, 3); // 3 : first/second/last frame
    if (lastPart.empty()) {
        return "";
    } else if (lastPart.size() > STACK_LEN_MAX) {
        // keep the begin 28 lines and the end 2 lines
        lastPart.erase(lastPart.begin() + (STACK_LEN_MAX - 2), lastPart.end() - 2); // 2 : end 2 lines
    }

    reverse(lastPart.begin(), lastPart.end());

    for (auto& it : lastPart) {
        it = Tbox::GetStackName(it);
    }
    return string{Tbox::ARRAY_STR} + StringUtil::VectorToString(lastPart, false);
}

vector<string> LogParse::GetValidStack(size_t num, stack<string>& inStack) const
{
    stack<string> src = inStack;
    vector<string> validStack;
    stack<string> outStatck;
    size_t len = src.size();
    for (size_t i = 0; i < len; i++) {
        auto stackFrame = src.top();
        if (!IsIgnoreLibrary(stackFrame)) {
            validStack.push_back(stackFrame);
        }
        src.pop();
    }
    if (validStack.empty()) {
        MatchIgnoreLibrary(inStack, outStatck, num);
        len = outStatck.size();
        for (size_t i = 0; i < len; i++) {
            validStack.push_back(outStatck.top());
            outStatck.pop();
        }
    }
    return validStack;
}

void LogParse::MatchIgnoreLibrary(stack<string> inStack, stack<string>& outStack, size_t num) const
{
    if (inStack.size() <= num) {
        outStack = inStack;
        return;
    }
    size_t count = 0;
    for (auto it = ignoreList_.rbegin(); it != ignoreList_.rend(); ++it) {
        if (count == ignoreList_.size() - 1) {
            outStack = inStack;
            return;
        }

        stack<string> src = inStack;
        while (src.size() > num) {
            string name = src.top();
            for (auto str : it->second) {
                if (name.find(str, 0) != string::npos) {
                    outStack = src;
                    return;
                }
            }
            src.pop();
        }
        count++;
    }
}

/*
 * INPUT :
 *  info : trace spliting by "\n"
 * OUTPUT :
 *  trace : last part trace to get Frame
 *  return string : valid trace spliting by "\n"
 */
std::string LogParse::GetFilterTrace(const std::string& info, std::vector<std::string>& trace,
    std::string eventType) const
{
    std::string newInfo = info;
    if (eventType == "JS_ERROR" && newInfo.find("libace_napi.z.so") != std::string::npos) {
        newInfo = StringUtil::GetRightSubstr(info, "libace_napi.z.so");
    }
    StringUtil::SplitStr(newInfo, "\n", trace, false, false);
    std::stack<std::string> traceStack;
    for (const auto& str : trace) {
        traceStack.push(str);
    }
    trace.clear();
    return GetValidBlock(traceStack, trace);
}

void LogParse::SetFrame(std::stack<std::string>& stack, std::map<std::string, std::string>& eventInfo) const
{
    std::vector<std::string> name = {"FIRST_FRAME", "SECOND_FRAME", "LAST_FRAME"};
    size_t len = stack.size();
    for (size_t i = 0; i < len; i++) {
        if (eventInfo.find(name[i]) == eventInfo.end()) {
            eventInfo[name[i]] = stack.top();
        }
        stack.pop();
    }
}
}
}
