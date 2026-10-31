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

#include <string>

#include <gtest/gtest.h>

#include "boot_detector_hiview.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {

/**
 * @tc.name: BootDetectorHiview001
 * @tc.desc: "/data" exactly (path length == prefix length) is rewritten.
 * @tc.type: FUNC
 */
HWTEST(BootDetectorHiviewTest, BootDetectorHiview001, TestSize.Level1)
{
    EXPECT_EQ(BootDetectorReplaceDataPrefix("/data"), "/mnt/data_mirror");
}

/**
 * @tc.name: BootDetectorHiview002
 * @tc.desc: "/data/..." (segment boundary is '/') is rewritten with the suffix preserved.
 * @tc.type: FUNC
 */
HWTEST(BootDetectorHiviewTest, BootDetectorHiview002, TestSize.Level1)
{
    EXPECT_EQ(BootDetectorReplaceDataPrefix("/data/app/el2/100"), "/mnt/data_mirror/app/el2/100");
}

/**
 * @tc.name: BootDetectorHiview003
 * @tc.desc: "/database" shares the prefix bytes but is not a whole segment, so it is unchanged.
 * @tc.type: FUNC
 */
HWTEST(BootDetectorHiviewTest, BootDetectorHiview003, TestSize.Level1)
{
    EXPECT_EQ(BootDetectorReplaceDataPrefix("/database"), "/database");
}

/**
 * @tc.name: BootDetectorHiview004
 * @tc.desc: A path not starting with "/data" is returned unchanged.
 * @tc.type: FUNC
 */
HWTEST(BootDetectorHiviewTest, BootDetectorHiview004, TestSize.Level1)
{
    EXPECT_EQ(BootDetectorReplaceDataPrefix("/system/lib/foo"), "/system/lib/foo");
}

/**
 * @tc.name: BootDetectorHiview005
 * @tc.desc: An empty string (path length < prefix length) yields an empty result.
 * @tc.type: FUNC
 */
HWTEST(BootDetectorHiviewTest, BootDetectorHiview005, TestSize.Level1)
{
    EXPECT_EQ(BootDetectorReplaceDataPrefix(""), "");
}
} // namespace HiviewDFX
} // namespace OHOS
