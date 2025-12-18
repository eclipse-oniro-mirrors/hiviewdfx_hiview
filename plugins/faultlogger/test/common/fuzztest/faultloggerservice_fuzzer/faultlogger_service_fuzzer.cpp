/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "faultlogger.h"
#include "faultlogger_service_ohos.h"
#include "faultlogger_service_fuzzer.h"
#include "faultlogger_fuzzertest_common.h"
#include "hiview_global.h"
#include "hiview_platform.h"
#include "sys_event.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr int FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH = 50;
constexpr int32_t FAULTLOGTYPE_SIZE = 6;
}

class HiviewTestContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type __UNUSED)
    {
        return "/data/log/hiview/sys_event_test";
    }
};

std::shared_ptr<Faultlogger> CreateFaultloggerInstance()
{
    static std::unique_ptr<HiviewPlatform> platform = std::make_unique<HiviewPlatform>();
    auto plugin = std::make_shared<Faultlogger>();
    plugin->SetName("Faultlogger");
    plugin->SetHandle(nullptr);
    plugin->SetHiviewContext(platform.get());
    plugin->OnLoad();
    return plugin;
}

void FuzzServiceInterfaceDump(const uint8_t* data, size_t size)
{
    constexpr int maxLen = 20;
    int32_t fd;
    if (size <= (sizeof(fd) + maxLen)) {
        return;
    }

    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService();

    STREAM_TO_VALUEINFO(data, fd);
    std::vector<std::u16string> args;
    char16_t arg[maxLen] = {0};
    errno_t err = strncpy_s(reinterpret_cast<char*>(arg), sizeof(arg), reinterpret_cast<const char*>(data), maxLen);
    if (err != EOK) {
        std::cout << "strncpy_s arg failed" << std::endl;
        return;
    }
    args.push_back(arg);

    (void)serviceOhos.Dump(fd, args);
}

void FuzzServiceInterfaceQuerySelfFaultLog(const uint8_t* data, size_t size)
{
    HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);

    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService();
    int32_t faultType;
    int32_t maxNum;
    auto offsetTotalLength = sizeof(faultType) + sizeof(maxNum);
    if (offsetTotalLength > size) {
        return;
    }

    STREAM_TO_VALUEINFO(data, faultType);
    STREAM_TO_VALUEINFO(data, maxNum);

    auto remoteObject = serviceOhos.QuerySelfFaultLog(faultType, maxNum);
    auto result = iface_cast<FaultLogQueryResultOhos>(remoteObject);
    if (result != nullptr) {
        while (result->HasNext()) {
            result->GetNext();
        }
    }
}

void FuzzServiceInterfaceAddFaultLog(const uint8_t* data, size_t size)
{
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService();
    FaultLogInfoOhos info;
    int32_t faultLogType {0};
    auto offsetTotalLength = sizeof(info.time) + sizeof(info.pid) + sizeof(info.uid) + sizeof(faultLogType) +
                            (6 * FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH); // 6 : Offset by 6 string length
    if (offsetTotalLength > size) {
        return;
    }

    STREAM_TO_VALUEINFO(data, info.time);
    STREAM_TO_VALUEINFO(data, info.pid);
    STREAM_TO_VALUEINFO(data, info.uid);
    STREAM_TO_VALUEINFO(data, faultLogType);
    info.faultLogType = abs(faultLogType % 10); // 10 : get the absolute value of the last digit of the number

    std::string module(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.module = module;
    std::string reason(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.reason = reason;
    std::string logPath(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.logPath = logPath;
    std::string registers(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.registers = registers;
    std::string hilog(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.sectionMaps["HILOG"] = hilog;
    std::string keyLogFile(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.sectionMaps["KEYLOGFILE"] = keyLogFile;
    serviceOhos.AddFaultLog(info);
    serviceOhos.Destroy();
}

void FuzzServiceInterfaceOnEvent(const uint8_t* data, size_t size)
{
    auto service = CreateFaultloggerInstance();

    int32_t pid;
    int32_t uid;
    int32_t tid;
    auto offsetTotalLength = sizeof(pid) + sizeof(uid) + sizeof(tid) +
                            (7 * FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH); // 7 : Offset by 7 string length
    if (offsetTotalLength > size) {
        return;
    }

    STREAM_TO_VALUEINFO(data, pid);
    STREAM_TO_VALUEINFO(data, uid);
    STREAM_TO_VALUEINFO(data, tid);

    std::string domain(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    std::string eventName(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    SysEventCreator sysEventCreator(domain, eventName, SysEventCreator::FAULT);
    std::map<std::string, std::string> bundle;
    std::string hilog(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    bundle["HILOG"] = hilog;
    std::string keyLogFile(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    bundle["KEYLOGFILE"] = keyLogFile;

    std::string summary(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    std::string packageName(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    sysEventCreator.SetKeyValue("name_", "JS_ERROR");
    sysEventCreator.SetKeyValue("pid_", pid);
    sysEventCreator.SetKeyValue("uid_", uid);
    sysEventCreator.SetKeyValue("tid_", tid);
    sysEventCreator.SetKeyValue("SUMMARY", summary);
    sysEventCreator.SetKeyValue("PACKAGE_NAME", packageName);
    sysEventCreator.SetKeyValue("bundle_", bundle);
    std::string desc(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    auto sysEvent = std::make_shared<SysEvent>(desc, nullptr, sysEventCreator);
    auto event = std::dynamic_pointer_cast<Event>(sysEvent);
    service->OnEvent(event);
}

void FuzzServiceInterfaceGwpAsanGrayscale(const uint8_t* data, size_t size)
{
    HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);

    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService();
    bool alwaysEnabled;
    double sampleRate;
    double maxSimutaneousAllocations;
    int32_t duration;
    auto offsetTotalLength = sizeof(alwaysEnabled) + sizeof(sampleRate) +
        sizeof(maxSimutaneousAllocations) + sizeof(duration);
    if (offsetTotalLength > size) {
        return;
    }

    STREAM_TO_VALUEINFO(data, alwaysEnabled);
    STREAM_TO_VALUEINFO(data, sampleRate);
    STREAM_TO_VALUEINFO(data, maxSimutaneousAllocations);
    STREAM_TO_VALUEINFO(data, duration);

    serviceOhos.EnableGwpAsanGrayscale(1, 1000, 2000, 5); // 1000 - 2000 is test value, 5 is test duration
    serviceOhos.DisableGwpAsanGrayscale();
    serviceOhos.GetGwpAsanGrayscaleState();
}

void FuzzFaultloggerServiceInterface(const uint8_t* data, size_t size)
{
    FuzzServiceInterfaceDump(data, size);
    FuzzServiceInterfaceQuerySelfFaultLog(data, size);
    FuzzServiceInterfaceAddFaultLog(data, size);
    FuzzServiceInterfaceOnEvent(data, size);
    FuzzServiceInterfaceGwpAsanGrayscale(data, size);
    usleep(10000); // 10000 : pause for 10000 microseconds to avoid resource depletion
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFaultloggerServiceInterface(data, size);
    return 0;
}
