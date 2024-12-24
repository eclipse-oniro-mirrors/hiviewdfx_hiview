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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_IO_COLLECTOR_IMPL_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_IO_COLLECTOR_IMPL_H

#include <mutex>

#include "io_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class IoCollectorImpl : public IoCollector {
public:
    IoCollectorImpl();
    virtual ~IoCollectorImpl() = default;

public:
    virtual CollectResult<ProcessIo> CollectProcessIo(int32_t pid) override;
    virtual CollectResult<std::string> CollectRawDiskStats() override;
    virtual CollectResult<std::vector<DiskStats>> CollectDiskStats(
        DiskStatsFilter filter = DefaultDiskStatsFilter, bool isUpdate = false) override;
    virtual CollectResult<std::string> ExportDiskStats(DiskStatsFilter filter = DefaultDiskStatsFilter) override;
    virtual CollectResult<std::vector<EMMCInfo>> CollectEMMCInfo() override;
    virtual CollectResult<std::string> ExportEMMCInfo() override;
    virtual CollectResult<std::vector<ProcessIoStats>> CollectAllProcIoStats(bool isUpdate = false) override;
    virtual CollectResult<std::string> ExportAllProcIoStats() override;
    virtual CollectResult<SysIoStats> CollectSysIoStats() override;
    virtual CollectResult<std::string> ExportSysIoStats() override;

private:
    void InitDiskData();
    void GetDiskStats(DiskStatsFilter filter, bool isUpdate, std::vector<DiskStats>& diskStats);
    void CalculateDiskStats(uint64_t period, bool isUpdate);
    void CalculateDeviceDiskStats(const DiskData& currData, const std::string& deviceName, uint64_t period);
    void CalculateEMMCInfo(std::vector<EMMCInfo>& mmcInfos);
    void ReadEMMCInfo(const std::string& path, std::vector<EMMCInfo>& mmcInfos);
    std::string GetEMMCPath(const std::string& path);
    void InitProcIoData();
    void GetProcIoStats(std::vector<ProcessIoStats>& allProcIoStats, bool isUpdate);
    void CalculateAllProcIoStats(uint64_t period, bool isUpdate);
    void CalculateProcIoStats(const ProcessIo& currData, int32_t pid, uint64_t period);
    bool ProcIoStatsFilter(const ProcessIoStats& stats);
    int32_t GetProcStateInCollectionPeriod(int32_t pid);
    std::string CreateExportFileName(const std::string& filePrefix);

private:
    std::mutex collectDiskMutex_;
    std::mutex collectProcIoMutex_;
    std::mutex exportFileMutex_;
    uint64_t preCollectDiskTime_ = 0;
    uint64_t preCollectProcIoTime_ = 0;
    uint64_t currCollectProcIoTime_ = 0;
    std::unordered_map<std::string, DiskStatsDevice> diskStatsMap_;
    std::unordered_map<int32_t, ProcessIoStatsInfo> procIoStatsMap_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_IO_COLLECTOR_IMPL_H