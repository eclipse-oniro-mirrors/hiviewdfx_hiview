/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * Author: zhengkaijie
 * Create: 2024-7-18
 */

#ifndef DFR_FAULT_DUMP_STATE_BASE_H
#define DFR_FAULT_DUMP_STATE_BASE_H

#include "fault_state_base.h"

#include <vector>
#include <map>
#include <sys/wait.h>
#include "fault_common.h"
#include "fault_info_base.h"
#include "fault_detector_base.h"
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {

class FaultDumpStateBase : public FaultStateBase {
public:
    ~FaultDumpStateBase() override {};
    void DoDumpLog(DumperInfo dumperInfo);

private:
    void DoPerDumpLog(DumperInfo &dumperInfo);
    void DoAppPerDumpLog(DumperInfo &dumperInfo);
    void DoServicePerDumpLog(DumperInfo &dumperInfo);
    std::string GetProcFilePath(CommonDumpType dumpType);
    void DumpSystemHeapInfo(DumperInfo &dumperInfo) const;
    void DumpIonNewProcInfo(DumperInfo &dumperInfo) const;
    void DumpSysviewProc(DumperInfo &dumperInfo) const;
    void DumpProcInfo(DumperInfo &dumperInfo);
    bool ForkProcessToDumpInfo(DumperInfo &dumperInfo);
    bool GetWriteFout(DumperInfo &dumperInfo, std::ofstream &fout) const;
    bool GetWriteFd(DumperInfo &dumperInfo, int &writeFd) const;
    void DumpMmDmabuf(DumperInfo &dumperInfo) const;

private:
    ffrt::mutex logDumpMtx_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FAULT_DUMP_STATE_BASE_H
