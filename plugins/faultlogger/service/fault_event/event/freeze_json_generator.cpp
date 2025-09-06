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

#include "freeze_json_generator.h"
#include "freeze_json_util.h"

#include <map>
#include <list>
#include <string>

namespace OHOS {
namespace HiviewDFX {

FreezeJsonException::FreezeJsonException(const FreezeJsonException::Builder& builder)
    : name_(builder.name_),
    message_(builder.message_)
{
}

FreezeJsonException::Builder& FreezeJsonException::Builder::InitName(const std::string& name)
{
    name_ = name;
    return *this;
}

FreezeJsonException::Builder& FreezeJsonException::Builder::InitMessage(const std::string& message)
{
    message_ = message;
    return *this;
}

FreezeJsonException FreezeJsonException::Builder::Build() const
{
    FreezeJsonException freezeJsonException = FreezeJsonException(*this);
    return freezeJsonException;
}

std::string FreezeJsonException::JsonStr() const
{
    std::map<std::string, std::string> exceptionMap;
    exceptionMap[jsonExceptionName] = name_;
    exceptionMap[jsonExceptionMessage] = message_;
    return FreezeJsonUtil::GetStrByMap(exceptionMap);
}

FreezeJsonMemory::FreezeJsonMemory(const FreezeJsonMemory::Builder& builder)
    : rss_(builder.rss_),
    vss_(builder.vss_),
    pss_(builder.pss_),
    sysFreeMem_(builder.sysFreeMem_),
    sysAvailMem_(builder.sysAvailMem_),
    sysTotalMem_(builder.sysTotalMem_)
{
}

FreezeJsonMemory::Builder& FreezeJsonMemory::Builder::InitRss(uint64_t rss)
{
    rss_ = rss;
    return *this;
}

FreezeJsonMemory::Builder& FreezeJsonMemory::Builder::InitVss(uint64_t vss)
{
    vss_ = vss;
    return *this;
}

FreezeJsonMemory::Builder& FreezeJsonMemory::Builder::InitPss(uint64_t pss)
{
    pss_ = pss;
    return *this;
}

FreezeJsonMemory::Builder& FreezeJsonMemory::Builder::InitSysFreeMem(uint64_t sysFreeMem)
{
    sysFreeMem_ = sysFreeMem;
    return *this;
}

FreezeJsonMemory::Builder& FreezeJsonMemory::Builder::InitSysAvailMem(uint64_t sysAvailMem)
{
    sysAvailMem_ = sysAvailMem;
    return *this;
}

FreezeJsonMemory::Builder& FreezeJsonMemory::Builder::InitSysTotalMem(uint64_t sysTotalMem)
{
    sysTotalMem_ = sysTotalMem;
    return *this;
}

FreezeJsonMemory FreezeJsonMemory::Builder::Build() const
{
    FreezeJsonMemory freezeJsonMemory = FreezeJsonMemory(*this);
    return freezeJsonMemory;
}

std::string FreezeJsonMemory::JsonStr() const
{
    std::map<std::string, uint64_t> memoryMap;
    memoryMap[jsonMemoryRss] = rss_;
    memoryMap[jsonMemoryVss] = vss_;
    memoryMap[jsonMemoryPss] = pss_;
    memoryMap[jsonMemorySysFreeMem] = sysFreeMem_;
    memoryMap[jsonMemorySysAvailMem] = sysAvailMem_;
    memoryMap[jsonMemorySysTotalMem] = sysTotalMem_;
    return FreezeJsonUtil::GetStrByMap(memoryMap);
}

FreezeJsonParams::FreezeJsonParams(const FreezeJsonParams::Builder& builder)
    : time_(builder.time_),
    uuid_(builder.uuid_),
    freezeType_(builder.freezeType_),
    foreground_(builder.foreground_),
    bundleVersion_(builder.bundleVersion_),
    bundleName_(builder.bundleName_),
    processName_(builder.processName_),
    externalLog_(builder.externalLog_),
    pid_(builder.pid_),
    uid_(builder.uid_),
    appRunningUniqueId_(builder.appRunningUniqueId_),
    exception_(builder.exception_),
    hilog_(builder.hilog_),
    eventHandler_(builder.eventHandler_),
    eventHandlerSize3s_(builder.eventHandlerSize3s_),
    eventHandlerSize6s_(builder.eventHandlerSize6s_),
    peerBinder_(builder.peerBinder_),
    threads_(builder.threads_),
    memory_(builder.memory_)
{
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitTime(unsigned long long time)
{
    time_ = time;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitUuid(const std::string& uuid)
{
    uuid_ = uuid;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitFreezeType(const std::string& freezeType)
{
    freezeType_ = freezeType;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitForeground(bool foreground)
{
    foreground_ = foreground;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitBundleVersion(const std::string& bundleVersion)
{
    bundleVersion_ = bundleVersion;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitBundleName(const std::string& bundleName)
{
    bundleName_ = bundleName;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitProcessName(const std::string& processName)
{
    processName_ = processName;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitExternalLog(const std::string& externalLog)
{
    externalLog_ = externalLog;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitPid(long pid)
{
    pid_ = pid;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitUid(long uid)
{
    uid_ = uid;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitAppRunningUniqueId(const std::string& appRunningUniqueId)
{
    appRunningUniqueId_ = appRunningUniqueId;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitException(const std::string& exception)
{
    exception_ = exception;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitHilog(const std::string& hilog)
{
    hilog_ = hilog;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitEventHandler(const std::string& eventHandler)
{
    eventHandler_ = eventHandler;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitEventHandlerSize3s(const std::string& eventHandlerSize3s)
{
    eventHandlerSize3s_ = eventHandlerSize3s;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitEventHandlerSize6s(const std::string& eventHandlerSize6s)
{
    eventHandlerSize6s_ = eventHandlerSize6s;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitPeerBinder(const std::string& peerBinder)
{
    peerBinder_ = peerBinder;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitThreads(const std::string& threads)
{
    threads_ = threads;
    return *this;
}

FreezeJsonParams::Builder& FreezeJsonParams::Builder::InitMemory(const std::string& memory)
{
    memory_ = memory;
    return *this;
}

FreezeJsonParams FreezeJsonParams::Builder::Build() const
{
    FreezeJsonParams freezeJsonParams = FreezeJsonParams(*this);
    return freezeJsonParams;
}

std::string FreezeJsonParams::JsonStr() const
{
    std::list<std::string> list;
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsTime, time_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsUuid, uuid_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsForeground, foreground_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsBundleVersion, bundleVersion_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsBundleName, bundleName_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsProcessName, processName_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsExternalLog, externalLog_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsPid, pid_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsUid, uid_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsAppRunningUniqueId, appRunningUniqueId_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsException, exception_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsHilog, hilog_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsEventHandler, eventHandler_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsEventHandlerSize3s, eventHandlerSize3s_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsEventHandlerSize6s, eventHandlerSize6s_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsPeerBinder, peerBinder_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsThreads, threads_));
    list.push_back(FreezeJsonUtil::GetStrByKeyValue(jsonParamsMemory, memory_));
    return FreezeJsonUtil::MergeKeyValueList(list);
}

} // namespace HiviewDFX
} // namespace OHOS