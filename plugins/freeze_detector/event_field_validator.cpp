/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "event_field_validator.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <sys/stat.h>
#include <vector>

#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D01, "EventFieldValidator");
namespace {
/* /etc/passwd: system services stay below the app user base (app = 10000) */
constexpr int32_t TRUSTED_SENDER_UID_MAX = 10000;
constexpr size_t MAX_NAME_LEN = 256;
constexpr size_t MAX_PATH_LEN = 512;
constexpr size_t MAX_SMALL_CONTENT_LEN = 8192;
constexpr size_t MAX_LARGE_CONTENT_LEN = 256 * 1024;
constexpr size_t MAX_EVENT_JSON_LEN = 2 * 1024 * 1024;

constexpr int32_t KERNEL_UID = 0;
constexpr int32_t GRAPHICS_UID = 1003;
constexpr int32_t HIVIEW_UID = 1201;
constexpr int32_t FOUNDATION_UID = 5523;
constexpr int32_t POWERMGR_UID = 5528;
constexpr int32_t INPUT_UID = 6696;

struct SenderPolicy {
    bool wholeDomainAllowed = false;
    std::set<std::string> allowedEvents;
    std::set<int32_t> allowedSystemUids;
};

const std::map<std::string, SenderPolicy>& GetDomainSenderPolicies()
{
    static const std::map<std::string, SenderPolicy> policies = {
        {"AAFWK", {false, {"BUSSINESS_THREAD_BLOCK_3S", "BUSSINESS_THREAD_BLOCK_6S",
            "BUSINESS_INPUT_BLOCK", "FREEZE_HALF_HIVIEW_LOG"}, {FOUNDATION_UID}}},
        {"ACE", {true, {}, {}}},
        {"FFRT", {true, {}, {}}},
        {"FRAMEWORK", {false, {"APP_HICOLLIE", "HIT_EMPTY_WARNING"}, {}}},
        {"GRAPHIC", {false, {"NO_DRAW", "JANK_FRAME_SKIP"}, {GRAPHICS_UID}}},
        {"GRAPHICS_GAME", {true, {}, {}}},
        {"WEBVIEW", {true, {}, {}}},
        {"KERNEL_VENDOR", {false, {}, {KERNEL_UID, HIVIEW_UID}}},
        {"MULTIMODALINPUT", {false, {}, {INPUT_UID}}},
        {"POWER", {false, {}, {POWERMGR_UID}}},
        {"RELIABILITY", {false, {}, {HIVIEW_UID, FOUNDATION_UID}}},
        {"THP", {false, {}, {}}},
        {"WINDOWMANAGER", {false, {}, {FOUNDATION_UID}}},
    };
    return policies;
}

const char* PATH_FIELD_ALLOWED_PREFIX[] = {"/data/log/", "/data/service/el2/"};

constexpr char FREEZE_LOG_PATH_PREFIX[] = "/data/log/eventlog/freeze/";
constexpr char STACK_PATH_SUFFIX[] = "_stack";
constexpr size_t STACK_PATH_SUFFIX_LEN = 6;
constexpr char BINDER_PATH_SUFFIX[] = "_binder";
constexpr size_t BINDER_PATH_SUFFIX_LEN = 7;

const std::vector<std::string>& GetNumericFields()
{
    static const std::vector<std::string> fields = {
        "PID", "UID", "TID", "INPUT_ID", "REMOTE_PID",
        "LAST_DISPATCH_EVENTID", "LAST_PROCESS_EVENTID", "LAST_MARKED_EVENTID"
    };
    return fields;
}

const std::vector<std::string>& GetNameFields()
{
    static const std::vector<std::string> fields = {
        "PACKAGE_NAME", "PROCESS_NAME", "SPECIFICSTACK_NAME", "TASK_NAME",
        "MODULE_NAME", "PNAMEID"
    };
    return fields;
}

const std::vector<std::string>& GetSmallContentFields()
{
    static const std::vector<std::string> fields = {
        "HITRACE_ID", "APP_RUNNING_UNIQUE_ID", "FAULT_TIME", "EVENT_TYPE"
    };
    return fields;
}

const std::vector<std::string>& GetLargeContentFields()
{
    static const std::vector<std::string> fields = {
        "MSG", "STACK", "MAIN_STACK", "EXTERNAL_LOG", "APPLICATION_HEAP_INFO",
        "APPLICATION_GC_INFO", "APPLICATION_IO_INFO", "PROCESS_LIFECYCLE_INFO"
    };
    return fields;
}

const std::vector<std::string>& GetPathFields()
{
    static const std::vector<std::string> fields = {"FREEZE_INFO_PATH"};
    return fields;
}

/* roots FreezeManager::GetAppFreezeFile may legitimately read and delete from */
const char* READ_PATH_ALLOWED_PREFIX[] = {"/data/log/", "/data/app/el2/"};
} // namespace

bool EventFieldValidator::IsDecimalValue(const std::string& value)
{
    if (value.empty() || value.size() > 10) { // 10 digits keep the value inside int32 range
        return false;
    }
    return value.find_first_not_of("0123456789") == std::string::npos;
}

bool EventFieldValidator::ToInt32Value(const std::string& value, int32_t& out)
{
    if (!IsDecimalValue(value)) {
        return false;
    }
    errno = 0;
    long long result = strtoll(value.c_str(), nullptr, 10);
    if (errno != 0 || result <= 0 || result > INT_MAX) {
        return false;
    }
    out = static_cast<int32_t>(result);
    return true;
}

bool EventFieldValidator::IsSafeName(const std::string& value)
{
    if (value.empty() || value.size() > MAX_NAME_LEN) {
        return false;
    }
    return value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._:-")
        == std::string::npos;
}

bool EventFieldValidator::IsOwnerOfPath(const std::string& path, int32_t senderUid)
{
    struct stat st = {};
    if (lstat(path.c_str(), &st) != 0) {
        return true; // only existing files are consumed by the log merge flow
    }
    return static_cast<int32_t>(st.st_uid) == senderUid;
}

bool EventFieldValidator::IsAcceptedPath(const std::string& path, int32_t senderUid)
{
    if (path.empty()) {
        return true;
    }
    if (path.size() > MAX_PATH_LEN || path.front() != '/') {
        return false;
    }
    if (path.find("..") != std::string::npos || path.find_first_not_of(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._/-") != std::string::npos) {
        return false;
    }
    bool inAllowedRoot = false;
    for (auto prefix : PATH_FIELD_ALLOWED_PREFIX) {
        if (path.compare(0, strlen(prefix), prefix) == 0) {
            inAllowedRoot = true;
            break;
        }
    }
    if (!inAllowedRoot) {
        return false;
    }
    return IsOwnerOfPath(path, senderUid);
}

bool EventFieldValidator::IsAcceptedReadPath(const std::string& path)
{
    if (path.empty()) {
        return true;
    }
    if (path.size() > MAX_PATH_LEN || path.front() != '/') {
        return false;
    }
    if (path.find("..") != std::string::npos || path.find_first_not_of(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._/-") != std::string::npos) {
        return false;
    }
    for (auto prefix : READ_PATH_ALLOWED_PREFIX) {
        if (path.compare(0, strlen(prefix), prefix) == 0) {
            return true;
        }
    }
    return false;
}
 
bool EventFieldValidator::ContainPathTraversal(const std::string& path)
{
    return path.find("..") != std::string::npos ||
        path.find('\\') != std::string::npos ||
        path.find('\0') != std::string::npos;
}

bool EventFieldValidator::CheckStackAndBinderPaths(const std::shared_ptr<SysEvent>& event)
{
    std::string stack = event->GetEventValue("STACK");
    if (!stack.empty() && FileUtil::FileExists(stack)) {
        if (ContainPathTraversal(stack)) {
            HIVIEW_LOGE("invalid stack info: %{public}s", stack.c_str());
            return false;
        }
        if (stack.find(FREEZE_LOG_PATH_PREFIX) != 0 ||
            stack.rfind(STACK_PATH_SUFFIX) != stack.size() - STACK_PATH_SUFFIX_LEN) {
            HIVIEW_LOGW("event:%{public}s rejected: STACK path not under freeze root or invalid suffix",
                event->eventName_.c_str());
            return false;
        }
    }

    std::string mainStack = event->GetEventValue("MAIN_STACK");
    if (!mainStack.empty() && mainStack.front() == '/') {
        HIVIEW_LOGW("event:%{public}s rejected: MAIN_STACK looks like a path", event->eventName_.c_str());
        return false;
    }

    std::string binderInfo = event->GetEventValue("BINDER_INFO");
    if (!binderInfo.empty()) {
        size_t commaPos = binderInfo.find(',');
        std::string binderPath = (commaPos != std::string::npos) ? binderInfo.substr(0, commaPos) : "";
        if (!binderPath.empty() && FileUtil::FileExists(binderPath)) {
            if (ContainPathTraversal(binderPath) ||
                binderPath.find(FREEZE_LOG_PATH_PREFIX) != 0 ||
                binderPath.rfind(BINDER_PATH_SUFFIX) != binderPath.size() - BINDER_PATH_SUFFIX_LEN) {
                HIVIEW_LOGW("event:%{public}s: invalid BINDER_INFO path, clearing field",
                    event->eventName_.c_str());
                event->SetEventValue("BINDER_INFO", "");
            }
        }
    }
    return true;
}

bool EventFieldValidator::IsTrustedSource(const std::shared_ptr<SysEvent>& event)
{
    return event->GetUid() <= TRUSTED_SENDER_UID_MAX;
}

bool EventFieldValidator::IsSenderAllowed(const std::string& domain, const std::string& eventName, int32_t senderUid)
{
    auto it = GetDomainSenderPolicies().find(domain);
    if (it == GetDomainSenderPolicies().end()) {
        return true;
    }
    const SenderPolicy& policy = it->second;
    if (senderUid > TRUSTED_SENDER_UID_MAX) {
        return policy.wholeDomainAllowed || policy.allowedEvents.count(eventName) > 0;
    }
    if (!policy.allowedSystemUids.empty() && policy.allowedSystemUids.count(senderUid) == 0) {
        return false;
    }
    return true;
}

bool EventFieldValidator::CheckIdentityFields(const std::shared_ptr<SysEvent>& event)
{
    int32_t senderPid = event->GetPid();
    if (senderPid <= 0) {
        HIVIEW_LOGW("event:%{public}s has no credible sender pid", event->eventName_.c_str());
        return false;
    }
    int32_t senderUid = event->GetUid();
    // numeric params do not surface through the string-typed GetEventValue;
    // read both forms so int-typed PID/UID (the common producer case) binds too
    int64_t bodyPid = event->GetEventIntValue("PID");
    std::string pidStr = event->GetEventValue("PID");
    if (bodyPid != 0) {
        if (bodyPid != static_cast<int64_t>(senderPid)) {
            HIVIEW_LOGW("event:%{public}s rejected: PID field does not match sender pid", event->eventName_.c_str());
            return false;
        }
    } else if (!pidStr.empty()) {
        int32_t parsed = 0;
        if (!ToInt32Value(pidStr, parsed) || parsed != senderPid) {
            HIVIEW_LOGW("event:%{public}s rejected: PID field does not match sender pid", event->eventName_.c_str());
            return false;
        }
    }
    int64_t bodyUid = event->GetEventIntValue("UID");
    std::string uidStr = event->GetEventValue("UID");
    if (bodyUid != 0) {
        if (bodyUid != static_cast<int64_t>(senderUid)) {
            HIVIEW_LOGW("event:%{public}s rejected: UID field does not match sender uid", event->eventName_.c_str());
            return false;
        }
    } else if (!uidStr.empty()) {
        int32_t parsed = 0;
        if (!ToInt32Value(uidStr, parsed) || parsed != senderUid) {
            HIVIEW_LOGW("event:%{public}s rejected: UID field does not match sender uid", event->eventName_.c_str());
            return false;
        }
    }
    std::string tidStr = event->GetEventValue("TID");
    int32_t bodyTid = 0;
    if (!tidStr.empty() && !ToInt32Value(tidStr, bodyTid)) {
        HIVIEW_LOGW("event:%{public}s rejected: invalid TID field", event->eventName_.c_str());
        return false;
    }
    return true;
}

bool EventFieldValidator::CheckNumericFields(const std::shared_ptr<SysEvent>& event)
{
    for (const auto& field : GetNumericFields()) {
        std::string value = event->GetEventValue(field);
        if (value.empty()) {
            continue;
        }
        int32_t dummy = 0;
        if (!ToInt32Value(value, dummy)) {
            HIVIEW_LOGW("event:%{public}s rejected: invalid numeric field %{public}s",
                event->eventName_.c_str(), field.c_str());
            return false;
        }
    }
    return true;
}

bool EventFieldValidator::CheckNameFields(const std::shared_ptr<SysEvent>& event)
{
    for (const auto& field : GetNameFields()) {
        std::string value = event->GetEventValue(field);
        if (value.empty()) {
            continue;
        }
        if (!IsSafeName(value)) {
            HIVIEW_LOGW("event:%{public}s rejected: unsafe name field %{public}s",
                event->eventName_.c_str(), field.c_str());
            return false;
        }
    }
    return true;
}

bool EventFieldValidator::CheckPathFields(const std::shared_ptr<SysEvent>& event)
{
    int32_t senderUid = event->GetUid();
    for (const auto& field : GetPathFields()) {
        std::string value = event->GetEventValue(field);
        if (value.empty()) {
            continue;
        }
        if (!IsAcceptedPath(value, senderUid)) {
            HIVIEW_LOGW("event:%{public}s rejected: unsafe path field %{public}s",
                event->eventName_.c_str(), field.c_str());
            return false;
        }
    }
    return true;
}

bool EventFieldValidator::CheckContentFields(const std::shared_ptr<SysEvent>& event)
{
    if (event->AsJsonStr().size() > MAX_EVENT_JSON_LEN) {
        HIVIEW_LOGW("event:%{public}s rejected: oversized event json", event->eventName_.c_str());
        return false;
    }
    for (const auto& field : GetSmallContentFields()) {
        std::string value = event->GetEventValue(field);
        if (value.size() > MAX_SMALL_CONTENT_LEN) {
            HIVIEW_LOGW("event:%{public}s rejected: oversized field %{public}s",
                event->eventName_.c_str(), field.c_str());
            return false;
        }
    }
    for (const auto& field : GetLargeContentFields()) {
        std::string value = event->GetEventValue(field);
        if (value.size() > MAX_LARGE_CONTENT_LEN) {
            HIVIEW_LOGW("event:%{public}s rejected: oversized field %{public}s",
                event->eventName_.c_str(), field.c_str());
            return false;
        }
    }
    std::string appRunningUniqueId = event->GetEventValue("APP_RUNNING_UNIQUE_ID");
    if (!appRunningUniqueId.empty() && appRunningUniqueId.find_first_not_of("0123456789") != std::string::npos) {
        HIVIEW_LOGW("event:%{public}s rejected: APP_RUNNING_UNIQUE_ID is not all digits",
            event->eventName_.c_str());
        return false;
    }
    return true;
}

bool EventFieldValidator::StripUntrustedBinderInfo(const std::shared_ptr<SysEvent>& event)
{
    if (!event->GetEventValue("HICOLLIE_BINDER_INFO").empty()) {
        event->SetEventValue("HICOLLIE_BINDER_INFO", "");
        HIVIEW_LOGI("event:%{public}s from untrusted sender: HICOLLIE_BINDER_INFO dropped",
            event->eventName_.c_str());
    }
    return true;
}

bool EventFieldValidator::ValidateUntrustedEvent(const std::shared_ptr<SysEvent>& event)
{
    if (!CheckIdentityFields(event) || !CheckNumericFields(event) || !CheckNameFields(event) ||
        !CheckPathFields(event) || !CheckContentFields(event)) {
        return false;
    }
    return StripUntrustedBinderInfo(event);
}

bool EventFieldValidator::ValidateEvent(const std::shared_ptr<SysEvent>& event)
{
    if (event == nullptr) {
        return false;
    }
    if (!IsSenderAllowed(event->domain_, event->eventName_, event->GetUid())) {
        HIVIEW_LOGW("event[%{public}s|%{public}s] rejected: sender uid %{public}d is not allowed for this domain",
            event->domain_.c_str(), event->eventName_.c_str(), event->GetUid());
        return false;
    }
    if (!CheckStackAndBinderPaths(event)) {
        return false;
    }
    if (IsTrustedSource(event)) {
        return true;
    }
    return ValidateUntrustedEvent(event);
}
} // namespace HiviewDFX
} // namespace OHOS
