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
#ifndef DFR_TELEMETRY_MANAGER_H
#define DFR_TELEMETRY_MANAGER_H

#include "singleton.h"
#include "i_telemetry.h"
#ifdef TELEMETRY_STANDALONE
#include "telemetry_common.h"
#include "telemetry_dfr_util.h"
#include "telemetry_event.h"
#else
#include "dfr_util.h"
#include "fault_common.h"
#endif
#include <deque>
#include <vector>
#include <string>
#include <map>
#include <atomic>
#include <ffrt.h>

namespace OHOS {
namespace HiviewDFX {
constexpr int DEFAULT_ROM_RSV_SIZE = 30720; // 30G
constexpr int MIN_ROM_RSV_SIZE = 1024; // 1G
constexpr int MAX_ROM_RSV_SIZE = 30720; // 30G
constexpr size_t MAX_QUOTA_SIZE = 10;

enum class TelemetryType : int {
    SYSTEM_TELEMETRY = 1001, // other scene
    RANDOM_TELEMETRY = 1002, // bundleName is empty and filterList not empty
};

struct EventQuota {
    time_t deliveryTs { 0 };
    std::string bundleName { "" };
    uint64_t faultType { 0 };
    std::string telemetryId { "" };
    int quota { 0 };
    int romRsvSize { MAX_ROM_RSV_SIZE };
    int ts3 { 0 };
    int nmdInterval { 0 };
    int profilerTime { 0 };
    uint64_t subLeakType { 0 };
    int subLeakCaptureThreshold { 0 };
    int ts1 { 0 };
    int ts2 { 0 };
    std::string targetSoNames { "" };
    int leakThreshold { 0 };
    // 1001: system telemetry (default), 1002: random telemetry
    int type { static_cast<int>(TelemetryType::SYSTEM_TELEMETRY) };
    // filterList for random telemetry, comma-separated bundle names
    std::string filterList;
};

struct GrayQuota {
    time_t screenOffCompressTs { 0 };
    std::deque<time_t> screenOffChargeCompressTs; // size: QUOTA_SCREEN_OFF_AND_CHARGE
    std::vector<EventQuota> quotas;
};

#ifdef DUBAI_CONFIG_FEATURE_MEM
class TelemetryManager : public Singleton<TelemetryManager>, public ITelemetry {
    DECLARE_SINGLETON(TelemetryManager);
public:
    void OnUnorderedEvent(const TelemetryEvent &msg) override;
    struct EventQuota GetEventQuota(uint64_t faultType, const std::string &bundleName) override;
    void ConsumeQuota(uint64_t faultType, const std::string &bundleName) override;
    bool CouldTelemetry(uint64_t faultType, const std::string &bundleName) override;
    bool InTelemetry(uint64_t faultType, const std::string &bundleName) override;
    void WriteQuotaFile(const std::string &path = TELEMETRY_QUOTA_PATH);
    void WriteQuotaFileUnlocked(const std::string &path = TELEMETRY_QUOTA_PATH);
    bool CouldCompress() override;
    void ConsumeCompressQuota() override;
    void SetCompressListSize(size_t size) override;
    std::vector<TelemetryLeakInfo> GetTelemetryInfos(uint64_t faultType) override;
    std::string GetRandomFilterList(uint64_t faultType) override;
    void Report(HiSysEvent::EventType eventType, const EventQuota &eq, int32_t errCode,
                const std::string &extMsg = "") override;

    // public for UT access
    bool IsEventVaild(std::map<std::string, std::string> &valuePairs);
    void HandleTelemetryStart(TelemetryEvent msg);
    void HandleTelemetryEnd(TelemetryEvent msg);
    void AddEventQuota(struct EventQuota eq);
    bool ReadData(std::ifstream &inFile, std::string &rtn);
    bool ReadData(std::ifstream &inFile, uint64_t &rtn);
    bool ReadData(std::ifstream &inFile, time_t &rtn);
    bool ReadData(std::ifstream &inFile, int &rtn);
#else
class TelemetryManager : public Singleton<TelemetryManager> {
    friend Singleton<TelemetryManager>;
public:
    TelemetryManager() = default;
    ~TelemetryManager() = default;
    static void OnUnorderedEvent(const TelemetryEvent &msg) {}
    static bool CouldTelemetry(uint64_t faultType, const std::string &bundleName) { return false; }
    static bool InTelemetry(uint64_t faultType, const std::string &bundleName) { return false; }
    static struct EventQuota GetEventQuota(uint64_t faultType, const std::string &bundleName) { return {}; }
    static void ConsumeQuota(uint64_t faultType, const std::string &bundleName) {}
    static void WriteQuotaFile(const std::string &path = TELEMETRY_QUOTA_PATH) {}
    static void WriteQuotaFileUnlocked(const std::string &path = TELEMETRY_QUOTA_PATH) {}
    static bool CouldCompress() { return false; }
    static void ConsumeCompressQuota() {}
    static void SetCompressListSize(size_t size) {}
    static std::vector<TelemetryLeakInfo> GetTelemetryInfos(uint64_t faultType) { return {}; }
    static std::string GetRandomFilterList(uint64_t faultType) { return ""; }
    static void Report(HiSysEvent::EventType eventType, const EventQuota &eq, int32_t errCode,
                const std::string &extMsg = "") {}
#endif

private:
    void InitQuota();
    void DoInitQuota();
    void HandleTelemetryStartImpl(std::map<std::string, std::string> &valuePairs, TelemetryEvent &msg);
    void BuildEventQuota(std::map<std::string, std::string> &valuePairs, struct EventQuota &eq);
    bool ValidateTelemetryType(const struct EventQuota &eq);
    bool IsBundleNameValid(const std::string &bundleName);
    void DispatchQuotaByFaultType(uint64_t fault, const struct EventQuota &eq, TelemetryEvent &msg,
        std::map<std::string, std::string> &valuePairs);
    void AddExtraMemEventQuota(EventQuota &eq, TelemetryEvent &msg);
    void AddMemEventQuota(EventQuota &eq, uint64_t leakType, TelemetryEvent &msg);
    void AddXssEventQuota(EventQuota &eq, uint64_t leakType, TelemetryEvent &msg);
    void AddLiteProfilersEventQuota(EventQuota &eq, uint64_t leakType, TelemetryEvent &msg);
    void DeteleEventQuota(const std::string &telemetryId);
    void WriteSandboxPropertyUnlocked(const struct EventQuota &eq);
    void WriteSystemPropertyUnlocked(const struct EventQuota &eq);
    void ClearSystemProperty();
    void ClearFilterListSandboxUnlocked();
    void ClearFilterListSandboxByFilterList(const std::string &filterList);
    void RemoveQuota(uint64_t faultType, const std::string &bundleName);
    bool ReadQuotaFile();
    bool ReadQuotaItemAndRestoreProperty(std::ifstream &fin, size_t index, bool &systemPropertyWritten);
    bool ReadEventQuotaItem(std::ifstream &fin, EventQuota &eq);
    void WriteData(std::ofstream &outFile, const time_t data);
    void WriteData(std::ofstream &outFile, const uint64_t data);
    void WriteData(std::ofstream &outFile, const int data);
    void WriteData(std::ofstream &outFile, const std::string &data);
    void WriteQuotaItemUnlocked(std::ofstream &fout, size_t index);
    bool HasCompressQuota();
    bool HasBundleQuota(uint64_t faultType, const std::string &bundleName);
    bool SetDirXattr(const std::string &dir, const std::string &value);
    bool SetDirXattr(const std::string &dir, const std::string &property, const std::string &value);
    void ClearSandboxProperty(std::map<std::string, std::string> &valuePairs);
    void ClearSandboxContentUnlocked();
    void ClearRandomTelemetryDataUnlocked();
    void WriteFilterListSandboxPropertyUnlocked(std::map<std::string, std::string> &valuePairs);
    bool HasQuotaWhenPowerConnect();
    bool HasQuotaWhenPowerDisconnect();
    void ConsumeQuotaPowerConnect();
    void ConsumeQuotaPowerDisConnect();
    size_t GetPowerConnectQuota();
    bool HaveEnoughRom(uint64_t faultType, const std::string &bundleName);
    int GetRomSizeValue(const std::string &romSizeStr);
    int GetIntEventValue(std::map<std::string, std::string> &valuePairs, const std::string &keyStr);

private:
    struct GrayQuota grayQuota_;
    ffrt::mutex quotaMtx_;
    std::atomic<size_t> compressListSize_ { 0 };
};
}
}
#endif
