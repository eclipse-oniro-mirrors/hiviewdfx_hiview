/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef DFR_LITE_PROFILER_H
#define DFR_LITE_PROFILER_H

#include <string>
#include <map>
#include <sys/types.h>
#include "fault_detector_base.h"
#include "dfr_profiler_manager.h"
#include "fault_common.h"
#include "singleton.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

class LiteProfiler : public Singleton<LiteProfiler> {
    DECLARE_SINGLETON(LiteProfiler);

public:
    void NotifyProcessCreate(pid_t pid);
    void NotifyProcessDied(pid_t pid);

private:
    void Report(pid_t pid, pid_t uid, std::string bundleName, std::string telemetryId);
    bool StartDumpLiteProfiler(std::string bundleName, pid_t pid, const TelemetryLeakInfo &telemetryInfo);

private:
    ffrt::mutex allDumpProfilerListMtx_;
    std::map<pid_t, std::shared_ptr<FaultInfoBase>> allDumpProfilerList_;

    ffrt::mutex telemetryInfoMtx_;
    std::map<std::string, TelemetryLeakInfo> telemetryInfoList_;

    std::string leakedTime_;
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // DFR_LITE_PROFILER_LEAK_H