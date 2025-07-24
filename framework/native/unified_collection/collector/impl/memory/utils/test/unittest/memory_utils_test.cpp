/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include <map>
#include <set>
#include <string>

#include "memory_utils.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
namespace {
const char TEST_SMAPS_PATH[] = "/data/test/hiview/ucollection/smaps_example.txt";
const int SUM_1_TO_43 = 946; // 946 : total sum from 1 to 43
const std::map<MemoryItemType, std::string> TYPE_TO_NAME_MAP = {
    {MemoryItemType::MEMORY_ITEM_ENTITY_DB, "/data/other.db"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DB_SHM, "/data/storage/other.db-shm"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_HAP, "/system/app/other.hap"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_HSP, "/system/app/other.hsp"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_SO, "/system/lib/other.so"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_SO1, "/system/lib/other.so.1"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_TTF, "/system/fonts/other.ttf"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DEV_PARAMETER, "/dev/__parameters__/u:object_r:debug_param:s0"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DEV_OTHER, "/dev/other"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DATA_STORAGE, "/data/storage/other.hap"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DMABUF, "/dmabuf:1000"},
    {MemoryItemType::MEMORY_ITEM_ENTITY_OTHER, "/other"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_INODE, "anon_inode:dev/other"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARKTS_CODE, "[anon:ArkTS Code:1000]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARKTS_HEAP, "[anon:ArkTS Heap]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_GUARD, "[anon:guard:1000]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_BSS, "[anon:ld-musl-arm.so.1.bss]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_BRK, "[anon:native_heap:brk]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC, "[anon:native_heap:jemalloc]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_META, "[anon:native_heap:jemalloc meta]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_TSD, "[anon:native_heap:jemalloc tsd]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_META, "[anon:native_heap:meta]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_MMAP, "[anon:native_heap:mmap]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_OTHER, "[anon:native_heap:other]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_SIGNAL_STACK, "[anon:signal_stack:1000]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_STACK, "[anon:stack:1000]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_V8, "[anon:v8]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANONYMOUS_OTHER, "[anon:other]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_CONTIGUOUS, "[contiguous]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_COPAGE, "[copage]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_FILE, "[file]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_GUARD, "[guard]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_HEAP, "[heap]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_IO, "[io]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_KSHARE, "[kshare]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_MALLOC, "[anon:libc_malloc]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_PREHISTORIC, "[prehistoric]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_RESERVE, "[reserve]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_SHMM, "[shmm]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_STACK, "[stack]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_UNKNOWN, "[unknown]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_VNODES, "[vnodes]"},
    {MemoryItemType::MEMORY_ITEM_TYPE_GRAPH_GL, ""},
    {MemoryItemType::MEMORY_ITEM_TYPE_GRAPH_GRAPHICS, ""},
    {MemoryItemType::MEMORY_ITEM_TYPE_OTHER, "[other]"},
};
}
class MemoryUtilsTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: MemoryUtilsTest001
 * @tc.desc: used to test ParseSmaps api
 * @tc.type: FUNC
*/
HWTEST_F(MemoryUtilsTest, MemoryUtilsTest001, TestSize.Level1)
{
    ProcessMemoryDetail processMemoryDetail;
    ASSERT_TRUE(ParseSmaps(1, TEST_SMAPS_PATH, processMemoryDetail, GraphicMemOption::LOW_LATENCY));
    ASSERT_EQ(processMemoryDetail.totalRss, SUM_1_TO_43);
    ASSERT_EQ(processMemoryDetail.totalPss, SUM_1_TO_43);
    ASSERT_EQ(processMemoryDetail.totalSharedDirty, SUM_1_TO_43);
    ASSERT_EQ(processMemoryDetail.totalPrivateDirty, SUM_1_TO_43);
    ASSERT_EQ(processMemoryDetail.totalSharedClean, SUM_1_TO_43);
    ASSERT_EQ(processMemoryDetail.totalPrivateClean, SUM_1_TO_43);
    ASSERT_EQ(processMemoryDetail.totalSwap, SUM_1_TO_43);
    ASSERT_EQ(processMemoryDetail.totalSwapPss, SUM_1_TO_43);
    ASSERT_EQ(processMemoryDetail.totalAllPss, SUM_1_TO_43 * 2); // 2 : totalAllPss = totalPss + totalSwapPss
    ASSERT_EQ(processMemoryDetail.totalAllSwap, SUM_1_TO_43 * 2); // 2 : totalAllSwap = totalSwap + totalSwapPss
    ASSERT_EQ(processMemoryDetail.details.size(), 12); // 12 : total number of memory classes

    std::set<MemoryItemType> memoryTypes;
    for (const auto& detail : processMemoryDetail.details) {
        for (const auto& item : detail.items) {
            memoryTypes.insert(item.type);
            if (item.type == MemoryItemType::MEMORY_ITEM_TYPE_OTHER) {
                ASSERT_EQ(item.iNode, 43); // 43 : iNode value of other item type
            } else if (item.type == MemoryItemType::MEMORY_ITEM_TYPE_GRAPH_GL ||
                item.type == MemoryItemType::MEMORY_ITEM_TYPE_GRAPH_GRAPHICS) {
                ASSERT_EQ(item.iNode, 0);
            } else {
                ASSERT_EQ(item.iNode, static_cast<int32_t>(item.type) + 1);
            }
            if (TYPE_TO_NAME_MAP.find(item.type) != TYPE_TO_NAME_MAP.end()) {
                ASSERT_EQ(item.name, TYPE_TO_NAME_MAP.at(item.type));
            }
        }
    }
    ASSERT_EQ(memoryTypes.size(), 45); // 45 : max num of memory item type
}
}
}
