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
#include <gtest/gtest.h>

#include "faultlogger_base.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
/**
 * @tc.name: QuerySelfFaultLog001
 * @tc.desc: Test calling querySelfFaultLog Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerBaseTest, QuerySelfFaultLog001, testing::ext::TestSize.Level3)
{
    FaultloggerBase faultloggerBase;
    faultloggerBase.QuerySelfFaultLog(100001, 0, 10, 101);
    faultloggerBase.QuerySelfFaultLog(100001, 0, 4, 101);
    auto ret = faultloggerBase.QuerySelfFaultLog(100001, 0, -1, 101);
    ASSERT_TRUE(ret == nullptr);
}

/**
 * @tc.name: GetGwpAsanGrayscaleState001
 * @tc.desc: Test calling GwpAsanGrayscal Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerBaseTest, GetGwpAsanGrayscaleState001, testing::ext::TestSize.Level3)
{
    FaultloggerBase faultloggerBase;
    faultloggerBase.EnableGwpAsanGrayscale(false, 1000, 2000, 5, static_cast<int64_t>(getuid()));
    faultloggerBase.EnableGwpAsanGrayscale(true, 2523, 2000, 5, static_cast<int64_t>(getuid()));
    faultloggerBase.DisableGwpAsanGrayscale(static_cast<int64_t>(getuid()));
    ASSERT_TRUE(faultloggerBase.GetGwpAsanGrayscaleState(static_cast<int64_t>(getuid())) >= 0);
}
} // namespace HiviewDFX
} // namespace OHOS
