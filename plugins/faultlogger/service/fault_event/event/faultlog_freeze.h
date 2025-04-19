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
#ifndef FAULTLOG_FREEZE_H
#define FAULTLOG_FREEZE_H

#include "faultlog_processor_base.h"
#include "freeze_json_util.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogFreeze : public FaultLogProcessorBase {
private:
    void ReportEventToAppEvent(const FaultLogInfo& info) override;
    void AddSpecificInfo(FaultLogInfo& info) override;
    FreezeJsonUtil::FreezeJsonCollector GetFreezeJsonCollector(const FaultLogInfo& info) const;
    std::string GetMemoryStrByPid(long pid) const;
    void ReportAppFreezeToAppEvent(const FaultLogInfo& info, bool isAppHicollie = false) const;

    std::string GetException(const std::string& name, const std::string& message) const;
    std::string GetFreezeHilogByPid(long pid) const;

    void FillProcMemory(long pid, uint64_t& rss,  uint64_t& vss) const;
    void FillSystemMemory(uint64_t& sysFreeMem, uint64_t& sysAvailMem, uint64_t& sysTotalMem) const;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
