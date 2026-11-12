/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DFR_KERNEL_THREAD_KILL_EVENT_HANDLER_H
#define DFR_KERNEL_THREAD_KILL_EVENT_HANDLER_H

#include "process_kill_event_handler.h"
namespace OHOS {
namespace HiviewDFX {

class KernelThreadKillLeakInfo : public ProcessKillLeakInfo {
public:
    explicit KernelThreadKillLeakInfo(ProcesssKillEventParam &eventParam,
        int64_t threadNum, int64_t foreGround, int64_t time, std::string &msg);

    std::string GetFileName() override;
    std::string GetFilePath() override;
    bool IsDumpMessage() override;
    void SysEventReport() override;
    void AppEventReport() override;
    void SubSysEventReport() override;
    void HandleSubeventInfo(const std::string &data) override;
    void DumpGeneralInfo(std::ofstream &fout) override;

protected:
    int threadNum_;
};
} // HiviewDFX
} // OHOS
#endif // DFR_DMA_KILL_EVENT_HANDLER_H
