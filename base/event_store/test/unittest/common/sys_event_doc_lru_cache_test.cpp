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
 
#include "sys_event_doc_lru_cache.h"

namespace OHOS::HiviewDFX {
namespace EventStore {
using namespace testing::ext;
namespace {
constexpr size_t LRU_CACHE_TEST_CAPACITY = 2;
}

class SysEventDocLruCacheTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SysEventDocLruCacheTest::SetUpTestCase()
{
}

void SysEventDocLruCacheTest::TearDownTestCase()
{
}

void SysEventDocLruCacheTest::SetUp()
{
}

void SysEventDocLruCacheTest::TearDown()
{
}

/**
 * @tc.name: SysEventDocLruCacheTest001
 * @tc.desc: test Contain of SysEventDocLruCache
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(SysEventDocLruCacheTest, SysEventDocLruCacheTest001, TestSize.Level1)
{
    SysEventDocLruCache testLruCache(LRU_CACHE_TEST_CAPACITY);
    ASSERT_FALSE(testLruCache.Contain("TEST_KEY"));
    LruCacheValue value = nullptr;
    testLruCache.Add("TEST_KEY", value);
    ASSERT_TRUE(testLruCache.Contain("TEST_KEY"));
}

/**
 * @tc.name: SysEventDocLruCacheTest002
 * @tc.desc: test Get of SysEventDocLruCache
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(SysEventDocLruCacheTest, SysEventDocLruCacheTest002, TestSize.Level1)
{
    SysEventDocLruCache testLruCache(LRU_CACHE_TEST_CAPACITY);
    ASSERT_EQ(testLruCache.Get("TEST_KEY"), nullptr);
    LruCacheValue value = nullptr;
    testLruCache.Add("TEST_KEY", value);
    ASSERT_NE(testLruCache.Get("TEST_KEY"), nullptr);
}

/**
 * @tc.name: SysEventDocLruCacheTest003
 * @tc.desc: test Add of SysEventDocLruCache
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(SysEventDocLruCacheTest, SysEventDocLruCacheTest003, TestSize.Level1)
{
    SysEventDocLruCache testLruCache(LRU_CACHE_TEST_CAPACITY);
    LruCacheValue value = nullptr;
    ASSERT_TRUE(testLruCache.Add("TEST_KEY", value));
    ASSERT_FALSE(testLruCache.Add("TEST_KEY", value));
    ASSERT_TRUE(testLruCache.Add("TEST_KEY_NEW", value));
    ASSERT_FALSE(testLruCache.Add("TEST_KEY_NEW", value));
    ASSERT_FALSE(testLruCache.Add("TEST_KEY_NEWEST", value));
}

/**
 * @tc.name: SysEventDocLruCacheTest004
 * @tc.desc: test Remove of SysEventDocLruCache
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(SysEventDocLruCacheTest, SysEventDocLruCacheTest004, TestSize.Level1)
{
    SysEventDocLruCache testLruCache(LRU_CACHE_TEST_CAPACITY);
    ASSERT_FALSE(testLruCache.Remove("TEST_KEY"));
    LruCacheValue value = nullptr;
    testLruCache.Add("TEST_KEY", value);
    ASSERT_TRUE(testLruCache.Remove("TEST_KEY"));
}

/**
 * @tc.name: SysEventDocLruCacheTest005
 * @tc.desc: test Clear of SysEventDocLruCache
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(SysEventDocLruCacheTest, SysEventDocLruCacheTest005, TestSize.Level1)
{
    SysEventDocLruCache testLruCache(LRU_CACHE_TEST_CAPACITY);
    LruCacheValue value = nullptr;
    ASSERT_TRUE(testLruCache.Add("TEST_KEY", value));
    ASSERT_NE(testLruCache.Get("TEST_KEY"), nullptr);
    testLruCache.Clear();
    ASSERT_EQ(testLruCache.Get("TEST_KEY"), nullptr);
}
}
}