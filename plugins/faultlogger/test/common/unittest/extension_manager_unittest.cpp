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

#include "faultlog_ext_connection.h"
#include "faultlog_ext_conn_manager.h"
#include "faultlog_info_inner.h"

namespace OHOS {
namespace HiviewDFX {
/**
 * @tc.name: RecordOperateTest001
 * @tc.desc: Test record operate
 * @tc.type: FUNC
 */
HWTEST(FaultLogExtConnManagerUnittest, RecordOperateTest001, testing::ext::TestSize.Level1)
{
    const int32_t uid = 20020200;
    FaultLogExtConnManager mgr;
    ASSERT_EQ(mgr.waitStartList_.size(), 0);

    mgr.AddToList("com.test.bundle.name", uid);
    ASSERT_EQ(mgr.waitStartList_.size(), 1);

    ASSERT_TRUE(mgr.IsExistList("com.test.bundle.name", uid));

    mgr.RemoveFromList("com.test.bundle.name", uid);
    ASSERT_EQ(mgr.waitStartList_.size(), 0);

    ASSERT_FALSE(mgr.IsExistList("com.test.bundle.name", uid));
}

/**
 * @tc.name: IsExtensionTest001
 * @tc.desc: Test IsExtension
 * @tc.type: FUNC
 */
HWTEST(FaultLogExtConnManagerUnittest, IsExtensionTest001, testing::ext::TestSize.Level1)
{
    FaultLogInfo info;
    FaultLogExtConnManager mgr;

    info.sectionMap["PROCESS_NAME"] = "com.test.bundle.name";
    ASSERT_FALSE(mgr.IsExtension(info));

    info.sectionMap["PROCESS_NAME"] = "com.test.bundle.name:faultLog";
    ASSERT_TRUE(mgr.IsExtension(info));
}

/**
 * @tc.name: GetExtNameTest001
 * @tc.desc: Test GetExtName
 * @tc.type: FUNC
 */
HWTEST(FaultLogExtConnManagerUnittest, GetExtNameTest001, testing::ext::TestSize.Level1)
{
    FaultLogExtConnManager mgr;
    ASSERT_EQ(mgr.GetExtName("com.test.bundle.name", 0), "");
}

/**
 * @tc.name: GetExtNameTest002
 * @tc.desc: Test GetExtName
 * @tc.type: FUNC
 */
HWTEST(FaultLogExtConnManagerUnittest, GetExtNameTest002, testing::ext::TestSize.Level1)
{
    FaultLogExtConnManager mgr;
    ASSERT_EQ(mgr.GetExtName("com.test.bundle.name", 100), "");
    ASSERT_EQ(mgr.GetExtName("com.ohos.mms", 100), "");
    ASSERT_EQ(mgr.GetExtName("com.ohos.telephonydataability", 100), "");
    ASSERT_EQ(mgr.GetExtName("com.ohos.medialibrary.medialibrarydata", 100), "");
}
} // namespace HiviewDFX
} // namespace OHOS
