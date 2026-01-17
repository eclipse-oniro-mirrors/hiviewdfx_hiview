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
#include <array>
#include <gtest/gtest.h>

#include "faultlog_event_factory.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
/**
 * @tc.name: EventHandlerStrategyFactory001
 * @tc.desc: Test CreateFaultLogEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogEventFactoryTest, EventHandlerStrategyFactory001, testing::ext::TestSize.Level3)
{
    std::array<FaultLogType, 10> typeList1 = {
        FaultLogType::JS_CRASH,
        FaultLogType::CJ_ERROR,
        FaultLogType::RUST_PANIC,
        FaultLogType::ADDR_SANITIZER,
        FaultLogType::CPP_CRASH,
        FaultLogType::PROCESS_PAGE_INFO,
        FaultLogType::APP_FREEZE,
        FaultLogType::APPFREEZE_WARNING,
        FaultLogType::SYS_FREEZE,
        FaultLogType::SYS_WARNING,
    };
    for (auto type : typeList1) {
        auto fault = FaultLogEventFactory::CreateFaultLogEvent(type);
        ASSERT_NE(fault, nullptr) << "CreateFaultLogEvent(" << type << ")";
    }
    std::array<FaultLogType, 2> typeList2 = {
        FaultLogType::ALL,
        FaultLogType::MAX_TYPE,
    };
    for (auto type : typeList2) {
        auto fault = FaultLogEventFactory::CreateFaultLogEvent(type);
        ASSERT_EQ(fault, nullptr) << "CreateFaultLogEvent(" << type << ")";
    }
}


} // namespace HiviewDFX
} // namespace OHOS
