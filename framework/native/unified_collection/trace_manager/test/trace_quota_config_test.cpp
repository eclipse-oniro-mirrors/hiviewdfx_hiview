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

#include <gtest/gtest.h>

#include "trace_common.h"
#include "trace_quota_config.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
class TraceQuotaConfigTest : public testing::Test {
public:
    void SetUp() {};

    void TearDown() {};

    static void SetUpTestCase() {};

    static void TearDownTestCase() {};
};

/**
 * @tc.name: TraceQuotaConfigTest001
 * @tc.desc: TraceQuotaConfig test
 * @tc.type: FUNC
 */
HWTEST_F(TraceQuotaConfigTest, TraceQuotaConfigTest001, testing::ext::TestSize.Level3)
{
    ASSERT_GE(TraceQuotaConfig::GetTraceQuotaByCaller(CallerName::XPERF), 0);
    ASSERT_GE(TraceQuotaConfig::GetTraceQuotaByCaller(CallerName::XPOWER), 0);
    ASSERT_GE(TraceQuotaConfig::GetTraceQuotaByCaller(CallerName::RELIABILITY), 0);
    ASSERT_GE(TraceQuotaConfig::GetTraceQuotaByCaller(CallerName::HIVIEW), 0);
    ASSERT_LT(TraceQuotaConfig::GetTraceQuotaByCaller(CallerName::SCREEN), 0);
}
} // namespace HiviewDFX
} // namespace OHOS