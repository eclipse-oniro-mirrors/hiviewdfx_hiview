/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>

#include "common_utils.h"
#include "file_util.h"
#include "string_util.h"
#include "memory_collector.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class MemoryCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_MEMORY_ENABLE
namespace {
bool HasValidAILibrary()
{
    const std::string libName = "libhiai_infra_proxy_1.0.z.so";
    void* handle = dlopen(libName.c_str(), RTLD_LAZY);
    return handle != nullptr;
}
}

/**
 * @tc.name: MemoryCollectorTest001
 * @tc.desc: used to test MemoryCollector.CollectProcessMemory
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<ProcessMemory> data = collector->CollectProcessMemory(1); // init process id
    std::cout << "collect process memory result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
    data = collector->CollectProcessMemory(-1); // invalid process id
    std::cout << "collect process memory result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::READ_FAILED);
}

/**
 * @tc.name: MemoryCollectorTest002
 * @tc.desc: used to test MemoryCollector.CollectSysMemory
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest002, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<SysMemory> data = collector->CollectSysMemory();
    std::cout << "collect system memory result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest003
 * @tc.desc: used to test MemoryCollector.CollectRawMemInfo
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest003, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::string> data = collector->CollectRawMemInfo();
    std::cout << "collect raw memory info result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest004
 * @tc.desc: used to test MemoryCollector.CollectAllProcessMemory
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest004, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::vector<ProcessMemory>> data = collector->CollectAllProcessMemory();
    std::cout << "collect all process memory result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest006
 * @tc.desc: used to test MemoryCollector.CollectRawSlabInfo
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest006, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::string> data = collector->CollectRawSlabInfo();
    std::cout << "collect raw slab info result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest007
 * @tc.desc: used to test MemoryCollector.CollectRawPageTypeInfo
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest007, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::string> data = collector->CollectRawPageTypeInfo();
    std::cout << "collect raw pagetype info result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest008
 * @tc.desc: used to test MemoryCollector.CollectRawDMA
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest008, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::string> data = collector->CollectRawDMA();
    std::cout << "collect raw DMA result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest009
 * @tc.desc: used to test MemoryCollector.CollectAllAIProcess
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest009, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::vector<AIProcessMem>> data = collector->CollectAllAIProcess();
    std::cout << "collect all AI process result" << data.retCode << std::endl;
    if (HasValidAILibrary()) {
        ASSERT_TRUE(data.retCode == UcError::SUCCESS);
    } else {
        ASSERT_TRUE(data.retCode == UcError::READ_FAILED);
    }
}

/**
 * @tc.name: MemoryCollectorTest010
 * @tc.desc: used to test MemoryCollector.ExportAllAIProcess
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest010, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::string> data = collector->ExportAllAIProcess();
    std::cout << "export all AI process result" << data.retCode << std::endl;
    if (HasValidAILibrary()) {
        ASSERT_TRUE(data.retCode == UcError::SUCCESS);
    } else {
        ASSERT_TRUE(data.retCode == UcError::READ_FAILED);
    }
}

/**
 * @tc.name: MemoryCollectorTest011
 * @tc.desc: used to test MemoryCollector.CollectRawSmaps
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest011, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::string> data = collector->CollectRawSmaps(1);
    std::cout << "collect raw smaps info result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest012
 * @tc.desc: used to test MemoryCollector.CollectHprof
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest012, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::string> data = collector->CollectHprof(1);
    std::cout << "collect heap snapshot result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest013
 * @tc.desc: used to test MemoryCollector.CollectProcessVss
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest013, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<uint64_t> data = collector->CollectProcessVss(1000);
    std::cout << "collect processvss result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest014
 * @tc.desc: used to test MemoryCollector.CollectMemoryLimit
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest014, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<MemoryLimit> data = collector->CollectMemoryLimit();
    std::cout << "collect memoryLimit result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest015
 * @tc.desc: used to test MemoryCollector.ExportMemView
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest015, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<std::string> data = collector->ExportMemView();
    std::cout << "collect raw memory view info result" << data.retCode << std::endl;
    if (FileUtil::FileExists("/proc/memview")) {
        ASSERT_EQ(data.retCode, UcError::SUCCESS);
    } else {
        ASSERT_EQ(data.retCode, UcError::UNSUPPORT);
    }
}

/**
 * @tc.name: MemoryCollectorTest016
 * @tc.desc: used to test MemoryCollector.CollectDdrFreq
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest016, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<uint32_t> data = collector->CollectDdrFreq();
    std::cout << "collect DDR current frequency info result" << data.retCode << std::endl;
    if (!FileUtil::FileExists("/sys/class/devfreq/ddrfreq/cur_freq")) {
        ASSERT_EQ(data.retCode, UcError::UNSUPPORT);
    } else {
        ASSERT_EQ(data.retCode, UcError::SUCCESS);
        ASSERT_GT(data.data, 0);
    }
}

/**
 * @tc.name: MemoryCollectorTest017
 * @tc.desc: used to test MemoryCollector.CollectDdrFreq
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest017, TestSize.Level1)
{
    std::cout << "MemoryCollector test" << std::endl;
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    auto data = collector->CollectProcessMemoryDetail(1, GraphicMemOption::LOW_MEMORY);
    std::cout << "collect processMemoryDetail result" << data.retCode << std::endl;
    ASSERT_EQ(data.data.name, "init");
    ASSERT_GT(data.data.totalPss, 0);
    ASSERT_GT(data.data.details.size(), 0);
    auto data2 = collector->CollectProcessMemoryDetail(1, GraphicMemOption::LOW_MEMORY);
    ASSERT_EQ(data2.retCode, UcError::SUCCESS);
    auto data3 = collector->CollectProcessMemoryDetail(1, GraphicMemOption::LOW_MEMORY);
    ASSERT_EQ(data3.retCode, UcError::SUCCESS);
}

/**
 * @tc.name: MemoryCollectorTest018
 * @tc.desc: used to test memory type
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest018, TestSize.Level3)
{
    ASSERT_EQ(MemoryCollector::MapNameToMemoryType("[anon:ArkTs Static Object Space]"),
        MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARK_OBJECT);
    ASSERT_EQ(MemoryCollector::MapNameToMemoryType("[anon:ArkTs Static Humongous Object Space]"),
        MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARK_HUMONGOUS_OBJECT);
    ASSERT_EQ(MemoryCollector::MapNameToMemoryType("[anon:ArkTs Static Non Movable Space]"),
        MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARK_NON_MOVABLE);
    ASSERT_EQ(MemoryCollector::MapMemoryTypeToClass(MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARK_OBJECT),
        MemoryClass::MEMORY_CLASS_ARK_STATIC_HEAP);
    ASSERT_EQ(MemoryCollector::MapMemoryTypeToClass(MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARK_HUMONGOUS_OBJECT),
        MemoryClass::MEMORY_CLASS_ARK_STATIC_HEAP);
    ASSERT_EQ(MemoryCollector::MapMemoryTypeToClass(MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARK_NON_MOVABLE),
        MemoryClass::MEMORY_CLASS_ARK_STATIC_HEAP);
}
#else
/**
 * @tc.name: MemoryCollectorTest001
 * @tc.desc: used to test empty MemoryCollector
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorTest, MemoryCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    auto ret1 = collector->CollectProcessMemory(0);
    ASSERT_EQ(ret1.retCode, UcError::FEATURE_CLOSED);

    auto ret2 = collector->CollectSysMemory();
    ASSERT_EQ(ret2.retCode, UcError::FEATURE_CLOSED);

    auto ret3 = collector->CollectRawMemInfo();
    ASSERT_EQ(ret3.retCode, UcError::FEATURE_CLOSED);

    auto ret4 = collector->ExportMemView();
    ASSERT_EQ(ret4.retCode, UcError::FEATURE_CLOSED);

    auto ret5 = collector->CollectAllProcessMemory();
    ASSERT_EQ(ret5.retCode, UcError::FEATURE_CLOSED);

    auto ret7 = collector->CollectRawSlabInfo();
    ASSERT_EQ(ret7.retCode, UcError::FEATURE_CLOSED);

    auto ret8 = collector->CollectRawPageTypeInfo();
    ASSERT_EQ(ret8.retCode, UcError::FEATURE_CLOSED);

    auto ret9 = collector->CollectRawDMA();
    ASSERT_EQ(ret9.retCode, UcError::FEATURE_CLOSED);

    auto ret10 = collector->CollectAllAIProcess();
    ASSERT_EQ(ret10.retCode, UcError::FEATURE_CLOSED);

    auto ret11 = collector->ExportAllAIProcess();
    ASSERT_EQ(ret11.retCode, UcError::FEATURE_CLOSED);

    auto ret12 = collector->CollectRawSmaps(0);
    ASSERT_EQ(ret12.retCode, UcError::FEATURE_CLOSED);

    auto ret13 = collector->CollectHprof(0);
    ASSERT_EQ(ret13.retCode, UcError::FEATURE_CLOSED);

    auto ret14 = collector->CollectProcessVss(0);
    ASSERT_EQ(ret14.retCode, UcError::FEATURE_CLOSED);

    auto ret15 = collector->CollectMemoryLimit();
    ASSERT_EQ(ret15.retCode, UcError::FEATURE_CLOSED);

    auto ret16 = collector->CollectDdrFreq();
    ASSERT_EQ(ret16.retCode, UcError::FEATURE_CLOSED);

    auto ret17 = collector->CollectProcessMemoryDetail(0, GraphicMemOption::NONE);
    ASSERT_EQ(ret17.retCode, UcError::FEATURE_CLOSED);
}
#endif
