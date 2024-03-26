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
#include "faultlog_manager.h"
#include "faultlogger_service_ohos.h"
#include "faultlogger_service_fuzzer.h"
#include "hiview_platform.h"
#include "securec.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
const int32_t FAULTLOGTYPE_SIZE = 6;
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
    auto service = CreateFaultloggerInstance();
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService(service.get());
    if (FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr) != service.get()) {
        printf("FaultloggerServiceOhos start service error.\n");
        return;
    }

    int32_t fd = static_cast<int32_t>(*data);
    std::vector<std::u16string> args;
    constexpr int maxLen = 20;
    char16_t arg[maxLen] = {0};
    errno_t err = strncpy_s(reinterpret_cast<char*>(arg), sizeof(arg), reinterpret_cast<const char*>(data), size);
    if (err != EOK) {
        std::cout << "strncpy_s arg failed" << std::endl;
        return;
    }
    args.push_back(arg);

    (void)serviceOhos.Dump(fd, args);
}

void FuzzServiceInterfaceQuerySelfFaultLog(const uint8_t* data, size_t size)
{
    auto service = CreateFaultloggerInstance();
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService(service.get());
    if (FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr) != service.get()) {
        printf("FaultloggerServiceOhos start service error.\n");
        return;
    }
    int32_t faultType = static_cast<int32_t>(*data);
    int32_t maxNum = static_cast<int32_t>(*data);
    auto remoteObject = serviceOhos.QuerySelfFaultLog(faultType, maxNum);
    auto result = iface_cast<FaultLogQueryResultOhos>(remoteObject);
    if (result != nullptr) {
        while (result->HasNext()) {
            result->GetNext();
        }
    }
}

void FuzzServiceInterfaceCreateTempFaultLogFile(const uint8_t* data, size_t size)
{
    auto faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    faultLogManager->Init();

    int64_t time = static_cast<int64_t>(*data);
    int32_t id = static_cast<int32_t>(*data);
    int32_t faultType = static_cast<int32_t>(*data);
    std::string module = std::string(reinterpret_cast<const char*>(data), size);
    faultLogManager->CreateTempFaultLogFile(time, id, faultType, module);
}

void FuzzServiceInterfaceAddFaultLog(const uint8_t* data, size_t size)
{
    auto service = CreateFaultloggerInstance();
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService(service.get());
    if (FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr) != service.get()) {
        printf("FaultloggerServiceOhos start service error.\n");
        return;
    }
    FaultLogInfoOhos info;
    info.time = static_cast<int64_t>(*data);
    info.pid = static_cast<int32_t>(*data);
    info.uid = static_cast<int32_t>(*data);
    info.faultLogType = static_cast<int32_t>(*data) % FAULTLOGTYPE_SIZE;
    info.module = std::string(reinterpret_cast<const char*>(data), size);
    info.logPath = std::string(reinterpret_cast<const char*>(data), size);
    info.reason = std::string(reinterpret_cast<const char*>(data), size);
    info.registers = std::string(reinterpret_cast<const char*>(data), size);
    info.sectionMaps["HILOG"] = std::string(reinterpret_cast<const char*>(data), size);
    info.sectionMaps["KEYLOGFILE"] = std::string(reinterpret_cast<const char*>(data), size);
    serviceOhos.AddFaultLog(info);
    serviceOhos.Destroy();
}

void FuzzServiceInterfaceGetFaultLogInfo(const uint8_t* data, size_t size)
{
    auto service = CreateFaultloggerInstance();
    std::vector<std::string> files;
    FileUtil::GetDirFiles("/data/log/faultlog/temp/", files);
    for (const auto& file : files) {
        service->GetFaultLogInfo(file);
    }
}

void FuzzServiceInterfaceOnEvent(const uint8_t* data, size_t size)
{
    auto service = CreateFaultloggerInstance();
    std::string domain = std::string(reinterpret_cast<const char*>(data), size);
    std::string eventName = std::string(reinterpret_cast<const char*>(data), size);
    SysEventCreator sysEventCreator(domain, eventName, SysEventCreator::FAULT);
    std::map<std::string, std::string> bundle;
    bundle["HILOG"] = std::string(reinterpret_cast<const char*>(data), size);
    bundle["KEYLOGFILE"] = std::string(reinterpret_cast<const char*>(data), size);
    sysEventCreator.SetKeyValue("name_", "JS_ERROR");
    sysEventCreator.SetKeyValue("pid_", static_cast<int32_t>(*data));
    sysEventCreator.SetKeyValue("uid_", static_cast<int32_t>(*data));
    sysEventCreator.SetKeyValue("tid_", static_cast<int32_t>(*data));
    sysEventCreator.SetKeyValue("SUMMARY", std::string(reinterpret_cast<const char*>(data), size));
    sysEventCreator.SetKeyValue("PACKAGE_NAME", std::string(reinterpret_cast<const char*>(data), size));
    sysEventCreator.SetKeyValue("bundle_", bundle);
    std::string desc = std::string(reinterpret_cast<const char*>(data), size);
    auto sysEvent = std::make_shared<SysEvent>(desc, nullptr, sysEventCreator);
    auto event = std::dynamic_pointer_cast<Event>(sysEvent);
    service->OnEvent(event);
}

void FuzzFaultloggerServiceInterface(const uint8_t* data, size_t size)
{
    FuzzServiceInterfaceDump(data, size);
    FuzzServiceInterfaceQuerySelfFaultLog(data, size);
    FuzzServiceInterfaceCreateTempFaultLogFile(data, size);
    FuzzServiceInterfaceAddFaultLog(data, size);
    FuzzServiceInterfaceGetFaultLogInfo(data, size);
    FuzzServiceInterfaceOnEvent(data, size);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::FuzzFaultloggerServiceInterface(data, size);
    return 0;
}
