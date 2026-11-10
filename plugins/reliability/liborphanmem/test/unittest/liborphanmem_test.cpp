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
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <sys/mman.h>
#include "chunk.h"
#include "collect_thread.h"
#include "heap_impl.h"
#include "linked_list.h"
#include "map_allocator.h"
#include "memory_mapper.h"
#include "performance_tracer.h"

#include <sstream>
#include <unistd.h>
#include <securec.h>
#include "heap_scanner.h"
#include "leak_info.h"
#include "orphanmem_detector.h"
#include "orphanmem_snapshot.h"
#include "orphanmem_snapshot_json_serializer.h"
#include "process_mappings.h"
#include "process_pipe.h"
#include "string_helper.h"
using namespace testing::ext;
using namespace OHOS;


// ===== From linked_list_test.cpp =====
class LinkedListTest : public ::testing::Test {};

/**
 * @tc.name  : LinkedList_ShouldBeEmpty_AfterDefaultConstruction
 * @tc.number: LinkedList_Constructor_Test_001
 * @tc.desc  : Test that default constructed LinkedList is empty and self-referential.
 */
HWTEST_F(LinkedListTest, DefaultConstructor_Empty, TestSize.Level1)
{
    LinkedList<int> list;
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.next_, &list);
    EXPECT_EQ(list.prev_, &list);
}

/**
 * @tc.name  : LinkedList_ShouldStoreData_AfterConstructionWithData
 * @tc.number: LinkedList_Constructor_Test_002
 * @tc.desc  : Test that LinkedList constructed with data stores the value and is still empty.
 */
HWTEST_F(LinkedListTest, ConstructorWithData, TestSize.Level1)
{
    LinkedList<int> list(42);
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.data_, 42);
}

/**
 * @tc.name  : LinkedList_ShouldNotBeEmpty_AfterInsert
 * @tc.number: LinkedList_Insert_Test_001
 * @tc.desc  : Test that LinkedList is not empty after inserting a node.
 */
HWTEST_F(LinkedListTest, InsertAndEmpty, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> node1(1);
    head.Insert(node1);
    EXPECT_FALSE(head.Empty());
    EXPECT_EQ(head.next_->data_, 1);
}

/**
 * @tc.name  : LinkedList_ShouldInsertInReverseOrder_WhenMultipleInserts
 * @tc.number: LinkedList_Insert_Test_002
 * @tc.desc  : Test that insert adds nodes at the head, resulting in reverse insertion order.
 */
HWTEST_F(LinkedListTest, InsertMultipleNodes, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> node1(1);
    LinkedList<int> node2(2);
    head.Insert(node1);
    head.Insert(node2);
    EXPECT_EQ(head.next_->data_, 2);
    EXPECT_EQ(head.next_->next_->data_, 1);
}

/**
 * @tc.name  : LinkedList_ShouldBeEmpty_AfterRemovingOnlyNode
 * @tc.number: LinkedList_Remove_Test_001
 * @tc.desc  : Test that LinkedList becomes empty after removing the only node.
 */
HWTEST_F(LinkedListTest, RemoveSingleNode, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> node1(1);
    head.Insert(node1);
    node1.Remove();
    EXPECT_TRUE(head.Empty());
}

/**
 * @tc.name  : LinkedList_ShouldKeepOtherNodes_AfterRemovingOneNode
 * @tc.number: LinkedList_Remove_Test_002
 * @tc.desc  : Test that other nodes remain after removing one node from a multi-node list.
 */
HWTEST_F(LinkedListTest, RemoveNodeOthersRemain, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> node1(1);
    LinkedList<int> node2(2);
    head.Insert(node1);
    head.Insert(node2);
    node2.Remove();
    EXPECT_FALSE(head.Empty());
    EXPECT_EQ(head.next_->data_, 1);
}

/**
 * @tc.name  : LinkedList_ShouldSupportReInsert_AfterRemove
 * @tc.number: LinkedList_Remove_Test_003
 * @tc.desc  : Test that a removed node can be re-inserted into the list.
 */
HWTEST_F(LinkedListTest, RemoveThenReInsert, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> node1(1);
    head.Insert(node1);
    node1.Remove();
    EXPECT_TRUE(head.Empty());
    head.Insert(node1);
    EXPECT_FALSE(head.Empty());
}

/**
 * @tc.name  : LinkedList_ShouldBeSelfReferential_AfterRemove
 * @tc.number: LinkedList_Remove_Test_004
 * @tc.desc  : Test that a removed node becomes self-referential (next/prev point to itself).
 */
HWTEST_F(LinkedListTest, SelfReferentialAfterRemove, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> node1(1);
    head.Insert(node1);
    node1.Remove();
    EXPECT_EQ(node1.next_, &node1);
    EXPECT_EQ(node1.prev_, &node1);
}

/**
 * @tc.name  : LinkedList_ShouldMaintainInsertionOrder_WhenInsertingMultiple
 * @tc.number: LinkedList_Insert_Test_003
 * @tc.desc  : Test that insert at head maintains the expected LIFO ordering.
 */
HWTEST_F(LinkedListTest, InsertAtHeadVerifyOrdering, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> a(10);
    LinkedList<int> b(20);
    LinkedList<int> c(30);
    head.Insert(a);
    head.Insert(b);
    head.Insert(c);
    EXPECT_EQ(head.next_->data_, 30);
    EXPECT_EQ(head.next_->next_->data_, 20);
    EXPECT_EQ(head.next_->next_->next_->data_, 10);
}

/**
 * @tc.name  : LinkedList_ShouldRemainEmpty_AfterRemovingHead
 * @tc.number: LinkedList_Remove_Test_005
 * @tc.desc  : Test that removing the head node from empty list keeps it empty.
 */
HWTEST_F(LinkedListTest, RemoveHeadNoEffect, TestSize.Level1)
{
    LinkedList<int> head;
    head.Remove();
    EXPECT_TRUE(head.Empty());
}

/**
 * @tc.name  : LinkedList_ShouldLinkCorrectly_AfterRemovingMiddleNode
 * @tc.number: LinkedList_Remove_Test_006
 * @tc.desc  : Test that removing a middle node correctly links the remaining nodes.
 */
HWTEST_F(LinkedListTest, RemoveMiddleNode, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> a(10);
    LinkedList<int> b(20);
    LinkedList<int> c(30);
    head.Insert(a);
    head.Insert(b);
    head.Insert(c);
    b.Remove();
    EXPECT_EQ(head.next_->data_, 30);
    EXPECT_EQ(head.next_->next_->data_, 10);
}

/**
 * @tc.name  : LinkedList_ShouldHaveCorrectPrevNext_AfterInsert
 * @tc.number: LinkedList_Insert_Test_004
 * @tc.desc  : Test that prev/next pointers are consistent after insert operation.
 */
HWTEST_F(LinkedListTest, PrevNextConsistencyAfterInsert, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> node1(1);
    head.Insert(node1);
    EXPECT_EQ(node1.prev_, &head);
    EXPECT_EQ(node1.next_, &head);
    EXPECT_EQ(head.prev_, &node1);
}

/**
 * @tc.name  : LinkedList_ShouldBeEmpty_AfterRemovingAllNodes
 * @tc.number: LinkedList_Remove_Test_007
 * @tc.desc  : Test that list becomes empty after removing all inserted nodes.
 */
HWTEST_F(LinkedListTest, EmptyAfterRemovingAll, TestSize.Level1)
{
    LinkedList<int> head;
    LinkedList<int> a(1);
    LinkedList<int> b(2);
    head.Insert(a);
    head.Insert(b);
    a.Remove();
    b.Remove();
    EXPECT_TRUE(head.Empty());
}

/**
 * @tc.name  : LinkedList_ShouldStorePointerType_AfterInsert
 * @tc.number: LinkedList_Insert_Test_005
 * @tc.desc  : Test that LinkedList works correctly with pointer type data.
 */
HWTEST_F(LinkedListTest, PointerTypeData, TestSize.Level1)
{
    LinkedList<int *> head;
    int val = 42;
    LinkedList<int *> node(&val);
    head.Insert(node);
    EXPECT_EQ(*head.next_->data_, 42);
}

/**
 * @tc.name  : LinkedList_ShouldHaveNullData_ForDefaultHeadNode
 * @tc.number: LinkedList_Constructor_Test_003
 * @tc.desc  : Test that default constructed LinkedList head has null data and is empty.
 */
HWTEST_F(LinkedListTest, ChunkPtrType, TestSize.Level1)
{
    LinkedList<int *> head;
    EXPECT_TRUE(head.Empty());
    EXPECT_EQ(head.data_, nullptr);
}


// ===== From map_allocator_test.cpp =====
class AllocatorTest : public ::testing::Test {
protected:
    Heap heap;
};

/**
 * @tc.name  : Heap_AllocDealloc_ShouldSucceed_WhenNormalSize
 * @tc.number: Heap_Alloc_Test_001
 * @tc.desc  : Test that Heap can allocate and deallocate a normal-sized block.
 */
HWTEST_F(AllocatorTest, HeapAllocDealloc, TestSize.Level1)
{
    void *ptr = heap.Allocate(64);
    ASSERT_NE(ptr, nullptr);
    heap.Deallocate(ptr);
}

/**
 * @tc.name  : Heap_Alloc_ShouldSucceed_WhenMinBucketSize
 * @tc.number: Heap_Alloc_Test_002
 * @tc.desc  : Test that Heap can allocate exactly K_MIN_BUCKET_ALLOCATION_SIZE bytes.
 */
HWTEST_F(AllocatorTest, HeapAllocateMinSize, TestSize.Level1)
{
    void *ptr = heap.Allocate(K_MIN_BUCKET_ALLOCATION_SIZE);
    ASSERT_NE(ptr, nullptr);
    heap.Deallocate(ptr);
}

/**
 * @tc.name  : Heap_Alloc_ShouldRoundUp_WhenBelowMinSize
 * @tc.number: Heap_Alloc_Test_003
 * @tc.desc  : Test that Heap allocates even when requested size is below minimum bucket size.
 */
HWTEST_F(AllocatorTest, HeapAllocateBelowMinSize, TestSize.Level1)
{
    void *ptr = heap.Allocate(1);
    ASSERT_NE(ptr, nullptr);
    heap.Deallocate(ptr);
}

/**
 * @tc.name  : Heap_Alloc_ShouldNotCrash_WhenZeroSize
 * @tc.number: Heap_Alloc_Test_004
 * @tc.desc  : Test that Heap allocation with zero size does not crash.
 */
HWTEST_F(AllocatorTest, HeapAllocateZeroSize, TestSize.Level1)
{
    void *ptr = heap.Allocate(0);
    heap.Deallocate(ptr);
}

/**
 * @tc.name  : Heap_Alloc_ShouldUseMapAlloc_WhenAboveBucketMax
 * @tc.number: Heap_Alloc_Test_005
 * @tc.desc  : Test that Heap uses MapAlloc path when size exceeds K_MAX_BUCKET_ALLOCATION_SIZE.
 */
HWTEST_F(AllocatorTest, HeapAllocateLargeSize, TestSize.Level1)
{
    void *ptr = heap.Allocate(K_CHUNK_SIZE / 2);
    if (ptr != nullptr) {
        heap.Deallocate(ptr);
    }
}

/**
 * @tc.name  : Heap_Empty_ShouldReturnTrue_WhenNoAllocations
 * @tc.number: Heap_Empty_Test_001
 * @tc.desc  : Test that Heap is empty before any allocations.
 */
HWTEST_F(AllocatorTest, HeapEmptyInitially, TestSize.Level1)
{
    EXPECT_TRUE(heap.Empty());
}

/**
 * @tc.name  : Heap_Empty_ShouldReturnFalse_WhenHasAllocations
 * @tc.number: Heap_Empty_Test_002
 * @tc.desc  : Test that Heap is not empty after an allocation.
 */
HWTEST_F(AllocatorTest, HeapNotEmptyAfterAllocation, TestSize.Level1)
{
    void *ptr = heap.Allocate(64);
    EXPECT_FALSE(heap.Empty());
    heap.Deallocate(ptr);
}

/**
 * @tc.name  : Heap_Alloc_ShouldSucceed_WhenMultipleSizes
 * @tc.number: Heap_Alloc_Test_006
 * @tc.desc  : Test that Heap can allocate blocks of different sizes simultaneously.
 */
HWTEST_F(AllocatorTest, HeapAllocateMultipleSizes, TestSize.Level1)
{
    void *p1 = heap.Allocate(8);
    void *p2 = heap.Allocate(16);
    void *p3 = heap.Allocate(32);
    void *p4 = heap.Allocate(64);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);
    ASSERT_NE(p4, nullptr);
    heap.Deallocate(p1);
    heap.Deallocate(p2);
    heap.Deallocate(p3);
    heap.Deallocate(p4);
}

/**
 * @tc.name  : Heap_CopyConstructor_ShouldShareImpl
 * @tc.number: Heap_Copy_Test_001
 * @tc.desc  : Test that Heap copy constructor shares the same impl and does not own it.
 */
HWTEST_F(AllocatorTest, HeapCopyConstructor, TestSize.Level1)
{
    Heap copy(heap);
    EXPECT_TRUE(copy == heap);
    void *ptr = copy.Allocate(64);
    ASSERT_NE(ptr, nullptr);
    copy.Deallocate(ptr);
}

/**
 * @tc.name  : Heap_OperatorEqual_ShouldReturnTrue_WhenSameImpl
 * @tc.number: Heap_Compare_Test_001
 * @tc.desc  : Test that operator== returns true for heaps sharing the same impl.
 */
HWTEST_F(AllocatorTest, HeapOperatorEqualSameHeap, TestSize.Level1)
{
    Heap copy(heap);
    EXPECT_TRUE(copy == heap);
}

/**
 * @tc.name  : Heap_OperatorNotEqual_ShouldReturnTrue_WhenDifferentImpl
 * @tc.number: Heap_Compare_Test_002
 * @tc.desc  : Test that operator!= returns true for heaps with different impls.
 */
HWTEST_F(AllocatorTest, HeapOperatorNotEqualDifferentHeaps, TestSize.Level1)
{
    Heap other;
    EXPECT_TRUE(heap != other);
}

/**
 * @tc.name  : Heap_Alloc_ShouldSucceed_WhenTemplateType
 * @tc.number: Heap_Alloc_Test_007
 * @tc.desc  : Test that Heap template allocate works correctly.
 */
HWTEST_F(AllocatorTest, HeapAllocateTemplateType, TestSize.Level1)
{
    int *ptr = heap.Allocate<int>();
    ASSERT_NE(ptr, nullptr);
    heap.Deallocate(ptr);
}

/**
 * @tc.name  : STLAllocator_ShouldAllocateAndDeallocate_WhenDefaultConstructed
 * @tc.number: STLAllocator_Test_001
 * @tc.desc  : Test that default-constructed STLAllocator can allocate and deallocate.
 */
HWTEST_F(AllocatorTest, STLAllocatorDefaultConstructor, TestSize.Level1)
{
    STLAllocator<int> alloc;
    int *ptr = alloc.allocate(10);
    ASSERT_NE(ptr, nullptr);
    alloc.deallocate(ptr, 10);
}

/**
 * @tc.name  : STLAllocator_ShouldAllocateAndDeallocate_WhenConstructedFromHeap
 * @tc.number: STLAllocator_Test_002
 * @tc.desc  : Test that Heap-constructed STLAllocator can allocate and deallocate.
 */
HWTEST_F(AllocatorTest, STLAllocatorFromHeap, TestSize.Level1)
{
    STLAllocator<int> alloc(heap);
    int *ptr = alloc.allocate(5);
    ASSERT_NE(ptr, nullptr);
    alloc.deallocate(ptr, 5);
}

/**
 * @tc.name  : STLAllocator_ShouldSupportRebind_WhenDifferentType
 * @tc.number: STLAllocator_Test_003
 * @tc.desc  : Test that STLAllocator rebind constructor works for different types.
 */
HWTEST_F(AllocatorTest, STLAllocatorRebindConstructor, TestSize.Level1)
{
    STLAllocator<int> allocInt(heap);
    STLAllocator<double> allocDouble(allocInt);
    double *ptr = allocDouble.allocate(3);
    ASSERT_NE(ptr, nullptr);
    allocDouble.deallocate(ptr, 3);
}

/**
 * @tc.name  : STLAllocator_ShouldReturnTrue_WhenSameHeap
 * @tc.number: STLAllocator_Compare_Test_001
 * @tc.desc  : Test that operator== returns true for STLAllocators from the same heap.
 */
HWTEST_F(AllocatorTest, STLAllocatorOperatorEqual, TestSize.Level1)
{
    STLAllocator<int> a1(heap);
    STLAllocator<int> a2(heap);
    EXPECT_TRUE(a1 == a2);
}

/**
 * @tc.name  : STLAllocator_ShouldReturnTrue_WhenDifferentHeap
 * @tc.number: STLAllocator_Compare_Test_002
 * @tc.desc  : Test that operator!= returns true for STLAllocators from different heaps.
 */
HWTEST_F(AllocatorTest, STLAllocatorOperatorNotEqual, TestSize.Level1)
{
    Heap heap1, heap2;
    STLAllocator<int> a1(heap1);
    STLAllocator<int> a2(heap2);
    EXPECT_TRUE(a1 != a2);
}

/**
 * @tc.name  : Allocator_MakeUnique_ShouldReturnValidPointer
 * @tc.number: Allocator_MakeUnique_Test_001
 * @tc.desc  : Test that Allocator make_unique returns a valid unique_ptr with correct value.
 */
HWTEST_F(AllocatorTest, AllocatorMakeUnique, TestSize.Level1)
{
    Allocator<int> alloc(heap);
    auto ptr = alloc.MakeUnique(42);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 42);
}

/**
 * @tc.name  : Allocator_ShouldAllocateAndDeallocate_WhenFromHeap
 * @tc.number: Allocator_Alloc_Test_001
 * @tc.desc  : Test that Allocator from Heap can allocate and deallocate.
 */
HWTEST_F(AllocatorTest, AllocatorFromHeap, TestSize.Level1)
{
    Allocator<int> alloc(heap);
    int *ptr = alloc.allocate(1);
    ASSERT_NE(ptr, nullptr);
    alloc.deallocate(ptr, 1);
}

/**
 * @tc.name  : Allocator_ShouldSupportRebind_WhenDifferentType
 * @tc.number: Allocator_Rebind_Test_001
 * @tc.desc  : Test that Allocator rebind constructor works for different types.
 */
HWTEST_F(AllocatorTest, AllocatorRebindConstructor, TestSize.Level1)
{
    Allocator<int> allocInt(heap);
    Allocator<double> allocDouble(allocInt);
    double *ptr = allocDouble.allocate(1);
    ASSERT_NE(ptr, nullptr);
    allocDouble.deallocate(ptr, 1);
}

/**
 * @tc.name  : Allocator_Deallocate_ShouldWork_WhenVoidPtr
 * @tc.number: Allocator_Dealloc_Test_001
 * @tc.desc  : Test that Allocator deallocate(void*) works correctly.
 */
HWTEST_F(AllocatorTest, AllocatorDeallocateVoidPtr, TestSize.Level1)
{
    Allocator<int> alloc(heap);
    int *ptr = alloc.allocate(1);
    ASSERT_NE(ptr, nullptr);
    alloc.deallocate(ptr, 1);
}

/**
 * @tc.name  : MapAllocator_Vector_ShouldWorkCorrectly
 * @tc.number: MapAllocator_Container_Test_001
 * @tc.desc  : Test that MAP_ALLOCATOR::vector works with custom allocator.
 */
HWTEST_F(AllocatorTest, MapAllocatorVector, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<int> vec((Allocator<int>(heap)));
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    EXPECT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[2], 3);
}

/**
 * @tc.name  : MapAllocator_String_ShouldWorkCorrectly
 * @tc.number: MapAllocator_Container_Test_002
 * @tc.desc  : Test that MAP_ALLOCATOR::String works with custom allocator.
 */
HWTEST_F(AllocatorTest, MapAllocatorString, TestSize.Level1)
{
    MAP_ALLOCATOR::String str((Allocator<char>(heap)));
    str = "hello";
    EXPECT_EQ(str, "hello");
    str += " world";
    EXPECT_EQ(str, "hello world");
}

/**
 * @tc.name  : MapAllocator_Map_ShouldWorkCorrectly
 * @tc.number: MapAllocator_Container_Test_003
 * @tc.desc  : Test that MAP_ALLOCATOR::map works with custom allocator.
 */
HWTEST_F(AllocatorTest, MapAllocatorMap, TestSize.Level1)
{
    MAP_ALLOCATOR::map<int, int> m((Allocator<MAP_ALLOCATOR::map<int, int>::value_type>(heap)));
    m[1] = 10;
    m[2] = 20;
    EXPECT_EQ(m[1], 10);
    EXPECT_EQ(m[2], 20);
    EXPECT_EQ(m.size(), 2u);
}

/**
 * @tc.name  : MapAllocator_UnorderedMap_ShouldWorkCorrectly
 * @tc.number: MapAllocator_Container_Test_004
 * @tc.desc  : Test that MAP_ALLOCATOR::unordered_map works with custom allocator.
 */
HWTEST_F(AllocatorTest, MapAllocatorUnorderedMap, TestSize.Level1)
{
    MAP_ALLOCATOR::unordered_map<int, int> m((Allocator<MAP_ALLOCATOR::unordered_map<int, int>::value_type>(heap)));
    m[1] = 10;
    m[2] = 20;
    EXPECT_EQ(m[1], 10);
    EXPECT_EQ(m.size(), 2u);
}

/**
 * @tc.name  : MapAllocator_UnorderedSet_ShouldDeduplicate
 * @tc.number: MapAllocator_Container_Test_005
 * @tc.desc  : Test that MAP_ALLOCATOR::unordered_set deduplicates elements.
 */
HWTEST_F(AllocatorTest, MapAllocatorUnorderedSet, TestSize.Level1)
{
    MMAP_ALLOCATOR::unordered_set<int> s((Allocator<int>(heap)));
    s.insert(1);
    s.insert(2);
    s.insert(1);
    EXPECT_EQ(s.size(), 2u);
}

/**
 * @tc.name  : Heap_AllocFree_ShouldNotLeak_WhenMultipleCycles
 * @tc.number: Heap_Alloc_Test_008
 * @tc.desc  : Test repeated alloc/free cycles do not crash or leak.
 */
HWTEST_F(AllocatorTest, MultipleAllocFreeCycles, TestSize.Level1)
{
    for (int i = 0; i < 100; i++) {
        void *ptr = heap.Allocate(16 + i * 8);
        ASSERT_NE(ptr, nullptr);
        heap.Deallocate(ptr);
    }
}

/**
 * @tc.name  : Chunk_IsChunk_ShouldReturnTrue_WhenChunkAllocatedPtr
 * @tc.number: Chunk_IsChunk_Test_001
 * @tc.desc  : Test that IsChunk returns true for pointer allocated via bucket path.
 */
HWTEST_F(AllocatorTest, ChunkAllocAndIsChunk, TestSize.Level1)
{
    void *ptr = heap.Allocate(K_MIN_BUCKET_ALLOCATION_SIZE);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(Chunk::IsChunk(ptr));
    heap.Deallocate(ptr);
}

/**
 * @tc.name  : Chunk_IsChunk_ShouldNotCrash_WhenStackPointer
 * @tc.number: Chunk_IsChunk_Test_002
 * @tc.desc  : Test that IsChunk does not crash for stack pointer input.
 */
HWTEST_F(AllocatorTest, ChunkIsChunkStackPointer, TestSize.Level1)
{
    int stackVar = 0;
    bool result = Chunk::IsChunk(&stackVar);
    (void)result;
}

/**
 * @tc.name  : Chunk_PtrToChunk_ShouldReturnValidChunk_WhenChunkPtr
 * @tc.number: Chunk_PtrToChunk_Test_001
 * @tc.desc  : Test that PtrToChunk returns a valid chunk for chunk-allocated pointer.
 */
HWTEST_F(AllocatorTest, ChunkPtrToChunk, TestSize.Level1)
{
    void *ptr = heap.Allocate(K_MIN_BUCKET_ALLOCATION_SIZE);
    ASSERT_NE(ptr, nullptr);
    Chunk *chunk = Chunk::PtrToChunk(ptr);
    ASSERT_NE(chunk, nullptr);
    heap.Deallocate(ptr);
}

/**
 * @tc.name  : MemoryMapper_MapAligned_ShouldReturnAlignedPointer_WhenPageSize
 * @tc.number: MemoryMapper_Test_001
 * @tc.desc  : Test that MapAligned returns a page-aligned pointer for page-sized allocation.
 */
HWTEST_F(AllocatorTest, MemoryMapperMapAlignedPage, TestSize.Level1)
{
    void *ptr = MemoryMapper::MapAligned(K_PAGE_SIZE, K_PAGE_SIZE);
    ASSERT_NE(ptr, nullptr);
    munmap(ptr, K_PAGE_SIZE);
}

/**
 * @tc.name  : MemoryMapper_MapAligned_ShouldReturnAlignedPointer_WhenLargeSize
 * @tc.number: MemoryMapper_Test_002
 * @tc.desc  : Test that MapAligned returns a chunk-aligned pointer for chunk-sized allocation.
 */
HWTEST_F(AllocatorTest, MemoryMapperMapAlignedLargeSize, TestSize.Level1)
{
    void *ptr = MemoryMapper::MapAligned(K_CHUNK_SIZE, K_CHUNK_SIZE);
    ASSERT_NE(ptr, nullptr);
    munmap(ptr, K_CHUNK_SIZE);
}

/**
 * @tc.name  : MemoryMapper_MapAligned_ShouldRoundUp_WhenNonPageSize
 * @tc.number: MemoryMapper_Test_003
 * @tc.desc  : Test that MapAligned rounds up non-page-aligned sizes correctly.
 */
HWTEST_F(AllocatorTest, MemoryMapperMapAlignedNonPageSize, TestSize.Level1)
{
    void *ptr = MemoryMapper::MapAligned(100, K_PAGE_SIZE);
    ASSERT_NE(ptr, nullptr);
    munmap(ptr, K_PAGE_SIZE);
}

/**
 * @tc.name  : Heap_Alloc_ShouldUseBucketPath_WhenExactMaxBucketSize
 * @tc.number: Heap_Alloc_Test_009
 * @tc.desc  : Test that allocating exactly K_MAX_BUCKET_ALLOCATION_SIZE uses bucket allocator path.
 */
HWTEST_F(AllocatorTest, HeapAllocateMaxBucketSize, TestSize.Level1)
{
    void *ptr = heap.Allocate(K_MAX_BUCKET_ALLOCATION_SIZE);
    ASSERT_NE(ptr, nullptr);
    heap.Deallocate(ptr);
}

/**
 * @tc.name  : Heap_Alloc_ShouldUseMapAllocPath_WhenJustAboveMaxBucket
 * @tc.number: Heap_Alloc_Test_010
 * @tc.desc  : Test that allocating just above K_MAX_BUCKET_ALLOCATION_SIZE uses MapAlloc path.
 */
HWTEST_F(AllocatorTest, HeapAllocateJustAboveMaxBucket, TestSize.Level1)
{
    void *ptr = heap.Allocate(K_MAX_BUCKET_ALLOCATION_SIZE + 1);
    if (ptr != nullptr) {
        heap.Deallocate(ptr);
    }
}


// ===== From performance_tracer_test.cpp =====
class PerformanceTracerTest : public ::testing::Test {};

/**
 * @tc.name  : PerformanceTracer_ShouldConstruct_WhenNameProvided
 * @tc.number: PerformanceTracer_Constructor_Test_001
 * @tc.desc  : Test that PerformanceTracer constructor works with a name.
 */
HWTEST_F(PerformanceTracerTest, ConstructorWithName, TestSize.Level1)
{
    PerformanceTracer tracer("TestTracer");
    tracer.Mark("first mark");
}

/**
 * @tc.name  : PerformanceTracer_ShouldConstruct_WhenNameIsNullptr
 * @tc.number: PerformanceTracer_Constructor_Test_002
 * @tc.desc  : Test that PerformanceTracer constructor works with nullptr name.
 */
HWTEST_F(PerformanceTracerTest, ConstructorWithoutName, TestSize.Level1)
{
    PerformanceTracer tracer(nullptr);
    tracer.Mark("mark without name");
}

/**
 * @tc.name  : PerformanceTracer_ShouldPrintName_WhenFirstMarkCalled
 * @tc.number: PerformanceTracer_Mark_Test_001
 * @tc.desc  : Test that PerformanceTracer Mark prints name on first call.
 */
HWTEST_F(PerformanceTracerTest, MarkFirstCall, TestSize.Level1)
{
    PerformanceTracer tracer("Test");
    tracer.Mark("first");
}

/**
 * @tc.name  : PerformanceTracer_ShouldPrintDuration_WhenSecondMarkCalled
 * @tc.number: PerformanceTracer_Mark_Test_002
 * @tc.desc  : Test that PerformanceTracer Mark prints duration on subsequent calls.
 */
HWTEST_F(PerformanceTracerTest, MarkSecondCall, TestSize.Level1)
{
    PerformanceTracer tracer("Test");
    tracer.Mark("first");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tracer.Mark("second");
}

/**
 * @tc.name  : PerformanceTracer_ShouldNotCrash_WhenMultipleMarksCalled
 * @tc.number: PerformanceTracer_Mark_Test_003
 * @tc.desc  : Test that PerformanceTracer Mark does not crash with multiple calls.
 */
HWTEST_F(PerformanceTracerTest, MarkMultipleCalls, TestSize.Level1)
{
    PerformanceTracer tracer("Test");
    for (int i = 0; i < 10; i++) {
        tracer.Mark("mark %d", i);
    }
}

/**
 * @tc.name  : PerformanceTracer_ShouldFormatArgs_WhenMarkWithFormatCalled
 * @tc.number: PerformanceTracer_Mark_Test_004
 * @tc.desc  : Test that PerformanceTracer Mark works with format arguments.
 */
HWTEST_F(PerformanceTracerTest, MarkWithFormatArgs, TestSize.Level1)
{
    PerformanceTracer tracer("Test");
    tracer.Mark("step %d value %d", 1, 42);
}

/**
 * @tc.name  : PerformanceTracer_ShouldConstruct_WhenDefaultConstructed
 * @tc.number: PerformanceTracer_Constructor_Test_003
 * @tc.desc  : Test that PerformanceTracer default constructor works.
 */
HWTEST_F(PerformanceTracerTest, DefaultConstructor, TestSize.Level1)
{
    PerformanceTracer tracer;
    tracer.Mark("default tracer mark");
}

/**
 * @tc.name  : PerformanceTracer_ShouldShowDuration_WhenMarkCalledAfterFirst
 * @tc.number: PerformanceTracer_Mark_Test_005
 * @tc.desc  : Test that PerformanceTracer Mark shows duration after first call.
 */
HWTEST_F(PerformanceTracerTest, MarkShowsDuration, TestSize.Level1)
{
    PerformanceTracer tracer("DurationTest");
    tracer.Mark("start");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    tracer.Mark("end");
}

/**
 * @tc.name  : PerformanceTracer_ShouldWork_WhenMarkWithNoFormatArgs
 * @tc.number: PerformanceTracer_Mark_Test_006
 * @tc.desc  : Test that PerformanceTracer Mark works with no format arguments.
 */
HWTEST_F(PerformanceTracerTest, MarkNoFormatArgs, TestSize.Level1)
{
    PerformanceTracer tracer("NoArgs");
    tracer.Mark("simple_label");
}

/**
 * @tc.name  : PerformanceTracer_ShouldWork_WhenNameNullptrWithSubsequentMarks
 * @tc.number: PerformanceTracer_Mark_Test_007
 * @tc.desc  : Test that PerformanceTracer Mark works with nullptr name on subsequent calls.
 */
HWTEST_F(PerformanceTracerTest, NullNameSubsequentCalls, TestSize.Level1)
{
    PerformanceTracer tracer(nullptr);
    tracer.Mark("first");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    tracer.Mark("second");
}


// ===== From collect_thread_test.cpp =====
class CollectThreadTest : public ::testing::Test {};

/**
 * @tc.name  : CollectThread_ShouldConstructAndDestruct_WhenNotStarted
 * @tc.number: CollectThread_Constructor_Test_001
 * @tc.desc  : Test that CollectThread constructor and destructor work without starting.
 */
HWTEST_F(CollectThreadTest, ConstructorDestructor, TestSize.Level1)
{
    auto func = []() -> int { return 0; };
    CollectThread thread(func);
}

/**
 * @tc.name  : CollectThread_ShouldStartAndJoin_WhenCalled
 * @tc.number: CollectThread_StartJoin_Test_001
 * @tc.desc  : Test that CollectThread Start and Join work correctly.
 */
HWTEST_F(CollectThreadTest, StartAndJoin, TestSize.Level1)
{
    std::atomic<int> counter{0};
    auto func = [&counter]() -> int {
        counter = 42;
        return 0;
    };
    CollectThread thread(func);
    EXPECT_TRUE(thread.Start());
    thread.Signal();
    int ret = thread.Join();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(counter.load(), 42);
}

/**
 * @tc.name  : CollectThread_ShouldReturnFunctionResult_WhenJoined
 * @tc.number: CollectThread_Join_Test_001
 * @tc.desc  : Test that CollectThread Join returns the function result.
 */
HWTEST_F(CollectThreadTest, JoinReturnsFunctionResult, TestSize.Level1)
{
    auto func = []() -> int { return 7; };
    CollectThread thread(func);
    EXPECT_TRUE(thread.Start());
    thread.Signal();
    int ret = thread.Join();
    EXPECT_EQ(ret, 7);
}

/**
 * @tc.name  : CollectThread_ShouldReturnNegativeOne_WhenJoinWithoutStart
 * @tc.number: CollectThread_Join_Test_002
 * @tc.desc  : Test that CollectThread Join returns -1 when called without Start.
 */
HWTEST_F(CollectThreadTest, JoinWithoutStart, TestSize.Level1)
{
    auto func = []() -> int { return 0; };
    CollectThread thread(func);
    EXPECT_EQ(thread.Join(), -1);
}

/**
 * @tc.name  : CollectThread_ShouldReturnNegativeOne_WhenDoubleJoin
 * @tc.number: CollectThread_Join_Test_003
 * @tc.desc  : Test that CollectThread Join returns -1 when called a second time.
 */
HWTEST_F(CollectThreadTest, DoubleJoin, TestSize.Level1)
{
    auto func = []() -> int { return 0; };
    CollectThread thread(func);
    EXPECT_TRUE(thread.Start());
    thread.Signal();
    EXPECT_EQ(thread.Join(), 0);
    EXPECT_EQ(thread.Join(), -1);
}

/**
 * @tc.name  : CollectThread_ShouldNotCrash_WhenSignalCalled
 * @tc.number: CollectThread_Signal_Test_001
 * @tc.desc  : Test that CollectThread Signal does not crash when called.
 */
HWTEST_F(CollectThreadTest, SignalNoCrash, TestSize.Level1)
{
    auto func = []() -> int { return 0; };
    CollectThread thread(func);
    thread.Start();
    thread.Signal();
    thread.Join();
}

/**
 * @tc.name  : CollectThread_ShouldReturnNegativeValue_WhenFunctionReturnsNegative
 * @tc.number: CollectThread_Join_Test_004
 * @tc.desc  : Test that CollectThread Join returns the negative value from function.
 */
HWTEST_F(CollectThreadTest, NegativeReturnValue, TestSize.Level1)
{
    auto func = []() -> int { return -1; };
    CollectThread thread(func);
    EXPECT_TRUE(thread.Start());
    thread.Signal();
    int ret = thread.Join();
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : CollectThread_ShouldNotCrash_WhenDestructedAfterStartWithoutJoin
 * @tc.number: CollectThread_Destructor_Test_001
 * @tc.desc  : Test that CollectThread destructor does not crash after start without join.
 */
HWTEST_F(CollectThreadTest, DestructorAfterStartNoJoin, TestSize.Level1)
{
    {
        auto func = []() -> int {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return 0;
        };
        CollectThread thread(func);
        EXPECT_TRUE(thread.Start());
        thread.Signal();
    }
}

/**
 * @tc.name  : CollectThread_ShouldComplete_WhenLongRunningFunction
 * @tc.number: CollectThread_StartJoin_Test_002
 * @tc.desc  : Test that CollectThread completes execution of a long running function.
 */
HWTEST_F(CollectThreadTest, LongRunningFunction, TestSize.Level1)
{
    std::atomic<bool> completed{false};
    auto func = [&completed]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        completed = true;
        return 0;
    };
    CollectThread thread(func);
    EXPECT_TRUE(thread.Start());
    thread.Signal();
    thread.Join();
    EXPECT_TRUE(completed.load());
}

/**
 * @tc.name  : CollectThread_ShouldModifySharedState_WhenFunctionExecutes
 * @tc.number: CollectThread_StartJoin_Test_003
 * @tc.desc  : Test that CollectThread function correctly modifies shared state.
 */
HWTEST_F(CollectThreadTest, ModifySharedState, TestSize.Level1)
{
    std::atomic<int> value{0};
    auto func = [&value]() -> int {
        value.fetch_add(10);
        return 0;
    };
    CollectThread thread(func);
    EXPECT_TRUE(thread.Start());
    thread.Signal();
    thread.Join();
    EXPECT_EQ(value.load(), 10);
}

// ===== From leak_info_test.cpp =====
class LeakInfoTest : public ::testing::Test {};

/**
 * @tc.name  : NodeSTL_ShouldReturnDirectly_WhenIsDirectTrue
 * @tc.number: NodeSTL_IfDirect_Test_001
 * @tc.desc  : Test that NodeSTL IfDirect returns DIRECTYLY when isDirect is true.
 */
HWTEST_F(LeakInfoTest, NodeSTL_IfDirect_True, TestSize.Level1)
{
    NodeSTL node;
    node.isDirect = true;
    EXPECT_EQ(node.IfDirect(), DIRECTYLY);
}

/**
 * @tc.name  : NodeSTL_ShouldReturnIndirectly_WhenIsDirectFalse
 * @tc.number: NodeSTL_IfDirect_Test_002
 * @tc.desc  : Test that NodeSTL IfDirect returns INDIRECTYLY when isDirect is false.
 */
HWTEST_F(LeakInfoTest, NodeSTL_IfDirect_False, TestSize.Level1)
{
    NodeSTL node;
    node.isDirect = false;
    EXPECT_EQ(node.IfDirect(), INDIRECTYLY);
}

/**
 * @tc.name  : NodeCompare_ShouldSortDirectBeforeIndirect_WhenComparing
 * @tc.number: NodeCompare_IsDirect_Test_001
 * @tc.desc  : Test that NodeCompare sorts direct nodes before indirect nodes.
 */
HWTEST_F(LeakInfoTest, NodeCompare_IsDirect, TestSize.Level1)
{
    NodeSTL a, b;
    a.isDirect = true;
    b.isDirect = false;
    a.capacity = 10;
    b.capacity = 10;
    a.refCapacity = 0;
    b.refCapacity = 0;
    NodeCompare cmp;
    EXPECT_TRUE(cmp({0x1000, a}, {0x2000, b}));
    EXPECT_FALSE(cmp({0x2000, b}, {0x1000, a}));
}

/**
 * @tc.name  : NodeCompare_ShouldSortLargerCapacityFirst_WhenBothDirect
 * @tc.number: NodeCompare_Capacity_Test_001
 * @tc.desc  : Test that NodeCompare sorts nodes with larger capacity first when both are direct.
 */
HWTEST_F(LeakInfoTest, NodeCompare_Capacity, TestSize.Level1)
{
    NodeSTL a, b;
    a.isDirect = true;
    b.isDirect = true;
    a.capacity = 100;
    b.capacity = 50;
    NodeCompare cmp;
    EXPECT_TRUE(cmp({0x1000, a}, {0x2000, b}));
}

/**
 * @tc.name  : NodeCompare_ShouldReturnFalse_WhenCapacityLess
 * @tc.number: NodeCompare_Capacity_Test_002
 * @tc.desc  : Test that NodeCompare returns false when the first node has less capacity.
 */
HWTEST_F(LeakInfoTest, NodeCompare_CapacityLess, TestSize.Level1)
{
    NodeSTL a, b;
    a.isDirect = true;
    b.isDirect = true;
    a.capacity = 50;
    b.capacity = 100;
    NodeCompare cmp;
    EXPECT_FALSE(cmp({0x1000, a}, {0x2000, b}));
}

/**
 * @tc.name  : NodeCompare_ShouldSortLargerRefCapacityFirst_WhenCapacityEqual
 * @tc.number: NodeCompare_RefCapacity_Test_001
 * @tc.desc  : Test that NodeCompare sorts nodes with larger refCapacity first when capacity is equal.
 */
HWTEST_F(LeakInfoTest, NodeCompare_RefCapacity, TestSize.Level1)
{
    NodeSTL a, b;
    a.isDirect = true;
    b.isDirect = true;
    a.capacity = 50;
    b.capacity = 50;
    a.refCapacity = 200;
    b.refCapacity = 100;
    NodeCompare cmp;
    EXPECT_TRUE(cmp({0x1000, a}, {0x2000, b}));
}

/**
 * @tc.name  : NodeCompare_ShouldReturnFalse_WhenRefCapacityLess
 * @tc.number: NodeCompare_RefCapacity_Test_002
 * @tc.desc  : Test that NodeCompare returns false when the first node has less refCapacity.
 */
HWTEST_F(LeakInfoTest, NodeCompare_RefCapacityLess, TestSize.Level1)
{
    NodeSTL a, b;
    a.isDirect = true;
    b.isDirect = true;
    a.capacity = 50;
    b.capacity = 50;
    a.refCapacity = 100;
    b.refCapacity = 200;
    NodeCompare cmp;
    EXPECT_FALSE(cmp({0x1000, a}, {0x2000, b}));
}

/**
 * @tc.name  : NodeCompare_ShouldSortByAddress_WhenCapacityAndRefCapacityEqual
 * @tc.number: NodeCompare_Address_Test_001
 * @tc.desc  : Test that NodeCompare uses address as tie-breaker when capacity and refCapacity are equal.
 */
HWTEST_F(LeakInfoTest, NodeCompare_Address, TestSize.Level1)
{
    NodeSTL a, b;
    a.isDirect = true;
    b.isDirect = true;
    a.capacity = 50;
    b.capacity = 50;
    a.refCapacity = 100;
    b.refCapacity = 100;
    NodeCompare cmp;
    EXPECT_TRUE(cmp({0x1000, a}, {0x2000, b}));
    EXPECT_FALSE(cmp({0x2000, b}, {0x1000, a}));
}

/**
 * @tc.name  : NodeCompare_ShouldReturnFalse_WhenNodesAreEqual
 * @tc.number: NodeCompare_Equal_Test_001
 * @tc.desc  : Test that NodeCompare returns false when two nodes have the same address.
 */
HWTEST_F(LeakInfoTest, NodeCompare_EqualNodes, TestSize.Level1)
{
    NodeSTL a, b;
    a.isDirect = true;
    b.isDirect = true;
    a.capacity = 50;
    b.capacity = 50;
    a.refCapacity = 100;
    b.refCapacity = 100;
    NodeCompare cmp;
    EXPECT_FALSE(cmp({0x1000, a}, {0x1000, b}));
}

/**
 * @tc.name  : NodeCompare_ShouldSortLargerCapacityFirst_WhenBothIndirect
 * @tc.number: NodeCompare_Capacity_Test_003
 * @tc.desc  : Test that NodeCompare sorts by capacity when both nodes are indirect.
 */
HWTEST_F(LeakInfoTest, NodeCompare_IndirectVsIndirect, TestSize.Level1)
{
    NodeSTL a, b;
    a.isDirect = false;
    b.isDirect = false;
    a.capacity = 200;
    b.capacity = 100;
    NodeCompare cmp;
    EXPECT_TRUE(cmp({0x1000, a}, {0x2000, b}));
}

/**
 * @tc.name  : LeakInfo_ShouldHaveEmptyGraph_WhenDefaultConstructed
 * @tc.number: LeakInfo_Constructor_Test_001
 * @tc.desc  : Test that LeakInfo default constructor produces an empty leak graph.
 */
HWTEST_F(LeakInfoTest, ConstructorDestructor, TestSize.Level1)
{
    LeakInfo info;
    EXPECT_TRUE(info.leakGraph_.empty());
}

/**
 * @tc.name  : LeakInfo_ShouldSetIndirect_WhenNodeIsReferenced
 * @tc.number: LeakInfo_UpdateDirect_Test_001
 * @tc.desc  : Test that LeakInfo UpdateDirect sets isDirect to false for referenced nodes.
 */
HWTEST_F(LeakInfoTest, UpdateDirect_SetsIndirect, TestSize.Level1)
{
    LeakInfo info;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = 0x2000;

    NodeSTL node2;
    node2.isDirect = true;
    node2.capacity = 32;

    info.leakGraph_[0x1000] = node1;
    info.leakGraph_[0x2000] = node2;

    info.UpdateDirect();
    EXPECT_TRUE(info.leakGraph_[0x1000].isDirect);
    EXPECT_FALSE(info.leakGraph_[0x2000].isDirect);
}

/**
 * @tc.name  : LeakInfo_ShouldKeepDirect_WhenNoReferences
 * @tc.number: LeakInfo_UpdateDirect_Test_002
 * @tc.desc  : Test that LeakInfo UpdateDirect keeps isDirect true when node has no references.
 */
HWTEST_F(LeakInfoTest, UpdateDirect_NoReferences, TestSize.Level1)
{
    LeakInfo info;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    info.leakGraph_[0x1000] = node1;

    info.UpdateDirect();
    EXPECT_TRUE(info.leakGraph_[0x1000].isDirect);
}

/**
 * @tc.name  : LeakInfo_ShouldKeepDirect_WhenReferenceToNonExistent
 * @tc.number: LeakInfo_UpdateDirect_Test_003
 * @tc.desc  : Test that LeakInfo UpdateDirect keeps isDirect true when reference points to non-existent node.
 */
HWTEST_F(LeakInfoTest, UpdateDirect_ReferenceToNonExistent, TestSize.Level1)
{
    LeakInfo info;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = 0x9999;

    info.leakGraph_[0x1000] = node1;
    info.UpdateDirect();
    EXPECT_TRUE(info.leakGraph_[0x1000].isDirect);
}

/**
 * @tc.name  : LeakInfo_ShouldUpdateChain_WhenReferencesAreChained
 * @tc.number: LeakInfo_UpdateDirect_Test_004
 * @tc.desc  : Test that LeakInfo UpdateDirect sets indirect for all referenced nodes in a chain.
 */
HWTEST_F(LeakInfoTest, UpdateDirect_ChainReferences, TestSize.Level1)
{
    LeakInfo info;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = 0x2000;

    NodeSTL node2;
    node2.isDirect = true;
    node2.capacity = 32;
    node2.references[0] = 0x3000;

    NodeSTL node3;
    node3.isDirect = true;
    node3.capacity = 16;

    info.leakGraph_[0x1000] = node1;
    info.leakGraph_[0x2000] = node2;
    info.leakGraph_[0x3000] = node3;

    info.UpdateDirect();
    EXPECT_TRUE(info.leakGraph_[0x1000].isDirect);
    EXPECT_FALSE(info.leakGraph_[0x2000].isDirect);
    EXPECT_FALSE(info.leakGraph_[0x3000].isDirect);
}

/**
 * @tc.name  : LeakInfo_ShouldUpdateRefCapacity_WhenReferencesExist
 * @tc.number: LeakInfo_UpdateRefCapacity_Test_001
 * @tc.desc  : Test that LeakInfo UpdateRefCapacity calculates refCapacity from referenced nodes.
 */
HWTEST_F(LeakInfoTest, UpdateRefCapacity, TestSize.Level1)
{
    LeakInfo info;
    NodeSTL node1;
    node1.capacity = 64;
    node1.references[0] = 0x2000;
    node1.refCapacity = 0;

    NodeSTL node2;
    node2.capacity = 32;
    info.leakGraph_[0x1000] = node1;
    info.leakGraph_[0x2000] = node2;

    info.UpdateRefCapacity();
    EXPECT_EQ(info.leakGraph_[0x1000].refCapacity, 32u);
}

/**
 * @tc.name  : LeakInfo_ShouldKeepZeroRefCapacity_WhenReferenceToNonExistent
 * @tc.number: LeakInfo_UpdateRefCapacity_Test_002
 * @tc.desc  : Test that LeakInfo UpdateRefCapacity keeps refCapacity zero when reference points to non-existent node.
 */
HWTEST_F(LeakInfoTest, UpdateRefCapacity_ReferenceToNonExistent, TestSize.Level1)
{
    LeakInfo info;
    NodeSTL node1;
    node1.capacity = 64;
    node1.references[0] = 0x9999;
    node1.refCapacity = 0;

    info.leakGraph_[0x1000] = node1;
    info.UpdateRefCapacity();
    EXPECT_EQ(info.leakGraph_[0x1000].refCapacity, 0u);
}

/**
 * @tc.name  : LeakInfo_ShouldSumRefCapacity_WhenMultipleReferences
 * @tc.number: LeakInfo_UpdateRefCapacity_Test_003
 * @tc.desc  : Test that LeakInfo UpdateRefCapacity sums capacities from multiple referenced nodes.
 */
HWTEST_F(LeakInfoTest, UpdateRefCapacity_MultipleReferences, TestSize.Level1)
{
    LeakInfo info;
    NodeSTL node1;
    node1.capacity = 64;
    node1.references[0] = 0x2000;
    node1.references[8] = 0x3000;
    node1.refCapacity = 0;

    NodeSTL node2;
    node2.capacity = 32;

    NodeSTL node3;
    node3.capacity = 16;

    info.leakGraph_[0x1000] = node1;
    info.leakGraph_[0x2000] = node2;
    info.leakGraph_[0x3000] = node3;

    info.UpdateRefCapacity();
    EXPECT_EQ(info.leakGraph_[0x1000].refCapacity, 48u);
}

/**
 * @tc.name  : LeakInfo_ShouldSortByCapacity_WhenMakeSortGraphCalled
 * @tc.number: LeakInfo_MakeSortGraph_Test_001
 * @tc.desc  : Test that LeakInfo MakeSortGraph sorts nodes by capacity descending.
 */
HWTEST_F(LeakInfoTest, MakeSortGraph, TestSize.Level1)
{
    LeakInfo info;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 100;

    NodeSTL node2;
    node2.isDirect = true;
    node2.capacity = 50;

    info.leakGraph_[0x1000] = node1;
    info.leakGraph_[0x2000] = node2;

    info.MakeSortGraph();
    EXPECT_EQ(info.sortedLeakGraph_.size(), 2u);
    EXPECT_EQ(info.sortedLeakGraph_.begin()->second.capacity, 100u);
}

/**
 * @tc.name  : LeakInfo_ShouldReturnEmpty_WhenMakeSortGraphOnEmpty
 * @tc.number: LeakInfo_MakeSortGraph_Test_002
 * @tc.desc  : Test that LeakInfo MakeSortGraph produces empty result on empty graph.
 */
HWTEST_F(LeakInfoTest, MakeSortGraphEmpty, TestSize.Level1)
{
    LeakInfo info;
    info.MakeSortGraph();
    EXPECT_TRUE(info.sortedLeakGraph_.empty());
}

/**
 * @tc.name  : LeakInfo_ShouldReturnVectorString_WhenToVectorCalled
 * @tc.number: LeakInfo_ToVector_Test_001
 * @tc.desc  : Test that LeakInfo ToVector returns the expected vector format string.
 */
HWTEST_F(LeakInfoTest, ToVector, TestSize.Level1)
{
    LeakInfo info;
    std::string output;
    EXPECT_TRUE(info.ToVector(output));
    EXPECT_EQ(output, "this is vector.");
}

/**
 * @tc.name  : LeakInfo_ShouldOutputTxt_WhenBasicLeakGraph
 * @tc.number: LeakInfo_ToTxt_Test_001
 * @tc.desc  : Test that LeakInfo ToTxt outputs leak information for basic graph.
 */
HWTEST_F(LeakInfoTest, ToTxt_Basic, TestSize.Level1)
{
    LeakInfo info;
    char buf[64] = {0};
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 32;
    node1.pc = 0;
    info.leakGraph_[reinterpret_cast<uintptr_t>(buf)] = node1;
    std::string output;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(1000)
    };
    EXPECT_TRUE(info.ToTxt(output, timeInfo));
    EXPECT_NE(output.find("bytes leak"), std::string::npos);
}

/**
 * @tc.name  : LeakInfo_ShouldShowReferencing_WhenToTxtWithReferences
 * @tc.number: LeakInfo_ToTxt_Test_002
 * @tc.desc  : Test that LeakInfo ToTxt shows referencing information for nodes with references.
 */
HWTEST_F(LeakInfoTest, ToTxt_WithReferences, TestSize.Level1)
{
    LeakInfo info;
    char buf1[64] = {0};
    char buf2[64] = {0};
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = reinterpret_cast<uintptr_t>(buf2);
    NodeSTL node2;
    node2.isDirect = true;
    node2.capacity = 32;
    info.leakGraph_[reinterpret_cast<uintptr_t>(buf1)] = node1;
    info.leakGraph_[reinterpret_cast<uintptr_t>(buf2)] = node2;
    std::string output;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(500)
    };
    EXPECT_TRUE(info.ToTxt(output, timeInfo));
    EXPECT_NE(output.find("referencing"), std::string::npos);
}

/**
 * @tc.name  : LeakInfo_ShouldShowTypeAndName_WhenToTxtWithTypeAndName
 * @tc.number: LeakInfo_ToTxt_Test_003
 * @tc.desc  : Test that LeakInfo ToTxt shows Variable Type, Variable Name, and PC when available.
 */
HWTEST_F(LeakInfoTest, ToTxt_WithTypeAndName, TestSize.Level1)
{
    LeakInfo info;
    char buf[64] = {0};
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 16;
    node1.type = "int*";
    node1.name = "myPtr";
    node1.pc = 0xABCD;
    info.leakGraph_[reinterpret_cast<uintptr_t>(buf)] = node1;
    std::string output;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_TRUE(info.ToTxt(output, timeInfo));
    EXPECT_NE(output.find("Variable Type"), std::string::npos);
    EXPECT_NE(output.find("Variable Name"), std::string::npos);
    EXPECT_NE(output.find("PC:"), std::string::npos);
}

/**
 * @tc.name  : LeakInfo_ShouldNotShowTypeAndName_WhenUnknown
 * @tc.number: LeakInfo_ToTxt_Test_004
 * @tc.desc  : Test that LeakInfo ToTxt does not show Variable Type, Variable Name, and PC when unknown.
 */
HWTEST_F(LeakInfoTest, ToTxt_UnknownTypeNotPrinted, TestSize.Level1)
{
    LeakInfo info;
    char buf[64] = {0};
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 16;
    node1.type = UNKNOWN;
    node1.name = UNKNOWN;
    node1.pc = 0;
    info.leakGraph_[reinterpret_cast<uintptr_t>(buf)] = node1;
    std::string output;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_TRUE(info.ToTxt(output, timeInfo));
    EXPECT_EQ(output.find("Variable Type"), std::string::npos);
    EXPECT_EQ(output.find("Variable Name"), std::string::npos);
    EXPECT_EQ(output.find("PC:"), std::string::npos);
}

/**
 * @tc.name  : LeakInfo_ShouldShowFirstAndLastMemory_WhenLargeCapacity
 * @tc.number: LeakInfo_ToTxt_Test_005
 * @tc.desc  : Test that LeakInfo ToTxt shows first 32 and last 32 memory for large capacity nodes.
 */
HWTEST_F(LeakInfoTest, ToTxt_LargeCapacity, TestSize.Level1)
{
    LeakInfo info;
    char buf[128] = {0};
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 128;
    info.leakGraph_[reinterpret_cast<uintptr_t>(buf)] = node1;
    std::string output;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_TRUE(info.ToTxt(output, timeInfo));
    EXPECT_NE(output.find("first 32 memory"), std::string::npos);
    EXPECT_NE(output.find("last 32 memory"), std::string::npos);
}

/**
 * @tc.name  : LeakInfo_ShouldShowMemoryDump_WhenSmallCapacity
 * @tc.number: LeakInfo_ToTxt_Test_006
 * @tc.desc  : Test that LeakInfo ToTxt shows memory dump for small capacity nodes.
 */
HWTEST_F(LeakInfoTest, ToTxt_SmallCapacityMemoryDump, TestSize.Level1)
{
    LeakInfo info;
    char buf[64] = {0};
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 32;
    info.leakGraph_[reinterpret_cast<uintptr_t>(buf)] = node1;
    std::string output;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_TRUE(info.ToTxt(output, timeInfo));
    EXPECT_NE(output.find("memory:"), std::string::npos);
}

/**
 * @tc.name  : LeakInfo_ShouldReturnFalse_WhenToHeapSnapshotWithEmptyGraph
 * @tc.number: LeakInfo_ToHeapSnapshot_Test_001
 * @tc.desc  : Test that LeakInfo ToHeapSnapshot returns false when graph is empty.
 */
HWTEST_F(LeakInfoTest, ToHeapSnapshot_EmptyGraph, TestSize.Level1)
{
    LeakInfo info;
    std::string output;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_FALSE(info.ToHeapSnapshot(output, timeInfo));
}

/**
 * @tc.name  : LeakInfo_ShouldReturnTrue_WhenToHeapSnapshotWithData
 * @tc.number: LeakInfo_ToHeapSnapshot_Test_002
 * @tc.desc  : Test that LeakInfo ToHeapSnapshot returns true and produces output when graph has data.
 */
HWTEST_F(LeakInfoTest, ToHeapSnapshot_WithData, TestSize.Level1)
{
    LeakInfo info;
    char buf[64] = {0};
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    info.leakGraph_[reinterpret_cast<uintptr_t>(buf)] = node1;
    std::string output;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_TRUE(info.ToHeapSnapshot(output, timeInfo));
    EXPECT_FALSE(output.empty());
}

/**
 * @tc.name  : LeakInfo_ShouldShowCostTime_WhenToTxtOutput
 * @tc.number: LeakInfo_ToTxt_Test_007
 * @tc.desc  : Test that LeakInfo ToTxt outputs cost time information.
 */
HWTEST_F(LeakInfoTest, ToTxt_CostTime, TestSize.Level1)
{
    LeakInfo info;
    char buf[64] = {0};
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 16;
    info.leakGraph_[reinterpret_cast<uintptr_t>(buf)] = node1;
    std::string output;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(5000)
    };
    EXPECT_TRUE(info.ToTxt(output, timeInfo));
    EXPECT_NE(output.find("cost"), std::string::npos);
    EXPECT_NE(output.find("ms"), std::string::npos);
}


// ===== From memleak_snapshot_test.cpp =====
class SnapshotTest : public ::testing::Test {};

/**
 * @tc.name  : Node_ShouldReturnCorrectValues_WhenConstructed
 * @tc.number: Node_Constructor_Test_001
 * @tc.desc  : Test that Node constructor and getters return correct values.
 */
HWTEST_F(SnapshotTest, Node_ConstructorAndGetters, TestSize.Level1)
{
    Node node(1, NodeType::NATIVELEAK_D, 0, 0x1000, 64, 0xABCD);
    EXPECT_EQ(node.GetId(), 1u);
    EXPECT_EQ(node.GetType(), NodeType::NATIVELEAK_D);
    EXPECT_EQ(node.GetNameIndex(), 0u);
    EXPECT_EQ(node.GetAddress(), 0x1000u);
    EXPECT_EQ(node.GetSelfSize(), 64u);
    EXPECT_EQ(node.GetMallocPc(), 0xABCDu);
    EXPECT_EQ(node.GetEdgeCount(), 0u);
}

/**
 * @tc.name  : Node_ShouldUpdateType_WhenSetTypeCalled
 * @tc.number: Node_SetType_Test_001
 * @tc.desc  : Test that Node SetType updates the node type.
 */
HWTEST_F(SnapshotTest, Node_SetType, TestSize.Level1)
{
    Node node(1, NodeType::NATIVELEAK_D, 0, 0x1000, 64, 0);
    node.SetType(NodeType::NATIVELEAK_I);
    EXPECT_EQ(node.GetType(), NodeType::NATIVELEAK_I);
}

/**
 * @tc.name  : Node_ShouldUpdateNameIndex_WhenSetNameIndexCalled
 * @tc.number: Node_SetNameIndex_Test_001
 * @tc.desc  : Test that Node SetNameIndex updates the name index.
 */
HWTEST_F(SnapshotTest, Node_SetNameIndex, TestSize.Level1)
{
    Node node(1, NodeType::NATIVELEAK_D, 0, 0x1000, 64, 0);
    node.SetNameIndex(5);
    EXPECT_EQ(node.GetNameIndex(), 5u);
}

/**
 * @tc.name  : Node_ShouldUpdateSelfSize_WhenSetSelfSizeCalled
 * @tc.number: Node_SetSelfSize_Test_001
 * @tc.desc  : Test that Node SetSelfSize updates the self size.
 */
HWTEST_F(SnapshotTest, Node_SetSelfSize, TestSize.Level1)
{
    Node node(1, NodeType::NATIVELEAK_D, 0, 0x1000, 64, 0);
    node.SetSelfSize(128);
    EXPECT_EQ(node.GetSelfSize(), 128u);
}

/**
 * @tc.name  : Node_ShouldIncrementEdgeCount_WhenIncEdgeCountCalled
 * @tc.number: Node_EdgeCount_Test_001
 * @tc.desc  : Test that Node IncEdgeCount increments the edge count.
 */
HWTEST_F(SnapshotTest, Node_IncEdgeCount, TestSize.Level1)
{
    Node node(1, NodeType::NATIVELEAK_D, 0, 0x1000, 64, 0);
    node.IncEdgeCount();
    node.IncEdgeCount();
    EXPECT_EQ(node.GetEdgeCount(), 2u);
}

/**
 * @tc.name  : Node_ShouldResetEdgeCount_WhenResetEdgeCountCalled
 * @tc.number: Node_EdgeCount_Test_002
 * @tc.desc  : Test that Node ResetEdgeCount resets the edge count to zero.
 */
HWTEST_F(SnapshotTest, Node_ResetEdgeCount, TestSize.Level1)
{
    Node node(1, NodeType::NATIVELEAK_D, 0, 0x1000, 64, 0);
    node.IncEdgeCount();
    node.ResetEdgeCount();
    EXPECT_EQ(node.GetEdgeCount(), 0u);
}

/**
 * @tc.name  : Edge_ShouldReturnCorrectValues_WhenConstructed
 * @tc.number: Edge_Constructor_Test_001
 * @tc.desc  : Test that Edge constructor and getters return correct values.
 */
HWTEST_F(SnapshotTest, Edge_ConstructorAndGetters, TestSize.Level1)
{
    Edge edge(EdgeType::REFERENCE, 0, 1, 2, 8);
    EXPECT_EQ(edge.GetType(), EdgeType::REFERENCE);
    EXPECT_EQ(edge.GetFromIndex(), 0u);
    EXPECT_EQ(edge.GetToIndex(), 1u);
    EXPECT_EQ(edge.GetNameOrIndex(), 2u);
    EXPECT_EQ(edge.GetOffset(), 8u);
}

/**
 * @tc.name  : HeapSnapshot_ShouldReturnZero_WhenNoNodesOrEdges
 * @tc.number: HeapSnapshot_Count_Test_001
 * @tc.desc  : Test that HeapSnapshot GetNodeCount and GetEdgeCount return zero initially.
 */
HWTEST_F(SnapshotTest, HeapSnapshot_GetNodeCountInitiallyZero, TestSize.Level1)
{
    LeakGraphSTL graph;
    MemLeakSnapshot snapshot(graph);
    EXPECT_EQ(snapshot.GetNodeCount(), 0u);
    EXPECT_EQ(snapshot.GetEdgeCount(), 0u);
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldReturnFalse_WhenGenerateLeakGraphOnEmpty
 * @tc.number: MemLeakSnapshot_Generate_Test_001
 * @tc.desc  : Test that MemLeakSnapshot GenerateLeakGraph returns false when graph is empty.
 */
HWTEST_F(SnapshotTest, GenerateLeakGraph_Empty, TestSize.Level1)
{
    LeakGraphSTL graph;
    MemLeakSnapshot snapshot(graph);
    EXPECT_FALSE(snapshot.GenerateLeakGraph());
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldReturnTrue_WhenGenerateLeakGraphWithSingleNode
 * @tc.number: MemLeakSnapshot_Generate_Test_002
 * @tc.desc  : Test that MemLeakSnapshot GenerateLeakGraph succeeds with a single node.
 */
HWTEST_F(SnapshotTest, GenerateLeakGraph_SingleNode, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node;
    node.isDirect = true;
    node.capacity = 64;
    graph[0x1000] = node;
    MemLeakSnapshot snapshot(graph);
    EXPECT_TRUE(snapshot.GenerateLeakGraph());
    EXPECT_EQ(snapshot.GetNodeCount(), 1u);
    EXPECT_EQ(snapshot.GetEdgeCount(), 0u);
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldCreateEdge_WhenGenerateLeakGraphWithConnectedNodes
 * @tc.number: MemLeakSnapshot_Generate_Test_003
 * @tc.desc  : Test that MemLeakSnapshot GenerateLeakGraph creates edge for two connected nodes.
 */
HWTEST_F(SnapshotTest, GenerateLeakGraph_TwoConnectedNodes, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = 0x2000;
    NodeSTL node2;
    node2.isDirect = false;
    node2.capacity = 32;
    graph[0x1000] = node1;
    graph[0x2000] = node2;
    MemLeakSnapshot snapshot(graph);
    EXPECT_TRUE(snapshot.GenerateLeakGraph());
    EXPECT_EQ(snapshot.GetNodeCount(), 2u);
    EXPECT_EQ(snapshot.GetEdgeCount(), 1u);
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldSkipEdge_WhenReferenceToNonExistent
 * @tc.number: MemLeakSnapshot_Generate_Test_004
 * @tc.desc  : Test that MemLeakSnapshot GenerateLeakGraph skips edge for reference to non-existent node.
 */
HWTEST_F(SnapshotTest, GenerateLeakGraph_RefToNonExistent, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = 0x9999;
    graph[0x1000] = node1;
    MemLeakSnapshot snapshot(graph);
    EXPECT_TRUE(snapshot.GenerateLeakGraph());
    EXPECT_EQ(snapshot.GetNodeCount(), 1u);
    EXPECT_EQ(snapshot.GetEdgeCount(), 0u);
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldSetLeakGraph_WhenSetLeakGraphCalled
 * @tc.number: MemLeakSnapshot_SetLeakGraph_Test_001
 * @tc.desc  : Test that MemLeakSnapshot SetLeakGraph sets the leak graph correctly.
 */
HWTEST_F(SnapshotTest, SetLeakGraph, TestSize.Level1)
{
    LeakGraphSTL graph1;
    LeakGraphSTL graph2;
    NodeSTL node;
    node.isDirect = true;
    node.capacity = 32;
    graph2[0x5000] = node;
    MemLeakSnapshot snapshot(graph1);
    snapshot.SetLeakGraph(graph2);
    EXPECT_EQ(snapshot.GetLeakGraph().size(), 1u);
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldAssignCorrectTypes_WhenDirectVsIndirect
 * @tc.number: MemLeakSnapshot_Generate_Test_005
 * @tc.desc  : Test that MemLeakSnapshot assigns NATIVELEAK_D and NATIVELEAK_I types correctly.
 */
HWTEST_F(SnapshotTest, DirectVsIndirectNodes, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL directNode;
    directNode.isDirect = true;
    directNode.capacity = 64;
    NodeSTL indirectNode;
    indirectNode.isDirect = false;
    indirectNode.capacity = 32;
    graph[0x1000] = directNode;
    graph[0x2000] = indirectNode;
    MemLeakSnapshot snapshot(graph);
    EXPECT_TRUE(snapshot.GenerateLeakGraph());
    EXPECT_EQ(snapshot.GetNode(0).GetType(), NodeType::NATIVELEAK_D);
    EXPECT_EQ(snapshot.GetNode(1).GetType(), NodeType::NATIVELEAK_I);
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldCreateMultipleEdges_WhenNodeHasMultipleReferences
 * @tc.number: MemLeakSnapshot_Generate_Test_006
 * @tc.desc  : Test that MemLeakSnapshot GenerateLeakGraph creates multiple edges from one node.
 */
HWTEST_F(SnapshotTest, GenerateLeakGraph_MultipleEdgesOneNode, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = 0x2000;
    node1.references[8] = 0x3000;
    NodeSTL node2;
    node2.isDirect = false;
    node2.capacity = 32;
    NodeSTL node3;
    node3.isDirect = false;
    node3.capacity = 16;
    graph[0x1000] = node1;
    graph[0x2000] = node2;
    graph[0x3000] = node3;
    MemLeakSnapshot snapshot(graph);
    EXPECT_TRUE(snapshot.GenerateLeakGraph());
    EXPECT_EQ(snapshot.GetEdgeCount(), 2u);
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldSetEdgeCount_WhenLeakGraphGenerated
 * @tc.number: MemLeakSnapshot_Generate_Test_007
 * @tc.desc  : Test that MemLeakSnapshot sets node edge count after graph generation.
 */
HWTEST_F(SnapshotTest, GenerateLeakGraph_NodeEdgeCount, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = 0x2000;
    NodeSTL node2;
    node2.isDirect = false;
    node2.capacity = 32;
    graph[0x1000] = node1;
    graph[0x2000] = node2;
    MemLeakSnapshot snapshot(graph);
    snapshot.GenerateLeakGraph();
    EXPECT_EQ(snapshot.GetNode(0).GetEdgeCount(), 1u);
    EXPECT_EQ(snapshot.GetNode(1).GetEdgeCount(), 0u);
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldReturnCollections_WhenLeakGraphGenerated
 * @tc.number: MemLeakSnapshot_Get_Test_001
 * @tc.desc  : Test that MemLeakSnapshot GetNodes, GetEdges, and GetStrings return correct collections.
 */
HWTEST_F(SnapshotTest, GetNodesGetEdgesGetStrings, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = 0x2000;
    NodeSTL node2;
    node2.isDirect = false;
    node2.capacity = 32;
    graph[0x1000] = node1;
    graph[0x2000] = node2;
    MemLeakSnapshot snapshot(graph);
    snapshot.GenerateLeakGraph();
    EXPECT_EQ(snapshot.GetNodes().size(), 2u);
    EXPECT_EQ(snapshot.GetEdges().size(), 1u);
    EXPECT_GE(snapshot.GetStrings().size(), 1u);
}

/**
 * @tc.name  : JSONSerializer_ShouldReturnFalse_WhenEmptySnapshot
 * @tc.number: JSONSerializer_Serialize_Test_001
 * @tc.desc  : Test that MemLeakSnapshotJSONSerializer Serialize returns false for empty snapshot.
 */
HWTEST_F(SnapshotTest, JSONSerialize_Empty, TestSize.Level1)
{
    LeakGraphSTL emptyGraph;
    MemLeakSnapshot snapshot(emptyGraph);
    std::string out;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_FALSE(MemLeakSnapshotJSONSerializer::Serialize(snapshot, out, timeInfo));
}

/**
 * @tc.name  : JSONSerializer_ShouldSerializeCorrectly_WhenSnapshotHasData
 * @tc.number: JSONSerializer_Serialize_Test_002
 * @tc.desc  : Test that MemLeakSnapshotJSONSerializer Serialize produces valid JSON with data.
 */
HWTEST_F(SnapshotTest, JSONSerialize_WithData, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    graph[0x1000] = node1;
    MemLeakSnapshot snapshot(graph);
    snapshot.GenerateLeakGraph();
    std::string out;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_TRUE(MemLeakSnapshotJSONSerializer::Serialize(snapshot, out, timeInfo));
    EXPECT_NE(out.find("\"nodes\":"), std::string::npos);
    EXPECT_NE(out.find("\"edges\":"), std::string::npos);
    EXPECT_NE(out.find("\"strings\":"), std::string::npos);
}

/**
 * @tc.name  : JSONSerializer_ShouldIncludeHeader_WhenSerialized
 * @tc.number: JSONSerializer_Serialize_Test_003
 * @tc.desc  : Test that MemLeakSnapshotJSONSerializer includes snapshot header in output.
 */
HWTEST_F(SnapshotTest, JSONSerialize_SnapshotHeader, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    graph[0x1000] = node1;
    MemLeakSnapshot snapshot(graph);
    snapshot.GenerateLeakGraph();
    std::string out;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(100)
    };
    EXPECT_TRUE(MemLeakSnapshotJSONSerializer::Serialize(snapshot, out, timeInfo));
    EXPECT_NE(out.find("\"snapshot\":"), std::string::npos);
    EXPECT_NE(out.find("\"node_count\":"), std::string::npos);
    EXPECT_NE(out.find("\"edge_count\":"), std::string::npos);
}

/**
 * @tc.name  : JSONSerializer_ShouldEscapeSpecialChars_WhenSerializing
 * @tc.number: JSONSerializer_Serialize_Test_004
 * @tc.desc  : Test that MemLeakSnapshotJSONSerializer escapes special characters in strings.
 */
HWTEST_F(SnapshotTest, JSONSerialize_EscapesSpecialChars, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 16;
    node1.name = "test\"string";
    graph[0x1000] = node1;
    MemLeakSnapshot snapshot(graph);
    snapshot.GenerateLeakGraph();
    std::string out;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_TRUE(MemLeakSnapshotJSONSerializer::Serialize(snapshot, out, timeInfo));
    EXPECT_NE(out.find("\\\""), std::string::npos);
}

/**
 * @tc.name  : JSONSerializer_ShouldSerializeMultipleEdges_WhenMultipleNodes
 * @tc.number: JSONSerializer_Serialize_Test_005
 * @tc.desc  : Test that MemLeakSnapshotJSONSerializer correctly serializes multiple edges and nodes.
 */
HWTEST_F(SnapshotTest, JSONSerialize_MultipleEdgesAndNodes, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    node1.references[0] = 0x2000;
    node1.references[8] = 0x3000;
    NodeSTL node2;
    node2.isDirect = false;
    node2.capacity = 32;
    NodeSTL node3;
    node3.isDirect = false;
    node3.capacity = 16;
    graph[0x1000] = node1;
    graph[0x2000] = node2;
    graph[0x3000] = node3;
    MemLeakSnapshot snapshot(graph);
    snapshot.GenerateLeakGraph();
    std::string out;
    MemLeakTimeInfo timeInfo = {
        std::chrono::high_resolution_clock::now(),
        std::chrono::high_resolution_clock::now(),
        std::chrono::microseconds(0)
    };
    EXPECT_TRUE(MemLeakSnapshotJSONSerializer::Serialize(snapshot, out, timeInfo));
    EXPECT_NE(out.find("\"edge_count\":2"), std::string::npos);
}

/**
 * @tc.name  : MemLeakSnapshot_ShouldRegenerate_WhenGenerateLeakGraphCalledTwice
 * @tc.number: MemLeakSnapshot_Generate_Test_008
 * @tc.desc  : Test that MemLeakSnapshot can regenerate the leak graph when called again.
 */
HWTEST_F(SnapshotTest, GenerateLeakGraph_ClearAndRegenerate, TestSize.Level1)
{
    LeakGraphSTL graph;
    NodeSTL node1;
    node1.isDirect = true;
    node1.capacity = 64;
    graph[0x1000] = node1;
    MemLeakSnapshot snapshot(graph);
    EXPECT_TRUE(snapshot.GenerateLeakGraph());
    EXPECT_EQ(snapshot.GetNodeCount(), 1u);
    // Regenerate should also work
    EXPECT_TRUE(snapshot.GenerateLeakGraph());
    EXPECT_EQ(snapshot.GetNodeCount(), 1u);
}

/**
 * @tc.name  : Node_ShouldHaveNATIVELEAK_D_WhenDefaultType
 * @tc.number: Node_DefaultType_Test_001
 * @tc.desc  : Test that Node default type is NATIVELEAK_D.
 */
HWTEST_F(SnapshotTest, Node_DefaultType, TestSize.Level1)
{
    Node node(0, NodeType::NATIVELEAK_D, 0, 0, 0, 0);
    EXPECT_EQ(node.GetType(), NodeType::NATIVELEAK_D);
}

/**
 * @tc.name  : Edge_ShouldHaveREFERENCE_WhenDefaultType
 * @tc.number: Edge_DefaultType_Test_001
 * @tc.desc  : Test that Edge default type is REFERENCE.
 */
HWTEST_F(SnapshotTest, Edge_DefaultType, TestSize.Level1)
{
    Edge edge(EdgeType::REFERENCE, 0, 0, 0, 0);
    EXPECT_EQ(edge.GetType(), EdgeType::REFERENCE);
}


// ===== From process_pipe_test.cpp =====
class ProcessPipeTest : public ::testing::Test {};

/**
 * @tc.name  : ProcessPipe_ShouldOpenPipe_WhenConstructed
 * @tc.number: ProcessPipe_Constructor_Test_001
 * @tc.desc  : Test that ProcessPipe constructor opens pipe.
 */
HWTEST_F(ProcessPipeTest, ConstructorOpensPipe, TestSize.Level1)
{
    ProcessPipe pipe;
    EXPECT_TRUE(pipe.IsOpened());
}

/**
 * @tc.name  : ProcessPipe_ShouldClosePipe_WhenDestructed
 * @tc.number: ProcessPipe_Destructor_Test_001
 * @tc.desc  : Test that ProcessPipe destructor closes pipe.
 */
HWTEST_F(ProcessPipeTest, DestructorClosesPipe, TestSize.Level1)
{
    {
        ProcessPipe pipe;
        EXPECT_TRUE(pipe.IsOpened());
    }
}

/**
 * @tc.name  : ProcessPipe_ShouldReturnTrue_WhenPipeIsOpened
 * @tc.number: ProcessPipe_IsOpened_Test_001
 * @tc.desc  : Test that ProcessPipe IsOpened returns true when pipe is open.
 */
HWTEST_F(ProcessPipeTest, IsOpened, TestSize.Level1)
{
    ProcessPipe pipe;
    EXPECT_TRUE(pipe.IsOpened());
}

/**
 * @tc.name  : ProcessPipe_ShouldReturnFalse_WhenFdSetToInvalid
 * @tc.number: ProcessPipe_IsOpened_Test_002
 * @tc.desc  : Test that ProcessPipe IsOpened returns false after SetFd(-1, -1).
 */
HWTEST_F(ProcessPipeTest, IsOpenedFalseAfterSetInvalidFd, TestSize.Level1)
{
    ProcessPipe pipe;
    pipe.SetFd(-1, -1);
    EXPECT_FALSE(pipe.IsOpened());
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveInt_WhenDataTransferred
 * @tc.number: ProcessPipe_SendReceive_Test_001
 * @tc.desc  : Test that ProcessPipe SendData and ReceiveData work for int type.
 */
HWTEST_F(ProcessPipeTest, SendAndReceiveDataInt, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    int data = 42;
    EXPECT_TRUE(pipe.SendData(data));
    int received = 0;
    EXPECT_TRUE(pipe.ReceiveData(&received));
    EXPECT_EQ(received, 42);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveSizeT_WhenDataTransferred
 * @tc.number: ProcessPipe_SendReceive_Test_002
 * @tc.desc  : Test that ProcessPipe SendData and ReceiveData work for size_t type.
 */
HWTEST_F(ProcessPipeTest, SendAndReceiveDataSizeT, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    size_t data = 12345678;
    EXPECT_TRUE(pipe.SendData(data));
    size_t received = 0;
    EXPECT_TRUE(pipe.ReceiveData(&received));
    EXPECT_EQ(received, 12345678u);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveUintptr_WhenDataTransferred
 * @tc.number: ProcessPipe_SendReceive_Test_003
 * @tc.desc  : Test that ProcessPipe SendData and ReceiveData work for uintptr_t type.
 */
HWTEST_F(ProcessPipeTest, SendAndReceiveDataUintptr, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    uintptr_t data = 0xDEADBEEF;
    EXPECT_TRUE(pipe.SendData(data));
    uintptr_t received = 0;
    EXPECT_TRUE(pipe.ReceiveData(&received));
    EXPECT_EQ(received, 0xDEADBEEFu);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveString_WhenNonEmpty
 * @tc.number: ProcessPipe_SendReceive_Test_004
 * @tc.desc  : Test that ProcessPipe SendString and ReceiveString work for non-empty string.
 */
HWTEST_F(ProcessPipeTest, SendAndReceiveStringNonEmpty, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    std::string sent = "hello world";
    EXPECT_TRUE(pipe.SendString(sent));
    std::string received;
    EXPECT_TRUE(pipe.ReceiveString(received));
    EXPECT_EQ(received, "hello world");
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveString_WhenEmpty
 * @tc.number: ProcessPipe_SendReceive_Test_005
 * @tc.desc  : Test that ProcessPipe SendString and ReceiveString work for empty string.
 */
HWTEST_F(ProcessPipeTest, SendAndReceiveStringEmpty, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    std::string sent = "";
    EXPECT_TRUE(pipe.SendString(sent));
    std::string received;
    EXPECT_TRUE(pipe.ReceiveString(received));
    EXPECT_EQ(received, "");
}

/**
 * @tc.name  : ProcessPipe_ShouldSetFd_WhenCalled
 * @tc.number: ProcessPipe_SetFd_Test_001
 * @tc.desc  : Test that ProcessPipe SetFd sets file descriptors correctly.
 */
HWTEST_F(ProcessPipeTest, SetFd, TestSize.Level1)
{
    ProcessPipe pipe;
    pipe.SetFd(10, 20);
    EXPECT_EQ(pipe.GetWriteFd(), 20);
}

/**
 * @tc.name  : ProcessPipe_ShouldReturnValidFd_WhenDefaultConstructed
 * @tc.number: ProcessPipe_GetWriteFd_Test_001
 * @tc.desc  : Test that ProcessPipe GetWriteFd returns valid fd after default construction.
 */
HWTEST_F(ProcessPipeTest, GetWriteFdDefault, TestSize.Level1)
{
    ProcessPipe pipe;
    EXPECT_GE(pipe.GetWriteFd(), 0);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveMultipleValues_WhenSequentialTransfer
 * @tc.number: ProcessPipe_SendReceive_Test_006
 * @tc.desc  : Test that ProcessPipe SendData and ReceiveData work for multiple sequential values.
 */
HWTEST_F(ProcessPipeTest, SendDataMultipleValues, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    size_t size1 = 10;
    size_t size2 = 20;
    EXPECT_TRUE(pipe.SendData(size1));
    EXPECT_TRUE(pipe.SendData(size2));
    size_t r1 = 0, r2 = 0;
    EXPECT_TRUE(pipe.ReceiveData(&r1));
    EXPECT_TRUE(pipe.ReceiveData(&r2));
    EXPECT_EQ(r1, 10u);
    EXPECT_EQ(r2, 20u);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveLeakGraph_WhenOneNode
 * @tc.number: ProcessPipe_LeakGraph_Test_001
 * @tc.desc  : Test that ProcessPipe SendLeakGraph and ReceiveLeakGraph work with one node.
 */
HWTEST_F(ProcessPipeTest, SendAndReceiveLeakGraphOneNode, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    Heap heap;
    LeakGraphALC graph((Allocator<LeakGraphALC::value_type>(heap)));
    NodeALC node1((Allocator<NodeALC>(heap)));
    node1.capacity = 128;
    node1.references[0] = 0x1000;
    graph.insert({0x2000, node1});
    EXPECT_TRUE(pipe.SendLeakGraph(graph));

    LeakGraphSTL received;
    EXPECT_TRUE(pipe.ReceiveLeakGraph(received));
    EXPECT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0x2000].capacity, 128u);
    EXPECT_EQ(received[0x2000].references.size(), 1u);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveLeakGraph_WhenEmpty
 * @tc.number: ProcessPipe_LeakGraph_Test_002
 * @tc.desc  : Test that ProcessPipe SendLeakGraph and ReceiveLeakGraph work for empty graph.
 */
HWTEST_F(ProcessPipeTest, SendLeakGraphEmpty, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    Heap heap;
    LeakGraphALC graph((Allocator<LeakGraphALC::value_type>(heap)));
    EXPECT_TRUE(pipe.SendLeakGraph(graph));

    LeakGraphSTL received;
    EXPECT_TRUE(pipe.ReceiveLeakGraph(received));
    EXPECT_TRUE(received.empty());
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveLeakGraph_WhenMultipleNodes
 * @tc.number: ProcessPipe_LeakGraph_Test_003
 * @tc.desc  : Test that ProcessPipe SendLeakGraph and ReceiveLeakGraph work with multiple nodes.
 */
HWTEST_F(ProcessPipeTest, SendLeakGraphMultipleNodes, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    Heap heap;
    LeakGraphALC graph((Allocator<LeakGraphALC::value_type>(heap)));
    NodeALC node1((Allocator<NodeALC>(heap)));
    node1.capacity = 64;
    NodeALC node2((Allocator<NodeALC>(heap)));
    node2.capacity = 128;
    node2.references[8] = 0x2000;
    node2.references[16] = 0x3000;
    graph.insert({0x1000, node1});
    graph.insert({0x2000, node2});
    EXPECT_TRUE(pipe.SendLeakGraph(graph));

    LeakGraphSTL received;
    EXPECT_TRUE(pipe.ReceiveLeakGraph(received));
    EXPECT_EQ(received.size(), 2u);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveData_WhenLargeStruct
 * @tc.number: ProcessPipe_SendReceive_Test_007
 * @tc.desc  : Test that ProcessPipe SendData and ReceiveData work for large struct.
 */
HWTEST_F(ProcessPipeTest, SendDataLargeStruct, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    struct LargeData {
        char buf[256];
        int val;
    };
    LargeData data;
    if (memset_s(data.buf, sizeof(data.buf), 'A', sizeof(data.buf)) != EOK) {
        return;
    }
    data.val = 99;
    EXPECT_TRUE(pipe.SendData(data));
    LargeData received = {};
    EXPECT_TRUE(pipe.ReceiveData(&received));
    EXPECT_EQ(received.val, 99);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveString_WhenLongString
 * @tc.number: ProcessPipe_SendReceive_Test_008
 * @tc.desc  : Test that ProcessPipe SendString and ReceiveString work for long string.
 */
HWTEST_F(ProcessPipeTest, SendStringLong, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    std::string sent(10000, 'x');
    EXPECT_TRUE(pipe.SendString(sent));
    std::string received;
    EXPECT_TRUE(pipe.ReceiveString(received));
    EXPECT_EQ(received.size(), 10000u);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveLeakGraph_WhenNodeNoReferences
 * @tc.number: ProcessPipe_LeakGraph_Test_004
 * @tc.desc  : Test that ProcessPipe SendLeakGraph and ReceiveLeakGraph work for node with no references.
 */
HWTEST_F(ProcessPipeTest, SendLeakGraphNodeNoReferences, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    Heap heap;
    LeakGraphALC graph((Allocator<LeakGraphALC::value_type>(heap)));
    NodeALC node1((Allocator<NodeALC>(heap)));
    node1.capacity = 32;
    graph.insert({0x5000, node1});
    EXPECT_TRUE(pipe.SendLeakGraph(graph));

    LeakGraphSTL received;
    EXPECT_TRUE(pipe.ReceiveLeakGraph(received));
    EXPECT_EQ(received[0x5000].capacity, 32u);
    EXPECT_TRUE(received[0x5000].references.empty());
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveData_WhenBoolType
 * @tc.number: ProcessPipe_SendReceive_Test_009
 * @tc.desc  : Test that ProcessPipe SendData and ReceiveData work for bool type.
 */
HWTEST_F(ProcessPipeTest, SendDataBool, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    bool data = true;
    EXPECT_TRUE(pipe.SendData(data));
    bool received = false;
    EXPECT_TRUE(pipe.ReceiveData(&received));
    EXPECT_TRUE(received);
}

/**
 * @tc.name  : ProcessPipe_ShouldSendAndReceiveString_WhenSpecialCharacters
 * @tc.number: ProcessPipe_SendReceive_Test_010
 * @tc.desc  : Test that ProcessPipe SendString and ReceiveString work with special characters.
 */
HWTEST_F(ProcessPipeTest, SendStringSpecialChars, TestSize.Level1)
{
    ProcessPipe pipe;
    if (!pipe.IsOpened()) {
        return;
    }
    std::string sent = "hello\nworld\r\n\t\"\\";
    EXPECT_TRUE(pipe.SendString(sent));
    std::string received;
    EXPECT_TRUE(pipe.ReceiveString(received));
    EXPECT_EQ(received, sent);
}
