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

// This file implements the DynamicTelemetryProxy that lazily dlopen's
// libleakdetectors_ext.z.so on first use, and dlclose's it when telemetry
// is turned off. When the .so is not present (blue zone), all calls degrade
// to the no-op DefaultTelemetry.

#include "i_telemetry.h"
#include "telemetry_common.h"
#include "telemetry_manager.h" // for EventQuota complete definition

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <dlfcn.h>
#include <mutex>

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002DA0, "TelemetryProxy");

namespace {
constexpr const char* const TELEMETRY_EXT_SO = "libleakdetectors_ext.z.so";

// Blue-zone no-op implementation (also used as fallback when dlopen fails).
class DefaultTelemetry : public ITelemetry {
public:
    bool CouldTelemetry(uint64_t, const std::string &) override { return false; }
    bool InTelemetry(uint64_t, const std::string &) override { return false; }
    struct EventQuota GetEventQuota(uint64_t, const std::string &) override { return {}; }
    void ConsumeQuota(uint64_t, const std::string &) override {}
    bool CouldCompress() override { return false; }
    void ConsumeCompressQuota() override {}
    void SetCompressListSize(size_t) override {}
    std::vector<TelemetryLeakInfo> GetTelemetryInfos(uint64_t) override { return {}; }
    std::string GetRandomFilterList(uint64_t) override { return ""; }
    void OnUnorderedEvent(const TelemetryEvent &) override {}
    void Report(HiSysEvent::EventType, const EventQuota &, int32_t, const std::string & = "") override {}
};

DefaultTelemetry& GetDefaultTelemetry()
{
    static DefaultTelemetry instance;
    return instance;
}
} // namespace

// DynamicTelemetryProxy: lazily loads libleakdetectors_ext.z.so, forwards
// ITelemetry calls to the real implementation, and unloads the .so when
// telemetry is turned off.
//
// Runtime gating: the .so is only loaded after a telemetry event with
// telemetryStatus="on" has been received. When "off" is received, the .so
// is immediately dlclose'd (after waiting for in-flight calls to drain).
// Until then (or after "off"), all calls short-circuit to DefaultTelemetry
// no-ops without dlopen.
//
// Thread safety: an active-call count guards dlclose. Each method acquires
// the count before calling into impl_ and releases it after. Unload waits
// for the count to reach zero before calling dlclose, so no thread is ever
// executing code inside the .so when it is unloaded.
class DynamicTelemetryProxy : public ITelemetry {
public:
    DynamicTelemetryProxy() = default;
    ~DynamicTelemetryProxy() override;

    bool CouldTelemetry(uint64_t faultType, const std::string &bundleName) override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            return GetDefaultTelemetry().CouldTelemetry(faultType, bundleName);
        }
        return guard.impl->CouldTelemetry(faultType, bundleName);
    }

    bool InTelemetry(uint64_t faultType, const std::string &bundleName) override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            return GetDefaultTelemetry().InTelemetry(faultType, bundleName);
        }
        return guard.impl->InTelemetry(faultType, bundleName);
    }

    struct EventQuota GetEventQuota(uint64_t faultType, const std::string &bundleName) override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            return GetDefaultTelemetry().GetEventQuota(faultType, bundleName);
        }
        return guard.impl->GetEventQuota(faultType, bundleName);
    }

    void ConsumeQuota(uint64_t faultType, const std::string &bundleName) override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            GetDefaultTelemetry().ConsumeQuota(faultType, bundleName);
            return;
        }
        guard.impl->ConsumeQuota(faultType, bundleName);
    }

    bool CouldCompress() override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            return GetDefaultTelemetry().CouldCompress();
        }
        return guard.impl->CouldCompress();
    }

    void ConsumeCompressQuota() override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            GetDefaultTelemetry().ConsumeCompressQuota();
            return;
        }
        guard.impl->ConsumeCompressQuota();
    }

    void SetCompressListSize(size_t size) override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            GetDefaultTelemetry().SetCompressListSize(size);
            return;
        }
        guard.impl->SetCompressListSize(size);
    }

    std::vector<TelemetryLeakInfo> GetTelemetryInfos(uint64_t faultType) override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            return GetDefaultTelemetry().GetTelemetryInfos(faultType);
        }
        return guard.impl->GetTelemetryInfos(faultType);
    }

    std::string GetRandomFilterList(uint64_t faultType) override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            return GetDefaultTelemetry().GetRandomFilterList(faultType);
        }
        return guard.impl->GetRandomFilterList(faultType);
    }

    void OnUnorderedEvent(const TelemetryEvent &msg) override
    {
        auto pairs = msg.GetKeyValuePairs();
        auto it = pairs.find("telemetryStatus");
        if (it != pairs.end()) {
            if (it->second == "on") {
                telemetryEnabled_ = true;
                HIVIEW_LOGI("telemetry enabled, will load %{public}s on next call", TELEMETRY_EXT_SO);
            } else if (it->second == "off") {
                telemetryEnabled_ = false;
                HIVIEW_LOGI("telemetry disabled, unloading %{public}s", TELEMETRY_EXT_SO);
                UnloadSo();
                GetDefaultTelemetry().OnUnorderedEvent(msg);
                return;
            }
        }
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            GetDefaultTelemetry().OnUnorderedEvent(msg);
            return;
        }
        guard.impl->OnUnorderedEvent(msg);
    }

    void Report(HiSysEvent::EventType eventType, const EventQuota &eq, int32_t errCode,
                const std::string &extMsg = "") override
    {
        auto guard = AcquireImpl();
        if (guard.impl == nullptr) {
            GetDefaultTelemetry().Report(eventType, eq, errCode, extMsg);
            return;
        }
        guard.impl->Report(eventType, eq, errCode, extMsg);
    }

private:
    // RAII guard: increments activeCalls_ on acquire, decrements on release.
    // While at least one guard is alive, UnloadSo() will block waiting for
    // the count to reach zero, ensuring no thread is inside the .so when
    // dlclose is called.
    struct ImplGuard {
        ITelemetry* impl = nullptr;
        DynamicTelemetryProxy* proxy = nullptr;
        ImplGuard() = default;
        ImplGuard(ITelemetry* i, DynamicTelemetryProxy* p) : impl(i), proxy(p) {}
        ImplGuard(const ImplGuard&) = delete;
        ImplGuard& operator=(const ImplGuard&) = delete;
        ImplGuard(ImplGuard&& other) noexcept : impl(other.impl), proxy(other.proxy)
        {
            other.impl = nullptr;
            other.proxy = nullptr;
        }
        ImplGuard& operator=(ImplGuard&& other) noexcept
        {
            if (this != &other) {
                Release();
                impl = other.impl;
                proxy = other.proxy;
                other.impl = nullptr;
                other.proxy = nullptr;
            }
            return *this;
        }
        ~ImplGuard() { Release(); }
        void Release()
        {
            if (proxy != nullptr) {
                proxy->activeCalls_.fetch_sub(1, std::memory_order_release);
                proxy->cv_.notify_one();
                proxy = nullptr;
            }
        }
    };

    ImplGuard AcquireImpl();
    void UnloadSo();

    std::mutex mtx_;
    std::condition_variable cv_;
    void* soHandle_ = nullptr;
    ITelemetry* impl_ = nullptr;
    std::atomic<int> activeCalls_{0}; // number of in-flight calls into impl_
    bool loadFailed_ = false;
    std::atomic<bool> telemetryEnabled_ { false };
};

DynamicTelemetryProxy::~DynamicTelemetryProxy()
{
    UnloadSo();
}

DynamicTelemetryProxy::ImplGuard DynamicTelemetryProxy::AcquireImpl()
{
#ifndef DUBAI_CONFIG_FEATURE_MEM
    return {};
#else
    if (!telemetryEnabled_.load(std::memory_order_relaxed)) {
        return {};
    }
    std::lock_guard<std::mutex> lock(mtx_);
    if (impl_ != nullptr) {
        activeCalls_.fetch_add(1, std::memory_order_acquire);
        return ImplGuard(impl_, this);
    }
    if (loadFailed_) {
        return {};
    }
    soHandle_ = dlopen(TELEMETRY_EXT_SO, RTLD_LAZY);
    if (soHandle_ == nullptr) {
        HIVIEW_LOGI("dlopen %{public}s failed: %{public}s, using no-op fallback", TELEMETRY_EXT_SO,
            dlerror());
        loadFailed_ = true;
        return {};
    }
    auto factory = reinterpret_cast<NewTelemetryInterfaceFunc>(
        dlsym(soHandle_, NEW_TELEMETRY_INTERFACE_SYMBOL));
    if (factory == nullptr) {
        HIVIEW_LOGE("dlsym %{public}s failed: %{public}s", NEW_TELEMETRY_INTERFACE_SYMBOL, dlerror());
        dlclose(soHandle_);
        soHandle_ = nullptr;
        loadFailed_ = true;
        return {};
    }
    impl_ = factory();
    if (impl_ == nullptr) {
        HIVIEW_LOGE("NewTelemetryInterface returned null");
        dlclose(soHandle_);
        soHandle_ = nullptr;
        loadFailed_ = true;
        return {};
    }
    HIVIEW_LOGI("dlopen %{public}s success, telemetry loaded", TELEMETRY_EXT_SO);
    activeCalls_.fetch_add(1, std::memory_order_acquire);
    return ImplGuard(impl_, this);
#endif
}

void DynamicTelemetryProxy::UnloadSo()
{
    std::unique_lock<std::mutex> lock(mtx_);
    // Wait for all in-flight calls to drain before dlclose.
    cv_.wait(lock, [this]() {
        return activeCalls_.load(std::memory_order_acquire) == 0;
    });
    if (soHandle_ != nullptr) {
        HIVIEW_LOGI("dlclose %{public}s", TELEMETRY_EXT_SO);
        dlclose(soHandle_);
        soHandle_ = nullptr;
        impl_ = nullptr;
        loadFailed_ = false;
    }
}

ITelemetry& GetTelemetry()
{
    static DynamicTelemetryProxy instance;
    return instance;
}

} // namespace HiviewDFX
} // namespace OHOS
