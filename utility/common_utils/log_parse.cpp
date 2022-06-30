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
#include "log_parse.h"

#include "string_util.h"
#include "tbox.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
const std::string LogParse::UNMATCHED_EXCEPTION = "UnMatchedException";

// some stack function is invalid, so it should be ignored
const std::map<std::string, std::set<std::string>> LogParse::ignoreList_ = {
    {"Level1", {
        "libc.so",
        "libclang_rt.asan-arm-android.so",
        "libclang_rt.asan-aarch64-android.so",
        "libc_fdleak_debug.so",
        "unknown",
        "watchdog",
        "kthread",
        "rdr_system_error"}
    },
    {"Level2", {
        "libart.so",
        "__switch_to",
        "dump_backtrace",
        "show_stack",
        "dump_stack",
        "java.lang.Object",
        "java.lang.Thread",
        "java.lang.reflect.Method.invoke",
        "com.android.internal.os.ZygoteInit.main",
        "com.android.internal.os.RuntimeInit"}
    },
    {"Level3", {
        "android.os.BinderProxy",
        "panic",
        "android.os.SystemClock.sleep"}
    }
};

// it is key word in app crash log, it can replace complexed info which may have safe and privacy info
const std::set<std::string> LogParse::exceptionList_ = {
    "ArithmeticException",
    "ArrayIndexOutOfBoundsException",
    "ArrayStoreException",
    "ClassCastException",
    "ClassNotFoundException",
    "CloneNotSupportedException",
    "EnumConstantNotPresentException",
    "IllegalAccessException",
    "IllegalArgumentException",
    "IllegalMonitorStateException",
    "IllegalStateException",
    "IllegalThreadStateException",
    "IndexOutOfBoundsException",
    "InstantiationException",
    "InterruptedException",
    "NegativeArraySizeException",
    "NoSuchFieldException",
    "NoSuchMethodException",
    "NullPointerException",
    "NumberFormatException",
    "ReflectiveOperationException",
    "RuntimeException",
    "SecurityException",
    "StringIndexOutOfBoundsException"
};

bool LogParse::IsIgnoreLibrary(const string& val) const
{
    for (auto list : ignoreList_) {
        for (auto str : list.second) {
            if (val.find(str, 0) != string::npos) {
                return true;
            }
        }
    }
    return false;
}

/*
 * Remove ignored backtrace
 * inStack : inverted sequence with fault log
 * outStack : filter stack
 */
bool LogParse::GetValidStack(int num, stack<string>& inStack, stack<string>& outStack) const
{
    vector<string> validStack;
    size_t count = static_cast<size_t>(num);
    // count < 1: indicate stack is empty
    if (count < 1 || inStack.empty()) {
        return false;
    }

    // Automatically checks if it is a stack
    bool iStack = TBox::IsCallStack(inStack.top());
    if (iStack) {
        validStack = GetValidStack(count, inStack);
    }
    outStack = GetStackTop(validStack, count);
    return true;
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

list<vector<string>> LogParse::StackToMultipart(stack<string>& inStack, size_t num) const
{
    stack<string> partStack;
    vector<string> validPart;
    list<vector<string>> multiPart;
    int len = inStack.size();
    for (int i = 0; i < len; i++) {
        string topStr = inStack.top();
        StringUtil::EraseString(topStr, "\t");
        if (TBox::HasCausedBy(topStr)) {
            topStr = MatchExceptionLibrary(topStr);
            if (!partStack.empty()) {
                validPart = GetValidStack(num, partStack);
            }
            validPart.insert(validPart.begin(), topStr);
            multiPart.push_back(validPart);
            partStack = stack<string>();
            validPart.clear();
            inStack.pop();
            continue;
        }
        partStack.push(topStr);
        inStack.pop();
    }
    if (!partStack.empty()) {
        validPart = GetValidStack(num, partStack);
        multiPart.push_back(validPart);
    }
    return multiPart;
}

string LogParse::GetValidBlock(stack<string> inStack, vector<string>& lastPart) const
{
    vector<string> validStack;

    list<vector<string>> multiPart = StackToMultipart(inStack, 3); // 3 : F1-F3NAME
    size_t size = multiPart.size();
    if (size == 0) {
        return "";
    }
    if (size == 1) {
        // only one part
        validStack = multiPart.front();
        if (validStack.size() > TRUST_LEN_MAX) {
            // keep the begin 28 lines and the end 2 lines
            validStack.erase(validStack.begin() + (TRUST_LEN_MAX - 2), validStack.end() - 2); // 2 : end 2 lines
        }
    } else if (size >= 2) { // at least 2 parts
        for (auto part : multiPart) {
            if (validStack.size() >= TRUST_LEN_MAX) {
                break;
            }
            validStack.insert(validStack.begin(), part.begin(), part.end());
        }
        if (multiPart.front().size() > TRUST_LEN_MAX) {
            // keep the begin 28 lines and the end 2 lines
            validStack.erase(validStack.begin() + (TRUST_LEN_MAX - 2), validStack.end() - 2); // 2 : end 2 lines
        } else if (validStack.size() > TRUST_LEN_MAX) {
            // keep the begin 2 lines and the end 28 lines
            validStack.erase(validStack.begin() + 2, validStack.end() - (TRUST_LEN_MAX - 2)); // 2 : begin 2 lines
        }
    }

    // get f1f2f3 from the lastPart
    for (auto part : multiPart) {
        // multiPart has at least 2 parts
        if (size > 1 && !part.empty() && HasExceptionList(part.front())) {
            part.erase(part.begin());
        }
        // lastPart should has at least 3 lines
        if (!part.empty()) {
            reverse(part.begin(), part.end());
            lastPart = part;
            break;
        }
    }
    return TBox::ARRAY_STR + StringUtil::VectorToString(validStack, false);
}

vector<string> LogParse::GetValidStack(size_t num, stack<string>& inStack) const
{
    stack<string> src = inStack;
    vector<string> validStack;
    stack<string> outStatck;
    string stackName;
    size_t len = src.size();
    for (size_t i = 0; i < len; i++) {
        stackName = TBox::GetStackName(src.top());  // extract function name from the stack
        if (!IsIgnoreLibrary(stackName)) {
            validStack.push_back(stackName);
        }
        src.pop();
    }
    if (validStack.empty()) {
        MatchIgnoreLibrary(inStack, outStatck, num);
        len = outStatck.size();
        for (size_t i = 0; i < len; i++) {
            stackName = TBox::GetStackName(outStatck.top());
            validStack.push_back(stackName);
            outStatck.pop();
        }
    }
    return validStack;
}

string LogParse::MatchExceptionLibrary(const string& val)
{
    for (auto& str : LogParse::exceptionList_) {
        if (val.find(str, 0) != string::npos) {
            return str;
        }
    }
    return UNMATCHED_EXCEPTION;
}

void LogParse::MatchIgnoreLibrary(stack<string> inStack, stack<string>& outStack, size_t num) const
{
    if (inStack.size() <= num) {
        outStack = inStack;
        return;
    }
    int count = 0;
    int size = ignoreList_.size();
    for (auto it = ignoreList_.rbegin(); it != ignoreList_.rend(); ++it) {
        if (count == size - 1) {
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
 *  trace : last part trace to get FNAME
 *  return string : valid trace spliting by "\n"
 */
std::string LogParse::GetFilterTrace(const std::string& info, std::vector<std::string>& trace) const
{
    StringUtil::SplitStr(info, "\n", trace, false, false);
    std::stack<std::string> traceStack;
    for (const auto& str : trace) {
        traceStack.push(str);
    }
    trace.clear();
    return GetValidBlock(traceStack, trace);
}

void LogParse::SetFname(std::stack<std::string>& stack, std::map<std::string, std::string>& eventInfo) const
{
    std::vector<std::string> name = {"F1NAME", "F2NAME", "F3NAME"};
    size_t len = stack.size();
    for (size_t i = 0; i < len; i++) {
        if (eventInfo.find(name[i]) == eventInfo.end()) {
            eventInfo[name[i]] = stack.top();
        }
        stack.pop();
    }
}

bool LogParse::HasExceptionList(const string& line) const
{
    auto iter = exceptionList_.find(line);
    if (line == UNMATCHED_EXCEPTION || iter != exceptionList_.end()) {
        return true;
    }
    return false;
}
}
}