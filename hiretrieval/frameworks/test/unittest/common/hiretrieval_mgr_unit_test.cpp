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

#include "hiretrieval_mgr_unit_test.h"

#include <gmock/gmock.h>

#include "hiretrieval_mgr.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char PREFERNECE_DIR[] = "//data/test/";

bool IsUnitializedCfg(const HiRetrievalMgr::Config& cfg)
{
    return std::string(cfg.userType).empty() &&
        std::string(cfg.deviceType).empty() &&
        std::string(cfg.deviceModel).empty();
}

bool IsSameCfg(const HiRetrievalMgr::Config& src, const HiRetrievalMgr::Config& dest)
{
    return std::string(src.userType) == std::string(dest.userType) &&
        std::string(src.deviceType) == std::string(dest.deviceType) &&
        std::string(src.deviceModel) == std::string(dest.deviceModel);
}

void RebuildConfigMemToSpecifiedLen(HiRetrievalMgr::Config& cfg, const size_t cnt)
{
    cfg.userType = "";
    cfg.userType.append(cnt, '0');
    cfg.deviceType = "";
    cfg.deviceType.append(cnt, '0');
    cfg.deviceModel = "";
    cfg.deviceModel.append(cnt, '0');
}
}

void HiRetrievalMgrUnitTest::SetUpTestCase()
{
}

void HiRetrievalMgrUnitTest::TearDownTestCase()
{
}

void HiRetrievalMgrUnitTest::SetUp()
{
}

void HiRetrievalMgrUnitTest::TearDown()
{
}

/**
 * @tc.name: HiRetrievalMgrUnitTest001
 * @tc.desc: test the GetLastParticipationTs of class HiRetrievalMgr
 * @tc.type: FUNC
 * @tc.require: issue3276
 */
HWTEST_F(HiRetrievalMgrUnitTest, HiRetrievalMgrUnitTest001, testing::ext::TestSize.Level3)
{
    HiRetrievalMgr::Config cfg {
        "user_type1",
        "device_type1",
        "device_model1",
    };
    auto& instance = HiRetrievalMgr::GetInstance();
    instance.SetWorkDir(PREFERNECE_DIR);
    ASSERT_EQ(instance.GetLastParticipationTs(), 0);
    auto ret = instance.Init();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ret = instance.Participate(cfg);
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ASSERT_GT(instance.GetLastParticipationTs(), 0);
    ret = instance.Quit();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ASSERT_GT(instance.GetLastParticipationTs(), 0);
}

/**
 * @tc.name: HiRetrievalMgrUnitTest002
 * @tc.desc: test the Participate/Quit/Run of class HiRetrievalMgr
 * @tc.type: FUNC
 * @tc.require: issue3276
 */
HWTEST_F(HiRetrievalMgrUnitTest, HiRetrievalMgrUnitTest002, testing::ext::TestSize.Level3)
{
    auto& instance = HiRetrievalMgr::GetInstance();
    instance.Quit();
    HiRetrievalMgr::Config cfg {
        "user_type2",
        "device_type2",
        "device_model2",
    };
    instance.SetWorkDir(PREFERNECE_DIR);
    auto ret = instance.Participate(cfg);
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::NOT_INIT);
    ret = instance.Run();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::NOT_INIT);
    ret = instance.Quit();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::NOT_INIT);
    ret = instance.Init();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ret = instance.Participate(cfg);
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ret = instance.Run();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ret = instance.Quit();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ret = instance.Quit();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::NOT_INIT);
}

/**
 * @tc.name: HiRetrievalMgrUnitTest003
 * @tc.desc: test the IsParticipant of class HiRetrievalMgr
 * @tc.type: FUNC
 * @tc.require: issue3276
 */
HWTEST_F(HiRetrievalMgrUnitTest, HiRetrievalMgrUnitTest003, testing::ext::TestSize.Level3)
{
    auto& instance = HiRetrievalMgr::GetInstance();
    instance.Quit();
    HiRetrievalMgr::Config cfg {
        "user_type3",
        "device_type3",
        "device_model3",
    };
    instance.SetWorkDir(PREFERNECE_DIR);
    auto ret = instance.Participate(cfg);
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::NOT_INIT);
    ASSERT_FALSE(instance.IsParticipant());
    ret = instance.Init();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ASSERT_FALSE(instance.IsParticipant());
    ret = instance.Participate(cfg);
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ASSERT_TRUE(instance.IsParticipant());
    ret = instance.Quit();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ASSERT_FALSE(instance.IsParticipant());
}

/**
 * @tc.name: HiRetrievalMgrUnitTest004
 * @tc.desc: test the GetCurrentConfig of class HiRetrievalMgr
 * @tc.type: FUNC
 * @tc.require: issue3276
 */
HWTEST_F(HiRetrievalMgrUnitTest, HiRetrievalMgrUnitTest004, testing::ext::TestSize.Level3)
{
    auto& instance = HiRetrievalMgr::GetInstance();
    instance.Quit();
    HiRetrievalMgr::Config cfg {
        "user_type4",
        "device_type4",
        "device_model4",
    };
    instance.SetWorkDir(PREFERNECE_DIR);
    auto preCfg = instance.GetCurrentConfig();
    ASSERT_TRUE(IsUnitializedCfg(preCfg));
    auto ret = instance.Init();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    ret = instance.Participate(cfg);
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    auto nailCfg = instance.GetCurrentConfig();
    ASSERT_FALSE(IsSameCfg(preCfg, nailCfg));
    ret = instance.Quit();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    nailCfg = instance.GetCurrentConfig();
    ASSERT_TRUE(IsUnitializedCfg(nailCfg));
}

/**
 * @tc.name: HiRetrievalMgrUnitTest005
 * @tc.desc: test the max length of member of HiRetrievalConfig
 * @tc.type: FUNC
 * @tc.require: issue3276
 */
HWTEST_F(HiRetrievalMgrUnitTest, HiRetrievalMgrUnitTest005, testing::ext::TestSize.Level3)
{
    auto& instance = HiRetrievalMgr::GetInstance();
    instance.SetWorkDir(PREFERNECE_DIR);
    instance.Quit();
    auto ret = instance.Init();
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    HiRetrievalMgr::Config cfg;
    int maxLen = 128; // support max length is 128
    RebuildConfigMemToSpecifiedLen(cfg, maxLen);
    ret = instance.Participate(cfg);
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    cfg = instance.GetCurrentConfig();
    ASSERT_EQ(cfg.userType.length(), maxLen);
    ASSERT_EQ(cfg.deviceType.length(), maxLen);
    ASSERT_EQ(cfg.deviceType.length(), maxLen);
    int overLimitLen = 130; // 130 is a test length which is more than limit len
    RebuildConfigMemToSpecifiedLen(cfg, overLimitLen);
    ret = instance.Participate(cfg);
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    cfg = instance.GetCurrentConfig();
    ASSERT_EQ(cfg.userType.length(), maxLen);
    ASSERT_EQ(cfg.deviceType.length(), maxLen);
    ASSERT_EQ(cfg.deviceType.length(), maxLen);
    int validLen = 100; // 100 is a test length which is less than limit len
    RebuildConfigMemToSpecifiedLen(cfg, validLen);
    ret = instance.Participate(cfg);
    ASSERT_TRUE(ret == HiRetrieval::NativeErrorCode::DLL_FAILED || ret == HiRetrieval::NativeErrorCode::SUCC);
    cfg = instance.GetCurrentConfig();
    ASSERT_EQ(cfg.userType.length(), validLen);
    ASSERT_EQ(cfg.deviceType.length(), validLen);
    ASSERT_EQ(cfg.deviceType.length(), validLen);
}
} // namespace HiviewDFX
} // namespace OHOS