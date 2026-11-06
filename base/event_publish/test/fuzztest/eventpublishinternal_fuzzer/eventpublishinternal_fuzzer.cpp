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

#include <memory>
#include <string>
#include <vector>

#define private public
#define protected public
#include "app_event_publisher_factory.h"
#include "event_publish.cpp"
#undef private
#undef protected
#include "user_data_size_reporter.h"
#include "eventpublishinternal_fuzzer.h"

#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FUZZ_MAX_STR = 32;
constexpr int FUZZ_SELECTOR_BYTES = 1;
constexpr int FUZZ_MAX_OBSERVER_NUM = 100;

std::string MakeStr(const uint8_t*& data, size_t& remain)
{
    std::string s;
    if (remain >= FUZZ_MAX_STR) {
        s = std::string(reinterpret_cast<const char*>(data), FUZZ_MAX_STR);
        data += FUZZ_MAX_STR;
        remain -= FUZZ_MAX_STR;
    }
    return s;
}

std::string MakeEventName(const uint8_t*& data, size_t& remain)
{
    std::string s;
    for (size_t i = 0; i < FUZZ_MAX_STR && remain > 0; ++i) {
        char c = static_cast<char>(*data);
        data++;
        remain--;
        if (c == '\0') {
            break;
        }
        s.push_back(c);
    }
    return s;
}

template <typename T>
bool ReadValue(const uint8_t*& data, size_t& remain, T& value)
{
    if (remain < sizeof(T)) {
        return false;
    }
    errno_t err = memcpy_s(&value, sizeof(T), data, sizeof(T));
    if (err != EOK) {
        return false;
    }
    data += sizeof(T);
    remain -= sizeof(T);
    return true;
}

void FuzzCheckAppListenedEvents(const uint8_t*& data, size_t& remain)
{
    std::string path = MakeStr(data, remain);
    std::string eventName = MakeEventName(data, remain);
    CheckAppListenedEvents(path, eventName);
}

void FuzzParseParamJson(const uint8_t*& data, size_t& remain)
{
    std::string paramJson = MakeStr(data, remain);
    Json::Value params;
    ParseParamJson(paramJson, params);
}

void FuzzIsDmpFile(const uint8_t*& data, size_t& remain)
{
    std::string dmpPath = MakeStr(data, remain);
    IsDmpFile(dmpPath);
}

void FuzzVerifyPathSecurity(const uint8_t*& data, size_t& remain)
{
    std::string path = MakeStr(data, remain);
    VerifyPathSecurity(path);
}

void FuzzGetTempFilePath(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    GetTempFilePath(uid);
}

void FuzzGetLinkFileName(const uint8_t*& data, size_t& remain)
{
    std::string desFileName = MakeStr(data, remain);
    int32_t observerNum = 0;
    if (!ReadValue(data, remain, observerNum)) {
        return;
    }
    observerNum = observerNum % FUZZ_MAX_OBSERVER_NUM;
    std::vector<std::string> linkExternalLogs;
    GetLinkFileName(desFileName, observerNum, linkExternalLogs);
}

void FuzzRemoveEventInternalField(const uint8_t*& data, size_t& remain)
{
    std::string jsonStr = MakeStr(data, remain);
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value eventJson;
    std::string errs;
    reader->parse(jsonStr.data(), jsonStr.data() + jsonStr.size(), &eventJson, &errs);
    RemoveEventInternalField(eventJson);
}

void FuzzParseString(const uint8_t*& data, size_t& remain)
{
    std::string jsonStr = MakeStr(data, remain);
    std::string key = MakeEventName(data, remain);
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    std::string errs;
    reader->parse(jsonStr.data(), jsonStr.data() + jsonStr.size(), &root, &errs);
    ParseString(root, key);
}

void FuzzCheckInSandBoxLog(const uint8_t*& data, size_t& remain)
{
    std::string curLogPath = MakeStr(data, remain);
    std::string sandBoxLogPath = MakeStr(data, remain);
    std::string eventName = MakeStr(data, remain);
    Json::Value externalLogJson(Json::arrayValue);
    bool logOverLimit = false;
    CheckInSandBoxLog(curLogPath, sandBoxLogPath, externalLogJson, logOverLimit, eventName);
}

void FuzzGetDesFileName(const uint8_t*& data, size_t& remain)
{
    std::string eventName = MakeEventName(data, remain);
    ExternalLogInfo info;
    info.extensionType = ".log";
    info.subPath = "hiappevent";
    constexpr uint64_t maxFileSize = 5 * 1024 * 1024;
    info.maxFileSize = maxFileSize;
    Json::Value params;
    int32_t pid = 0;
    if (ReadValue(data, remain, pid)) {
        params["pid"] = pid;
    }
    GetDesFileName(params, eventName, info);
}

void FuzzGetPathPlaceHolder(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    GetPathPlaceHolder(uid);
}

void FuzzGetBundleNameAndAppIndex(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string bundleName;
    int32_t appIndex = 0;
    GetBundleNameAndAppIndex(uid, bundleName, appIndex);
}

void FuzzGetInputMethodPathHolder(const uint8_t*& data, size_t& remain)
{
    std::string bundleName = MakeEventName(data, remain);
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    GetInputMethodPathHolder(bundleName, uid);
}

void FuzzGetEventObserverNum(const uint8_t*& data, size_t& remain)
{
    std::string eventName = MakeEventName(data, remain);
    ExternalLogInfo info;
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string pathHolder = MakeEventName(data, remain);
    GetEventObserverNum(eventName, info, uid, pathHolder);
}

void FuzzGetExternalLogInfo(const uint8_t*& data, size_t& remain)
{
    std::string eventName = MakeEventName(data, remain);
    ExternalLogInfo info;
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string pathHolder = MakeEventName(data, remain);
    GetExternalLogInfo(eventName, info, uid, pathHolder);
}

void FuzzCreateSandBox(const uint8_t*& data, size_t& remain)
{
    std::string dirPath = MakeStr(data, remain);
    CreateSandBox(dirPath);
}

void FuzzSaveEventToTempFile(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    Json::Value eventJson;
    eventJson["domain"] = "OS";
    eventJson["name"] = MakeEventName(data, remain);
    SaveEventToTempFile(uid, eventJson);
}

void FuzzWriteEventJson(const uint8_t*& data, size_t& remain)
{
    Json::Value eventJson;
    eventJson["domain"] = "OS";
    eventJson["name"] = MakeEventName(data, remain);
    std::string filePath = MakeStr(data, remain);
    WriteEventJson(eventJson, filePath);
}

void FuzzSaveEventToSandBox(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string pathHolder = MakeEventName(data, remain);
    Json::Value eventJson;
    eventJson["domain"] = "OS";
    eventJson["name"] = "APP_CRASH";
    SaveEventToSandBox(uid, pathHolder, eventJson);
}

void FuzzSaveLogToSandBox(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string pathHolder = MakeEventName(data, remain);
    Json::Value eventJson;
    eventJson["name"] = "APP_CRASH";
    eventJson["params"]["external_log"] = Json::Value(Json::arrayValue);
    eventJson["params"]["external_log"].append(MakeStr(data, remain));
    uint32_t maxFileSizeBytes = 0;
    if (!ReadValue(data, remain, maxFileSizeBytes)) {
        return;
    }
    SaveLogToSandBox(uid, pathHolder, eventJson, maxFileSizeBytes);
}

void FuzzCopyExternalLogsToSandBox(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    ExternalLogInfo info;
    info.extensionType = ".log";
    info.subPath = "hiappevent";
    constexpr uint64_t maxFileSize = 5 * 1024 * 1024;
    info.maxFileSize =maxFileSize;
    Json::Value eventJson;
    eventJson["name"] = "APP_CRASH";
    eventJson["params"]["external_log"] = Json::Value(Json::arrayValue);
    eventJson["params"]["external_log"].append(MakeStr(data, remain));
    uint32_t maxFileSizeBytes = 0;
    if (!ReadValue(data, remain, maxFileSizeBytes)) {
        return;
    }
    uint8_t needRefinedRaw = 0;
    if (!ReadValue(data, remain, needRefinedRaw)) {
        return;
    }
    CopyExternalLogsToSandBox(uid, info, eventJson, maxFileSizeBytes, needRefinedRaw != 0);
}

void FuzzCreateLinkFile(const uint8_t*& data, size_t& remain)
{
    std::string destPath = MakeStr(data, remain);
    std::vector<std::string> linkExternalLogs;
    linkExternalLogs.push_back(MakeStr(data, remain));
    CreateLinkFile(destPath, linkExternalLogs);
}

void FuzzCopyExternalLog(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string curLogPath = MakeStr(data, remain);
    std::string destPath = MakeStr(data, remain);
    uint32_t maxFileSizeByte = 0;
    if (!ReadValue(data, remain, maxFileSizeByte)) {
        return;
    }
    std::vector<std::string> linkExternalLogs;
    CopyExternalLog(uid, curLogPath, destPath, maxFileSizeByte, linkExternalLogs);
}

void FuzzSaveLinkExternalLog(const uint8_t*& data, size_t& remain)
{
    Json::Value eventJson;
    eventJson["name"] = "APP_CRASH";
    std::string path = MakeStr(data, remain);
    std::string sandBoxLogPath = MakeStr(data, remain);
    std::vector<std::string> linkExternalLogs;
    linkExternalLogs.push_back(MakeStr(data, remain));
    SaveLinkExternalLog(eventJson, path, sandBoxLogPath, linkExternalLogs);
}

void FuzzReportAppEventSend(const uint8_t*& data, size_t& remain)
{
    Json::Value eventJson;
    eventJson["name"] = MakeEventName(data, remain);
    eventJson["params"]["external_log"] = Json::Value(Json::arrayValue);
    eventJson["params"]["bundle_name"] = MakeStr(data, remain);
    ReportAppEventSend(eventJson);
}

void FuzzSetSandBoxAccess(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string dirPath = MakeStr(data, remain);
    SetSandBoxAccess(uid, dirPath);
}

void FuzzGetBundleManager(const uint8_t*& data, size_t& remain)
{
    GetBundleManager();
}

void FuzzGetPublisherRegistryMap(const uint8_t*& data, size_t& remain)
{
    AppEventPublisherFactory::GetPublisherRegistryMap();
}

void FuzzStartSendingThread(const uint8_t*& data, size_t& remain)
{
    auto impl = std::make_shared<EventPublish::Impl>();
    impl->StartSendingThread();
}

void FuzzSendEventToSandBox(const uint8_t*& data, size_t& remain)
{
    auto impl = std::make_shared<EventPublish::Impl>();
    impl->SendEventToSandBox();
}

void FuzzStartOverLimitThread(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string pathHolder = MakeEventName(data, remain);
    Json::Value eventJson;
    eventJson["name"] = "RESOURCE_OVERLIMIT";
    uint32_t maxFileSizeBytes = 0;
    if (!ReadValue(data, remain, maxFileSizeBytes)) {
        return;
    }
    auto impl = std::make_shared<EventPublish::Impl>();
    impl->StartOverLimitThread(uid, pathHolder, eventJson, maxFileSizeBytes);
}

void FuzzSendOverLimitEventToSandBox(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string pathHolder = MakeEventName(data, remain);
    Json::Value eventJson;
    eventJson["name"] = "RESOURCE_OVERLIMIT";
    uint32_t maxFileSizeBytes = 0;
    if (!ReadValue(data, remain, maxFileSizeBytes)) {
        return;
    }
    auto impl = std::make_shared<EventPublish::Impl>();
    impl->SendOverLimitEventToSandBox(uid, pathHolder, eventJson, maxFileSizeBytes);
}

using FuzzFunc = void(*)(const uint8_t*&, size_t&);

const FuzzFunc FUZZ_FUNCS[] = {
    FuzzCheckAppListenedEvents,
    FuzzParseParamJson,
    FuzzIsDmpFile,
    FuzzVerifyPathSecurity,
    FuzzGetTempFilePath,
    FuzzGetLinkFileName,
    FuzzRemoveEventInternalField,
    FuzzParseString,
    FuzzCheckInSandBoxLog,
    FuzzGetDesFileName,
    FuzzGetPathPlaceHolder,
    FuzzGetBundleNameAndAppIndex,
    FuzzGetInputMethodPathHolder,
    FuzzGetEventObserverNum,
    FuzzGetExternalLogInfo,
    FuzzCreateSandBox,
    FuzzSaveEventToTempFile,
    FuzzWriteEventJson,
    FuzzSaveEventToSandBox,
    FuzzSaveLogToSandBox,
    FuzzCopyExternalLogsToSandBox,
    FuzzCreateLinkFile,
    FuzzCopyExternalLog,
    FuzzSaveLinkExternalLog,
    FuzzReportAppEventSend,
    FuzzSetSandBoxAccess,
    FuzzGetBundleManager,
    FuzzGetPublisherRegistryMap,
    FuzzStartSendingThread,
    FuzzSendEventToSandBox,
    FuzzStartOverLimitThread,
    FuzzSendOverLimitEventToSandBox,
};

void FuzzEventPublishInternal(const uint8_t* data, size_t size)
{
    if (size < FUZZ_SELECTOR_BYTES) {
        return;
    }
    constexpr size_t funcCount = sizeof(FUZZ_FUNCS) / sizeof(FUZZ_FUNCS[0]);
    uint8_t selector = data[0] % funcCount;
    data += FUZZ_SELECTOR_BYTES;
    size_t remain = size - FUZZ_SELECTOR_BYTES;
    FUZZ_FUNCS[selector](data, remain);
}
} // namespace HiviewDFX
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FuzzEventPublishInternal(data, size);
    return 0;
}
