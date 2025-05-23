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
#include "faultlog_processor_factory.h"

#include "faultlog_freeze.h"
#include "faultlog_cppcrash.h"
#include "faultlog_events_processor.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

FaultLogProcessorFactory::FaultLogProcessorFactory()
{
    InitializeProcessors();
}

void FaultLogProcessorFactory::InitializeProcessors()
{
    faultLogProcessors[FaultLogType::CPP_CRASH] = []() {
        return std::make_unique<FaultLogCppCrash>();
    };
    faultLogProcessors[FaultLogType::APP_FREEZE] = []() {
        return std::make_unique<FaultLogFreeze>();
    };
    faultLogProcessors[FaultLogType::SYS_FREEZE] = faultLogProcessors[FaultLogType::APP_FREEZE];
    faultLogProcessors[FaultLogType::SYS_WARNING] = faultLogProcessors[FaultLogType::APP_FREEZE];
}

std::unique_ptr<FaultLogProcessorInterface> FaultLogProcessorFactory::CreateFaultLogProcessor(FaultLogType type)
{
    auto it = faultLogProcessors.find(type);
    if (it != faultLogProcessors.end()) {
        return it->second();
    }
    return nullptr;
}
} // namespace HiviewDFX
} // namespace OHOS
