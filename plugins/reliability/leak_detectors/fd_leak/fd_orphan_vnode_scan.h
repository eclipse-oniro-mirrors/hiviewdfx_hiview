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

#ifndef FD_ORPHAN_VNODE_SCAN_H
#define FD_ORPHAN_VNODE_SCAN_H

#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "ffrt.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
struct FdOrphanVnodeInfo {
    int32_t pid{ -1 };
    int32_t fdNum{ 0 };
    std::string processName{ "unknown" };
    std::string timeStr;
};

class FdOrphanVnodeScan : public Singleton<FdOrphanVnodeScan> {
    DECLARE_SINGLETON(FdOrphanVnodeScan);

public:
    void ScanOrphanVnodeOnChargeSleep();
    void DumpOrphanVnodeInfo(const std::vector<std::string> &topOrphanVnodeList,
        const FdOrphanVnodeInfo &info);

private:
    void ReportOrphanVnodeLeakEvent(const FdOrphanVnodeInfo &info);
    void ExtractTopProcessInfo(const std::vector<std::string>& topOrphanVnodeList,
        int32_t& pid, std::string& processName, int32_t& fdNum);
    void RecordProcInfo(std::ofstream &fout, const std::string &procFilePath, const std::string &title);

private:
    std::atomic<time_t> lastScanOrphanVnodeTime_ { 0 };
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // FD_ORPHAN_VNODE_SCAN_H