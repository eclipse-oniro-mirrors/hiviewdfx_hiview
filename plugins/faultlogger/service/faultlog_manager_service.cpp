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

#include "faultlog_manager_service.h"

#include "common_utils.h"
#include "hiview_logger.h"
#include "faultlog_bundle_util.h"
#include "faultlog_dump.h"
#include "faultlog_processor_factory.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
namespace {
constexpr int32_t MAX_QUERY_NUM = 100;
constexpr int MIN_APP_UID = 10000;
} // namespace

void FaultLogManagerService::Dump(int fd, const std::vector<std::string>& cmds)
{
    if (!faultLogManager_) {
        return;
    }
    FaultLogDump faultLogDump(fd, faultLogManager_);
    faultLogDump.DumpByCommands(cmds);
}

void FaultLogManagerService::AddFaultLog(FaultLogInfo& info)
{
    if (!faultLogManager_) {
        return;
    }
    FaultLogProcessorFactory factory;
    auto processor = factory.CreateFaultLogProcessor(static_cast<FaultLogType>(info.faultLogType));
    if (processor) {
        processor->AddFaultLog(info, workLoop_, faultLogManager_);
    }
}

std::unique_ptr<FaultLogQueryResultInner> FaultLogManagerService::QuerySelfFaultLog(int32_t id,
    int32_t pid, int32_t faultType, int32_t maxNum)
{
    if (!faultLogManager_) {
        return nullptr;
    }
    if ((faultType < FaultLogType::ALL) || (faultType > FaultLogType::APP_FREEZE)) {
        HIVIEW_LOGW("Unsupported fault type");
        return nullptr;
    }

    if (maxNum < 0 || maxNum > MAX_QUERY_NUM) {
        maxNum = MAX_QUERY_NUM;
    }

    std::string name;
    if (id >= MIN_APP_UID) {
        name = GetApplicationNameById(id);
    }

    if (name.empty()) {
        name = CommonUtils::GetProcNameByPid(pid);
    }
    return std::make_unique<FaultLogQueryResultInner>(faultLogManager_->GetFaultInfoList(name, id, faultType, maxNum));
}
}  // namespace HiviewDFX
}  // namespace OHOS
