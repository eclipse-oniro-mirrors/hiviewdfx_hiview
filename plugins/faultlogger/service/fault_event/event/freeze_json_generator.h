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

#ifndef FREEZE_JSON_GENERATOR_H
#define FREEZE_JSON_GENERATOR_H

#include <string>

namespace OHOS {
namespace HiviewDFX {

class FreezeJsonException {
public:
    class Builder {
    public:
        Builder() {};
        ~Builder() {};
        Builder& InitName(const std::string& name);
        Builder& InitMessage(const std::string& message);
        FreezeJsonException Build() const;

    private:
        std::string name_ = "";
        std::string message_ = "";
        friend class FreezeJsonException;
    };

    explicit FreezeJsonException(const FreezeJsonException::Builder& builder);
    ~FreezeJsonException() {};
    std::string JsonStr() const;

private:
    static constexpr const char* const jsonExceptionName = "name";
    static constexpr const char* const jsonExceptionMessage = "message";

    std::string name_;
    std::string message_;
};

class FreezeJsonMemory {
public:
    class Builder {
    public:
        Builder() {};
        ~Builder() {};
        Builder& InitRss(uint64_t rss);
        Builder& InitVss(uint64_t vss);
        Builder& InitPss(uint64_t pss);
        Builder& InitSysFreeMem(uint64_t sysFreeMem);
        Builder& InitSysAvailMem(uint64_t sysAvailMem);
        Builder& InitSysTotalMem(uint64_t sysTotalMem);
        Builder& InitHeapTotalSize(uint64_t heapTotalSize);
        Builder& InitHeapObjectSize(uint64_t heapObjectSize);
        FreezeJsonMemory Build() const;

    private:
        uint64_t rss_ = 0;
        uint64_t vss_ = 0;
        uint64_t pss_ = 0;
        uint64_t sysFreeMem_ = 0;
        uint64_t sysAvailMem_ = 0;
        uint64_t sysTotalMem_ = 0;
        uint64_t heapTotalSize_ = 0;
        uint64_t heapObjectSize_ = 0;
        friend class FreezeJsonMemory;
    };

    explicit FreezeJsonMemory(const FreezeJsonMemory::Builder& builder);
    ~FreezeJsonMemory() {};
    std::string JsonStr() const;

private:
    static constexpr const char* const jsonMemoryRss = "rss";
    static constexpr const char* const jsonMemoryVss = "vss";
    static constexpr const char* const jsonMemoryPss = "pss";
    static constexpr const char* const jsonMemorySysFreeMem = "sys_free_mem";
    static constexpr const char* const jsonMemorySysAvailMem = "sys_avail_mem";
    static constexpr const char* const jsonMemorySysTotalMem = "sys_total_mem";
    static constexpr const char* const jsonMemoryHeapTotalSize = "vm_heap_total_size";
    static constexpr const char* const jsonMemoryHeapObjcetSize = "vm_object_used_size";
    uint64_t rss_;
    uint64_t vss_;
    uint64_t pss_;
    uint64_t sysFreeMem_;
    uint64_t sysAvailMem_;
    uint64_t sysTotalMem_;
    uint64_t heapTotalSize_;
    uint64_t heapObjectSize_;
};

class FreezeJsonParams {
public:
    class Builder {
    public:
        Builder() {};
        ~Builder() {};
        Builder& InitTime(unsigned long long time);
        Builder& InitUuid(const std::string& uuid);
        Builder& InitFreezeType(const std::string& freezeType);
        Builder& InitForeground(bool foreground);
        Builder& InitBundleVersion(const std::string& bundleVersion);
        Builder& InitBundleName(const std::string& bundleName);
        Builder& InitProcessName(const std::string& processName);
        Builder& InitProcessLifeTime(const uint64_t& processLifeTime);
        Builder& InitExternalLog(const std::string& externalLog);
        Builder& InitPid(long pid);
        Builder& InitUid(long uid);
        Builder& InitAppRunningUniqueId(const std::string& appRunningUniqueId);
        Builder& InitException(const std::string& exception);
        Builder& InitHilog(const std::string& hilog);
        Builder& InitEventHandler(const std::string& eventHandler);
        Builder& InitEventHandlerSize3s(const std::string& eventHandlerSize3s);
        Builder& InitEventHandlerSize6s(const std::string& eventHandlerSize6s);
        Builder& InitPeerBinder(const std::string& peerBinder);
        Builder& InitThreads(const std::string& threads);
        Builder& InitMemory(const std::string& memory);
        FreezeJsonParams Build() const;

    private:
        unsigned long long time_ = 0;
        std::string uuid_ = "";
        std::string freezeType_ = "";
        bool foreground_ = false;
        std::string bundleVersion_ = "";
        std::string bundleName_ = "";
        std::string processName_ = "";
        uint64_t processLifeTime_ = 0;
        std::string externalLog_ = "[]";
        long pid_ = 0;
        long uid_ = 0;
        std::string appRunningUniqueId_ = "";
        std::string exception_ = "{}";
        std::string hilog_ = "[]";
        std::string eventHandler_ = "[]";
        std::string eventHandlerSize3s_ = "";
        std::string eventHandlerSize6s_ = "";
        std::string peerBinder_ = "[]";
        std::string threads_ = "[]";
        std::string memory_ = "{}";
        friend class FreezeJsonParams;
    };

    explicit FreezeJsonParams(const FreezeJsonParams::Builder& builder);
    ~FreezeJsonParams() {};
    std::string JsonStr() const;

private:
    static constexpr const char* const jsonParamsTime = "time";
    static constexpr const char* const jsonParamsUuid = "uuid";
    static constexpr const char* const jsonParamsFreezeType = "freeze_type";
    static constexpr const char* const jsonParamsForeground = "foreground";
    static constexpr const char* const jsonParamsBundleVersion = "bundle_version";
    static constexpr const char* const jsonParamsBundleName = "bundle_name";
    static constexpr const char* const jsonParamsProcessName = "process_name";
    static constexpr const char* const jsonParamsProcessLifeTime = "process_life_time";
    static constexpr const char* const jsonParamsExternalLog = "external_log";
    static constexpr const char* const jsonParamsPid = "pid";
    static constexpr const char* const jsonParamsUid = "uid";
    static constexpr const char* const jsonParamsAppRunningUniqueId = "app_running_unique_id";
    static constexpr const char* const jsonParamsException = "exception";
    static constexpr const char* const jsonParamsHilog = "hilog";
    static constexpr const char* const jsonParamsEventHandler = "event_handler";
    static constexpr const char* const jsonParamsEventHandlerSize3s = "event_handler_size_3s";
    static constexpr const char* const jsonParamsEventHandlerSize6s = "event_handler_size_6s";
    static constexpr const char* const jsonParamsPeerBinder = "peer_binder";
    static constexpr const char* const jsonParamsThreads = "threads";
    static constexpr const char* const jsonParamsMemory = "memory";

    unsigned long long time_;
    std::string uuid_;
    std::string freezeType_;
    bool foreground_;
    std::string bundleVersion_;
    std::string bundleName_;
    std::string processName_;
    uint64_t processLifeTime_ = 0;
    std::string externalLog_ = "";
    long pid_ = 0;
    long uid_ = 0;
    std::string appRunningUniqueId_ = "";
    std::string exception_;
    std::string hilog_;
    std::string eventHandler_;
    std::string eventHandlerSize3s_;
    std::string eventHandlerSize6s_;
    std::string peerBinder_;
    std::string threads_;
    std::string memory_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif