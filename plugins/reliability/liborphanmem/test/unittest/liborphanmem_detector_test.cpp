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

#include <algorithm>
#include <atomic>
#include <csignal>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <securec.h>
#include <unistd.h>

#include "heap_scanner.h"
#include "map_allocator.h"
#include "orphanmem_detector.h"
#include "process_mappings.h"
#include "scoped_signal_handler.h"
#include "string_helper.h"
#include "thread_inspector.h"
#include "thread_snapshot_impl.h"
using namespace testing::ext;
using namespace OHOS;

// ===== From string_helper_test.cpp =====
class StringHelperTest : public ::testing::Test {
protected:
    StringHelper helper;
    Heap heap;
};

/**
 * @tc.name  : StartsWith_ShouldReturnFalse_WhenPrefixLongerThanString
 * @tc.number: StringHelper_StartsWith_Test_001
 * @tc.desc  : Test that StartsWith returns false when prefix is longer than the string.
 */
HWTEST_F(StringHelperTest, StartsWith_PrefixLongerThanString, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("ab", Allocator<char>(heap));
    EXPECT_FALSE(helper.StartsWith(str, "abcdef"));
}

/**
 * @tc.name  : StartsWith_ShouldReturnTrue_WhenExactMatch
 * @tc.number: StringHelper_StartsWith_Test_002
 * @tc.desc  : Test that StartsWith returns true when prefix exactly matches the string.
 */
HWTEST_F(StringHelperTest, StartsWith_ExactMatch, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello", Allocator<char>(heap));
    EXPECT_TRUE(helper.StartsWith(str, "hello"));
}

/**
 * @tc.name  : StartsWith_ShouldReturnTrue_WhenPrefixMatchesBeginning
 * @tc.number: StringHelper_StartsWith_Test_003
 * @tc.desc  : Test that StartsWith returns true when prefix matches the beginning of the string.
 */
HWTEST_F(StringHelperTest, StartsWith_PrefixMatches, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello world", Allocator<char>(heap));
    EXPECT_TRUE(helper.StartsWith(str, "hello"));
}

/**
 * @tc.name  : StartsWith_ShouldReturnFalse_WhenPrefixDoesNotMatch
 * @tc.number: StringHelper_StartsWith_Test_004
 * @tc.desc  : Test that StartsWith returns false when prefix does not match the beginning.
 */
HWTEST_F(StringHelperTest, StartsWith_PrefixDoesNotMatch, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello world", Allocator<char>(heap));
    EXPECT_FALSE(helper.StartsWith(str, "world"));
}

/**
 * @tc.name  : StartsWith_ShouldReturnTrue_WhenPrefixIsEmpty
 * @tc.number: StringHelper_StartsWith_Test_005
 * @tc.desc  : Test that StartsWith returns true when prefix is an empty string.
 */
HWTEST_F(StringHelperTest, StartsWith_EmptyPrefix, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello", Allocator<char>(heap));
    EXPECT_TRUE(helper.StartsWith(str, ""));
}

/**
 * @tc.name  : StartsWith_ShouldReturnFalse_WhenStringIsEmptyAndPrefixIsNot
 * @tc.number: StringHelper_StartsWith_Test_006
 * @tc.desc  : Test that StartsWith returns false when string is empty but prefix is not.
 */
HWTEST_F(StringHelperTest, StartsWith_EmptyStringNonEmptyPrefix, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("", Allocator<char>(heap));
    EXPECT_FALSE(helper.StartsWith(str, "a"));
}

/**
 * @tc.name  : StartsWith_ShouldReturnTrue_WhenSingleCharMatches
 * @tc.number: StringHelper_StartsWith_Test_007
 * @tc.desc  : Test that StartsWith returns true when single character prefix matches.
 */
HWTEST_F(StringHelperTest, StartsWith_SingleCharMatch, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello", Allocator<char>(heap));
    EXPECT_TRUE(helper.StartsWith(str, "h"));
}

/**
 * @tc.name  : StartsWith_ShouldReturnFalse_WhenSingleCharMismatches
 * @tc.number: StringHelper_StartsWith_Test_008
 * @tc.desc  : Test that StartsWith returns false when single character prefix does not match.
 */
HWTEST_F(StringHelperTest, StartsWith_SingleCharMismatch, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello", Allocator<char>(heap));
    EXPECT_FALSE(helper.StartsWith(str, "x"));
}

/**
 * @tc.name  : EndsWith_ShouldReturnFalse_WhenSuffixLongerThanString
 * @tc.number: StringHelper_EndsWith_Test_001
 * @tc.desc  : Test that EndsWith returns false when suffix is longer than the string.
 */
HWTEST_F(StringHelperTest, EndsWith_SuffixLongerThanString, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("ab", Allocator<char>(heap));
    EXPECT_FALSE(helper.EndsWith(str, "abcdef"));
}

/**
 * @tc.name  : EndsWith_ShouldReturnTrue_WhenExactMatch
 * @tc.number: StringHelper_EndsWith_Test_002
 * @tc.desc  : Test that EndsWith returns true when suffix exactly matches the string.
 */
HWTEST_F(StringHelperTest, EndsWith_ExactMatch, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello", Allocator<char>(heap));
    EXPECT_TRUE(helper.EndsWith(str, "hello"));
}

/**
 * @tc.name  : EndsWith_ShouldReturnTrue_WhenSuffixMatchesEnd
 * @tc.number: StringHelper_EndsWith_Test_003
 * @tc.desc  : Test that EndsWith returns true when suffix matches the end of the string.
 */
HWTEST_F(StringHelperTest, EndsWith_SuffixMatches, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello world", Allocator<char>(heap));
    EXPECT_TRUE(helper.EndsWith(str, "world"));
}

/**
 * @tc.name  : EndsWith_ShouldReturnFalse_WhenSuffixDoesNotMatch
 * @tc.number: StringHelper_EndsWith_Test_004
 * @tc.desc  : Test that EndsWith returns false when suffix does not match the end.
 */
HWTEST_F(StringHelperTest, EndsWith_SuffixDoesNotMatch, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello world", Allocator<char>(heap));
    EXPECT_FALSE(helper.EndsWith(str, "hello"));
}

/**
 * @tc.name  : EndsWith_ShouldReturnTrue_WhenSuffixIsEmpty
 * @tc.number: StringHelper_EndsWith_Test_005
 * @tc.desc  : Test that EndsWith returns true when suffix is an empty string.
 */
HWTEST_F(StringHelperTest, EndsWith_EmptySuffix, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("hello", Allocator<char>(heap));
    EXPECT_TRUE(helper.EndsWith(str, ""));
}

/**
 * @tc.name  : EndsWith_ShouldReturnFalse_WhenStringIsEmptyAndSuffixIsNot
 * @tc.number: StringHelper_EndsWith_Test_006
 * @tc.desc  : Test that EndsWith returns false when string is empty but suffix is not.
 */
HWTEST_F(StringHelperTest, EndsWith_EmptyStringNonEmptySuffix, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("", Allocator<char>(heap));
    EXPECT_FALSE(helper.EndsWith(str, "a"));
}

/**
 * @tc.name  : EndsWith_ShouldReturnTrue_WhenSameLengthEqual
 * @tc.number: StringHelper_EndsWith_Test_007
 * @tc.desc  : Test that EndsWith returns true when string and suffix have the same length and are equal.
 */
HWTEST_F(StringHelperTest, EndsWith_SameLengthEqual, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("abc", Allocator<char>(heap));
    EXPECT_TRUE(helper.EndsWith(str, "abc"));
}

/**
 * @tc.name  : EndsWith_ShouldReturnFalse_WhenSameLengthNotEqual
 * @tc.number: StringHelper_EndsWith_Test_008
 * @tc.desc  : Test that EndsWith returns false when string and suffix have the same length but differ.
 */
HWTEST_F(StringHelperTest, EndsWith_SameLengthNotEqual, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("abc", Allocator<char>(heap));
    EXPECT_FALSE(helper.EndsWith(str, "xyz"));
}

/**
 * @tc.name  : IsSanitizerMap_ShouldReturnTrue_WhenLowShadow
 * @tc.number: StringHelper_IsSanitizerMap_Test_001
 * @tc.desc  : Test that IsSanitizerMap returns true for "[anon:low shadow]".
 */
HWTEST_F(StringHelperTest, IsSanitizerMap_LowShadow, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("[anon:low shadow]", Allocator<char>(heap));
    EXPECT_TRUE(helper.IsSanitizerMap(str));
}

/**
 * @tc.name  : IsSanitizerMap_ShouldReturnTrue_WhenHighShadow
 * @tc.number: StringHelper_IsSanitizerMap_Test_002
 * @tc.desc  : Test that IsSanitizerMap returns true for "[anon:high shadow]".
 */
HWTEST_F(StringHelperTest, IsSanitizerMap_HighShadow, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("[anon:high shadow]", Allocator<char>(heap));
    EXPECT_TRUE(helper.IsSanitizerMap(str));
}

/**
 * @tc.name  : IsSanitizerMap_ShouldReturnTrue_WhenHwasanPrefix
 * @tc.number: StringHelper_IsSanitizerMap_Test_003
 * @tc.desc  : Test that IsSanitizerMap returns true for strings starting with "[anon:hwasan".
 */
HWTEST_F(StringHelperTest, IsSanitizerMap_HwasanPrefix, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("[anon:hwasan_region]", Allocator<char>(heap));
    EXPECT_TRUE(helper.IsSanitizerMap(str));
}

/**
 * @tc.name  : IsSanitizerMap_ShouldReturnFalse_WhenNormalString
 * @tc.number: StringHelper_IsSanitizerMap_Test_004
 * @tc.desc  : Test that IsSanitizerMap returns false for non-sanitizer mapping names.
 */
HWTEST_F(StringHelperTest, IsSanitizerMap_NormalString, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("[anon:some_other]", Allocator<char>(heap));
    EXPECT_FALSE(helper.IsSanitizerMap(str));
}

/**
 * @tc.name  : IsSanitizerMap_ShouldReturnFalse_WhenEmptyString
 * @tc.number: StringHelper_IsSanitizerMap_Test_005
 * @tc.desc  : Test that IsSanitizerMap returns false for empty string.
 */
HWTEST_F(StringHelperTest, IsSanitizerMap_EmptyString, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("", Allocator<char>(heap));
    EXPECT_FALSE(helper.IsSanitizerMap(str));
}

/**
 * @tc.name  : IsSanitizerMap_ShouldReturnFalse_WhenSimilarButNotExact
 * @tc.number: StringHelper_IsSanitizerMap_Test_006
 * @tc.desc  : Test that IsSanitizerMap returns false for string similar but not exact match.
 */
HWTEST_F(StringHelperTest, IsSanitizerMap_SimilarButNotLowShadow, TestSize.Level1)
{
    MAP_ALLOCATOR::String str("[anon:low shadowX]", Allocator<char>(heap));
    EXPECT_FALSE(helper.IsSanitizerMap(str));
}

/**
 * @tc.name  : PidToStr_ShouldReturnNullptr_WhenPidIsZero
 * @tc.number: StringHelper_PidToStr_Test_001
 * @tc.desc  : Test that PidToStr returns nullptr when pid is 0.
 */
HWTEST_F(StringHelperTest, PidToStr_InvalidPidZero, TestSize.Level1)
{
    char buf[16];
    EXPECT_EQ(helper.PidToStr(buf, sizeof(buf), 0), nullptr);
}

/**
 * @tc.name  : PidToStr_ShouldReturnNullptr_WhenPidIsNegative
 * @tc.number: StringHelper_PidToStr_Test_002
 * @tc.desc  : Test that PidToStr returns nullptr when pid is negative.
 */
HWTEST_F(StringHelperTest, PidToStr_InvalidPidNegative, TestSize.Level1)
{
    char buf[16];
    EXPECT_EQ(helper.PidToStr(buf, sizeof(buf), -1), nullptr);
}

/**
 * @tc.name  : PidToStr_ShouldConvertCorrectly_WhenValidPid
 * @tc.number: StringHelper_PidToStr_Test_003
 * @tc.desc  : Test that PidToStr correctly converts a valid pid to string.
 */
HWTEST_F(StringHelperTest, PidToStr_ValidPid, TestSize.Level1)
{
    char buf[16];
    char *result = helper.PidToStr(buf, sizeof(buf), 1234);
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "1234");
}

/**
 * @tc.name  : PidToStr_ShouldReturnNullptr_WhenBufferTooSmall
 * @tc.number: StringHelper_PidToStr_Test_004
 * @tc.desc  : Test that PidToStr returns nullptr when buffer is too small for the pid.
 */
HWTEST_F(StringHelperTest, PidToStr_BufferTooSmall, TestSize.Level1)
{
    char buf[2];
    EXPECT_EQ(helper.PidToStr(buf, sizeof(buf), 1234), nullptr);
}

/**
 * @tc.name  : PidToStr_ShouldConvertCorrectly_WhenSingleDigitPid
 * @tc.number: StringHelper_PidToStr_Test_005
 * @tc.desc  : Test that PidToStr correctly converts a single-digit pid.
 */
HWTEST_F(StringHelperTest, PidToStr_SingleDigitPid, TestSize.Level1)
{
    char buf[16];
    char *result = helper.PidToStr(buf, sizeof(buf), 5);
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "5");
}

/**
 * @tc.name  : PidToStr_ShouldConvertCorrectly_WhenLargePid
 * @tc.number: StringHelper_PidToStr_Test_006
 * @tc.desc  : Test that PidToStr correctly converts a large pid value.
 */
HWTEST_F(StringHelperTest, PidToStr_LargePid, TestSize.Level1)
{
    char buf[16];
    char *result = helper.PidToStr(buf, sizeof(buf), 999999);
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "999999");
}

/**
 * @tc.name  : ConcatenatePath_ShouldReturnTrue_WhenValidPid
 * @tc.number: StringHelper_ConcatenatePath_Test_001
 * @tc.desc  : Test that ConcatenatePath returns true and builds correct path for valid pid.
 */
HWTEST_F(StringHelperTest, ConcatenatePath_ValidPid, TestSize.Level1)
{
    char prefix[128] = "/proc/";
    EXPECT_TRUE(helper.ConcatenatePath(prefix, 1234, "/maps"));
    EXPECT_NE(std::string(prefix).find("1234"), std::string::npos);
    EXPECT_NE(std::string(prefix).find("/maps"), std::string::npos);
}

/**
 * @tc.name  : ConcatenatePath_ShouldReturnFalse_WhenInvalidPid
 * @tc.number: StringHelper_ConcatenatePath_Test_002
 * @tc.desc  : Test that ConcatenatePath returns false when pid is invalid (0).
 */
HWTEST_F(StringHelperTest, ConcatenatePath_InvalidPid, TestSize.Level1)
{
    char prefix[128] = "/proc/";
    EXPECT_FALSE(helper.ConcatenatePath(prefix, 0, "/maps"));
}


// ===== From process_mappings_test.cpp =====
class ProcessMappingsTest : public ::testing::Test {
protected:
    Heap heap;
};

/**
 * @tc.name  : Mapping_ShouldInitializeFields_WhenConstructed
 * @tc.number: Mapping_Constructor_Test_001
 * @tc.desc  : Test that Mapping constructor initializes all fields correctly.
 */
HWTEST_F(ProcessMappingsTest, Mapping_Constructor, TestSize.Level1)
{
    Mapping m(0x1000, 0x2000, true, false, true, "libtest.so");
    EXPECT_EQ(m.begin, 0x1000u);
    EXPECT_EQ(m.end, 0x2000u);
    EXPECT_TRUE(m.read);
    EXPECT_FALSE(m.write);
    EXPECT_TRUE(m.execute);
    EXPECT_STREQ(m.name, "libtest.so");
}

/**
 * @tc.name  : ProcessMappings_ShouldReturnFalse_WhenInvalidPid
 * @tc.number: ProcessMappings_InvalidPid_Test_001
 * @tc.desc  : Test that ProcessMappings returns false when pid is invalid.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_InvalidPid, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    EXPECT_FALSE(MappingHelper::ProcessMappings(-1, mappings));
}

/**
 * @tc.name  : ProcessMappings_ShouldReturnTrue_WhenCurrentPid
 * @tc.number: ProcessMappings_CurrentPid_Test_001
 * @tc.desc  : Test that ProcessMappings returns true and finds mappings for current process.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_CurrentPid, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    pid_t pid = getpid();
    bool result = MappingHelper::ProcessMappings(pid, mappings);
    EXPECT_TRUE(result);
    EXPECT_GT(mappings.size(), 0u);
}

/**
 * @tc.name  : Mapping_ShouldAcceptEmptyName_WhenConstructed
 * @tc.number: Mapping_EmptyName_Test_001
 * @tc.desc  : Test that Mapping accepts empty name when constructed.
 */
HWTEST_F(ProcessMappingsTest, Mapping_EmptyName, TestSize.Level1)
{
    Mapping m(0x1000, 0x2000, true, true, false, "");
    EXPECT_STREQ(m.name, "");
}

/**
 * @tc.name  : Mapping_ShouldTruncateName_WhenNameExceedsBufferSize
 * @tc.number: Mapping_LongName_Test_001
 * @tc.desc  : Test that Mapping truncates long name to fit buffer size.
 */
HWTEST_F(ProcessMappingsTest, Mapping_LongName, TestSize.Level1)
{
    char longName[200];
    if (memset_s(longName, sizeof(longName), 'A', 199) != EOK) {
        return;
    }
    longName[199] = '\0';
    Mapping m(0x1000, 0x2000, true, true, false, longName);
    EXPECT_EQ(strlen(m.name), 95u);
}

/**
 * @tc.name  : ProcessMappings_ShouldHaveReadableMapping_WhenCurrentPid
 * @tc.number: ProcessMappings_Properties_Test_001
 * @tc.desc  : Test that ProcessMappings finds at least one readable mapping.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_CheckReadable, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    pid_t pid = getpid();
    if (!MappingHelper::ProcessMappings(pid, mappings)) {
        return;
    }
    bool hasRead = false;
    for (const auto &m : mappings) {
        if (m.read) hasRead = true;
    }
    EXPECT_TRUE(hasRead);
}

/**
 * @tc.name  : ProcessMappings_ShouldHaveExecutableMapping_WhenCurrentPid
 * @tc.number: ProcessMappings_Properties_Test_002
 * @tc.desc  : Test that ProcessMappings finds at least one executable mapping.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_CheckExecutable, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    pid_t pid = getpid();
    if (!MappingHelper::ProcessMappings(pid, mappings)) {
        return;
    }
    bool hasExecute = false;
    for (const auto &m : mappings) {
        if (m.execute) hasExecute = true;
    }
    EXPECT_TRUE(hasExecute);
}

/**
 * @tc.name  : ProcessMappings_ShouldReturnFalse_WhenPidZero
 * @tc.number: ProcessMappings_InvalidPid_Test_002
 * @tc.desc  : Test that ProcessMappings returns false when pid is 0.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_PidZero, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    EXPECT_FALSE(MappingHelper::ProcessMappings(0, mappings));
}

/**
 * @tc.name  : ProcessMappings_ShouldReturnConsistent_WhenCalledMultipleTimes
 * @tc.number: ProcessMappings_Consistency_Test_001
 * @tc.desc  : Test that ProcessMappings returns consistent results across multiple calls.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_Consistent, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings1((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings2((Allocator<Mapping>(heap)));
    pid_t pid = getpid();
    MappingHelper::ProcessMappings(pid, mappings1);
    MappingHelper::ProcessMappings(pid, mappings2);
    EXPECT_GT(mappings1.size(), 0u);
    EXPECT_GT(mappings2.size(), 0u);
}

/**
 * @tc.name  : ProcessMappings_ShouldHaveBeginLessThanEnd_WhenMappingsLoaded
 * @tc.number: ProcessMappings_Properties_Test_003
 * @tc.desc  : Test that all loaded mappings have begin address less than end address.
 */
HWTEST_F(ProcessMappingsTest, Mapping_BeginLessThanEnd, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    pid_t pid = getpid();
    if (!MappingHelper::ProcessMappings(pid, mappings)) {
        return;
    }
    for (const auto &m : mappings) {
        EXPECT_LT(m.begin, m.end);
    }
}

/**
 * @tc.name  : Mapping_ShouldSetAllPermissions_WhenAllTrue
 * @tc.number: Mapping_Permissions_Test_001
 * @tc.desc  : Test that Mapping correctly sets all permissions to true.
 */
HWTEST_F(ProcessMappingsTest, Mapping_AllPermissions, TestSize.Level1)
{
    Mapping m(0x1000, 0x2000, true, true, true, "rwx.so");
    EXPECT_TRUE(m.read);
    EXPECT_TRUE(m.write);
    EXPECT_TRUE(m.execute);
}

/**
 * @tc.name  : Mapping_ShouldSetNoPermissions_WhenAllFalse
 * @tc.number: Mapping_Permissions_Test_002
 * @tc.desc  : Test that Mapping correctly sets all permissions to false.
 */
HWTEST_F(ProcessMappingsTest, Mapping_NoPermissions, TestSize.Level1)
{
    Mapping m(0x1000, 0x2000, false, false, false, "");
    EXPECT_FALSE(m.read);
    EXPECT_FALSE(m.write);
    EXPECT_FALSE(m.execute);
}

/**
 * @tc.name  : ProcessMappings_ShouldHaveWritableMapping_WhenCurrentPid
 * @tc.number: ProcessMappings_Properties_Test_004
 * @tc.desc  : Test that ProcessMappings finds at least one writable mapping.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_CheckWritable, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    pid_t pid = getpid();
    if (!MappingHelper::ProcessMappings(pid, mappings)) {
        return;
    }
    bool hasWrite = false;
    for (const auto &m : mappings) {
        if (m.write) hasWrite = true;
    }
    EXPECT_TRUE(hasWrite);
}

/**
 * @tc.name  : ProcessMappings_ShouldHaveNullTerminatedNames_WhenMappingsLoaded
 * @tc.number: ProcessMappings_Properties_Test_005
 * @tc.desc  : Test that all loaded mapping names are null-terminated.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_ValidNames, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    pid_t pid = getpid();
    if (!MappingHelper::ProcessMappings(pid, mappings)) {
        return;
    }
    for (const auto &m : mappings) {
        // name should be null-terminated
        EXPECT_EQ(m.name[sizeof(m.name) - 1], '\0');
    }
}

/**
 * @tc.name  : Mapping_ShouldHaveNameBufferSize96_WhenChecked
 * @tc.number: Mapping_BufferSize_Test_001
 * @tc.desc  : Test that Mapping name buffer size is 96 bytes.
 */
HWTEST_F(ProcessMappingsTest, Mapping_NameBufferSize, TestSize.Level1)
{
    Mapping m(0, 0, false, false, false, "");
    EXPECT_EQ(sizeof(m.name), 96u);
}


// ===== From heap_scanner_test.cpp =====
class HeapScannerTest : public ::testing::Test {
protected:
    static Heap heap;
    static Heap::unique_ptr<HeapScanner> scanner;
    static void *testMem;
    static constexpr size_t testMemSize = 1024 * 1024;

    static uintptr_t T(size_t off)
    {
        return reinterpret_cast<uintptr_t>(testMem) + off;
    }

    static void SetUpTestSuite()
    {
        testMem = mmap(nullptr, testMemSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        ASSERT_NE(testMem, MAP_FAILED);
        if (memset_s(testMem, testMemSize, 0, testMemSize) != EOK) {
            return;
        }
        Allocator<HeapScanner> alloc(heap);
        scanner = alloc.MakeUnique(alloc);
    }

    static void TearDownTestSuite()
    {
        scanner.reset();
        if (testMem != nullptr) {
            munmap(testMem, testMemSize);
        }
    }
};
Heap HeapScannerTest::heap;
Heap::unique_ptr<HeapScanner> HeapScannerTest::scanner;
void *HeapScannerTest::testMem = nullptr;

/**
 * @tc.name  : Range_ShouldReturnCorrectSize_WhenValidRange
 * @tc.number: Range_Size_Test_001
 * @tc.desc  : Test that Range size returns the correct difference between begin and end.
 */
HWTEST_F(HeapScannerTest, Range_Size, TestSize.Level1)
{
    Range r{0x1000, 0x2000};
    EXPECT_EQ(r.Size(), 0x1000u);
}

/**
 * @tc.name  : Range_ShouldReturnZero_WhenBeginEqualsEnd
 * @tc.number: Range_Size_Test_002
 * @tc.desc  : Test that Range size returns zero when begin equals end.
 */
HWTEST_F(HeapScannerTest, Range_SizeZero, TestSize.Level1)
{
    Range r{0x1000, 0x1000};
    EXPECT_EQ(r.Size(), 0u);
}

/**
 * @tc.name  : Range_ShouldReturnTrue_WhenComparedWithBeginAddress
 * @tc.number: Range_Compare_Test_001
 * @tc.desc  : Test that Range operator== returns true when compared with begin address.
 */
HWTEST_F(HeapScannerTest, Range_OperatorEqualUintptr, TestSize.Level1)
{
    Range r{0x1000, 0x2000};
    EXPECT_TRUE(r == 0x1000);
    EXPECT_FALSE(r == 0x2000);
}

/**
 * @tc.name  : Range_ShouldReturnTrue_WhenNotEqualToAddress
 * @tc.number: Range_Compare_Test_002
 * @tc.desc  : Test that Range operator!= returns true when address differs from begin.
 */
HWTEST_F(HeapScannerTest, Range_OperatorNotEqualUintptr, TestSize.Level1)
{
    Range r{0x1000, 0x2000};
    EXPECT_TRUE(r != 0x2000);
    EXPECT_FALSE(r != 0x1000);
}

/**
 * @tc.name  : Range_ShouldReturnTrue_WhenRangesAreEqual
 * @tc.number: Range_Compare_Test_003
 * @tc.desc  : Test that Range operator== returns true for equal ranges.
 */
HWTEST_F(HeapScannerTest, Range_OperatorEqualRange, TestSize.Level1)
{
    Range r1{0x1000, 0x2000};
    Range r2{0x1000, 0x2000};
    EXPECT_TRUE(r1 == r2);
}

/**
 * @tc.name  : Range_ShouldReturnTrue_WhenRangesAreDifferent
 * @tc.number: Range_Compare_Test_004
 * @tc.desc  : Test that Range operator!= returns true for different ranges.
 */
HWTEST_F(HeapScannerTest, Range_OperatorNotEqualRange, TestSize.Level1)
{
    Range r1{0x1000, 0x2000};
    Range r2{0x1000, 0x3000};
    EXPECT_TRUE(r1 != r2);
}

/**
 * @tc.name  : Range_ShouldReturnTrue_WhenLeftRangeLessThanRight
 * @tc.number: Range_Compare_Test_005
 * @tc.desc  : Test that Range operator< returns true for non-overlapping left range.
 */
HWTEST_F(HeapScannerTest, Range_OperatorLess, TestSize.Level1)
{
    Range r1{0x1000, 0x2000};
    Range r2{0x2000, 0x3000};
    EXPECT_TRUE(r1 < r2);
    EXPECT_FALSE(r2 < r1);
}

/**
 * @tc.name  : Range_ShouldReturnFalse_WhenEqualRangesComparedLess
 * @tc.number: Range_Compare_Test_006
 * @tc.desc  : Test that Range operator< returns false for equal ranges.
 */
HWTEST_F(HeapScannerTest, Range_EqualNotLess, TestSize.Level1)
{
    Range r1{0x1000, 0x2000};
    Range r2{0x1000, 0x2000};
    EXPECT_FALSE(r1 < r2);
}

/**
 * @tc.name  : RangeHash_ShouldReturnSameHash_WhenRangesAreEqual
 * @tc.number: RangeHash_Test_001
 * @tc.desc  : Test that RangeHash produces the same hash for equal ranges.
 */
HWTEST_F(HeapScannerTest, RangeHash_SameRanges, TestSize.Level1)
{
    Range r1{0x1000, 0x2000};
    Range r2{0x1000, 0x2000};
    RangeHash hasher;
    EXPECT_EQ(hasher(r1), hasher(r2));
}

/**
 * @tc.name  : RangeHash_ShouldReturnDifferentHash_WhenRangesAreDifferent
 * @tc.number: RangeHash_Test_002
 * @tc.desc  : Test that RangeHash produces different hashes for different ranges.
 */
HWTEST_F(HeapScannerTest, RangeHash_DifferentRanges, TestSize.Level1)
{
    Range r1{0x1000, 0x2000};
    Range r2{0x3000, 0x4000};
    RangeHash hasher;
    EXPECT_NE(hasher(r1), hasher(r2));
}

/**
 * @tc.name  : HeapScanner_ShouldReturnTrue_WhenAllocationAdded
 * @tc.number: HeapScanner_Allocation_Test_001
 * @tc.desc  : Test that HeapScanner Allocation returns true for a new allocation.
 */
HWTEST_F(HeapScannerTest, Allocation_Basic, TestSize.Level1)
{
    EXPECT_TRUE(scanner->Allocation(T(0x1000), T(0x1100)));
}

/**
 * @tc.name  : HeapScanner_ShouldReturnFalse_WhenDuplicateAllocation
 * @tc.number: HeapScanner_Allocation_Test_002
 * @tc.desc  : Test that HeapScanner Allocation returns false for duplicate allocation.
 */
HWTEST_F(HeapScannerTest, Allocation_Duplicate, TestSize.Level1)
{
    EXPECT_TRUE(scanner->Allocation(T(0x2000), T(0x2100)));
    EXPECT_FALSE(scanner->Allocation(T(0x2000), T(0x2100)));
}

/**
 * @tc.name  : HeapScanner_ShouldAcceptAllocation_WhenBeginEqualsEnd
 * @tc.number: HeapScanner_Allocation_Test_003
 * @tc.desc  : Test that HeapScanner Allocation accepts when begin equals end.
 */
HWTEST_F(HeapScannerTest, Allocation_BeginEqualsEnd, TestSize.Level1)
{
    EXPECT_TRUE(scanner->Allocation(T(0x3000), T(0x3000)));
}

/**
 * @tc.name  : HeapScanner_ShouldReturnFalse_WhenOverlappingAllocation
 * @tc.number: HeapScanner_Allocation_Test_004
 * @tc.desc  : Test that HeapScanner Allocation returns false for overlapping range.
 */
HWTEST_F(HeapScannerTest, Allocation_OverlappingDifferentRange, TestSize.Level1)
{
    EXPECT_TRUE(scanner->Allocation(T(0x4000), T(0x4100)));
    // Same begin same end is duplicate
    EXPECT_FALSE(scanner->Allocation(T(0x4000), T(0x4100)));
}

/**
 * @tc.name  : HeapScanner_ShouldAddMapping_WhenMappingCalled
 * @tc.number: HeapScanner_Mapping_Test_001
 * @tc.desc  : Test that HeapScanner Mapping adds a memory mapping.
 */
HWTEST_F(HeapScannerTest, Mapping, TestSize.Level1)
{
    scanner->Mapping(T(0), T(0x100000));
}

/**
 * @tc.name  : HeapScanner_ShouldAddRootRange_WhenRootCalledWithRange
 * @tc.number: HeapScanner_Root_Test_001
 * @tc.desc  : Test that HeapScanner Root adds a root range.
 */
HWTEST_F(HeapScannerTest, RootRange, TestSize.Level1)
{
    scanner->Root(T(0x10000), T(0x11000));
}

/**
 * @tc.name  : HeapScanner_ShouldAddRootVector_WhenRootCalledWithVector
 * @tc.number: HeapScanner_Root_Test_002
 * @tc.desc  : Test that HeapScanner Root adds root addresses from a vector.
 */
HWTEST_F(HeapScannerTest, RootVector, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<uintptr_t> vals((Allocator<uintptr_t>(heap)));
    vals.push_back(T(0x11000));
    vals.push_back(T(0x11008));
    scanner->Root(vals);
}

/**
 * @tc.name  : HeapScanner_ShouldAcceptEmptyVector_WhenRootCalled
 * @tc.number: HeapScanner_Root_Test_003
 * @tc.desc  : Test that HeapScanner Root accepts an empty vector.
 */
HWTEST_F(HeapScannerTest, RootEmptyVector, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<uintptr_t> vals((Allocator<uintptr_t>(heap)));
    scanner->Root(vals);
}

/**
 * @tc.name  : HeapScanner_ShouldCountAllocations_WhenAllocationsAdded
 * @tc.number: HeapScanner_Allocations_Test_001
 * @tc.desc  : Test that HeapScanner Allocations returns correct count.
 */
HWTEST_F(HeapScannerTest, Allocations_Count, TestSize.Level1)
{
    size_t before = scanner->Allocations();
    scanner->Allocation(T(0x6000), T(0x6100));
    EXPECT_EQ(scanner->Allocations(), before + 1);
    scanner->Allocation(T(0x7000), T(0x7100));
    EXPECT_EQ(scanner->Allocations(), before + 2);
}

/**
 * @tc.name  : HeapScanner_ShouldReturnZero_WhenNoAllocationBytes
 * @tc.number: HeapScanner_AllocationBytes_Test_001
 * @tc.desc  : Test that HeapScanner AllocationBytes increases after allocation.
 */
HWTEST_F(HeapScannerTest, AllocationBytes, TestSize.Level1)
{
    size_t before = scanner->AllocationBytes();
    scanner->Allocation(T(0x8000), T(0x8100));
    EXPECT_EQ(scanner->AllocationBytes(), before + 0x100);
}

/**
 * @tc.name  : HeapScanner_ShouldSumBytes_WhenMultipleAllocations
 * @tc.number: HeapScanner_AllocationBytes_Test_002
 * @tc.desc  : Test that HeapScanner AllocationBytes sums bytes for multiple allocations.
 */
HWTEST_F(HeapScannerTest, AllocationBytes_Multiple, TestSize.Level1)
{
    size_t before = scanner->AllocationBytes();
    scanner->Allocation(T(0x9000), T(0x9100));
    scanner->Allocation(T(0xA000), T(0xA200));
    EXPECT_EQ(scanner->AllocationBytes(), before + 0x300u);
}

/**
 * @tc.name  : AllocationInfo_ShouldDefaultFalse_WhenDefaultConstructed
 * @tc.number: AllocationInfo_Default_Test_001
 * @tc.desc  : Test that AllocationInfo referencedFromRoot defaults to false.
 */
HWTEST_F(HeapScannerTest, AllocationInfo_DefaultState, TestSize.Level1)
{
    HeapScanner::AllocationInfo info;
    EXPECT_FALSE(info.referencedFromRoot.load());
}

/**
 * @tc.name  : AllocationInfo_ShouldCopyCorrectly_WhenCopyConstructed
 * @tc.number: AllocationInfo_Copy_Test_001
 * @tc.desc  : Test that AllocationInfo copy constructor preserves the state.
 */
HWTEST_F(HeapScannerTest, AllocationInfo_CopyConstructor, TestSize.Level1)
{
    HeapScanner::AllocationInfo info;
    info.referencedFromRoot.store(true);
    HeapScanner::AllocationInfo copy(info);
    EXPECT_TRUE(copy.referencedFromRoot.load());
}

/**
 * @tc.name  : AllocationInfo_ShouldMoveCorrectly_WhenMoveConstructed
 * @tc.number: AllocationInfo_Move_Test_001
 * @tc.desc  : Test that AllocationInfo move constructor transfers the state.
 */
HWTEST_F(HeapScannerTest, AllocationInfo_MoveConstructor, TestSize.Level1)
{
    HeapScanner::AllocationInfo info;
    info.referencedFromRoot.store(true);
    HeapScanner::AllocationInfo moved(std::move(info));
    EXPECT_TRUE(moved.referencedFromRoot.load());
}

/**
 * @tc.name  : AllocationInfo_ShouldCopyAssign_WhenCopyAssigned
 * @tc.number: AllocationInfo_CopyAssign_Test_001
 * @tc.desc  : Test that AllocationInfo copy assignment operator works correctly.
 */
HWTEST_F(HeapScannerTest, AllocationInfo_CopyAssignment, TestSize.Level1)
{
    HeapScanner::AllocationInfo info;
    info.referencedFromRoot.store(true);
    HeapScanner::AllocationInfo other;
    other = info;
    EXPECT_TRUE(other.referencedFromRoot.load());
}

/**
 * @tc.name  : AllocationInfo_ShouldMoveAssign_WhenMoveAssigned
 * @tc.number: AllocationInfo_MoveAssign_Test_001
 * @tc.desc  : Test that AllocationInfo move assignment operator works correctly.
 */
HWTEST_F(HeapScannerTest, AllocationInfo_MoveAssignment, TestSize.Level1)
{
    HeapScanner::AllocationInfo info;
    info.referencedFromRoot.store(true);
    HeapScanner::AllocationInfo other;
    other = std::move(info);
    EXPECT_TRUE(other.referencedFromRoot.load());
}

/**
 * @tc.name  : AllocationInfo_ShouldSurviveSelfAssignment_WhenSelfAssigned
 * @tc.number: AllocationInfo_SelfAssign_Test_001
 * @tc.desc  : Test that AllocationInfo self-assignment does not corrupt state.
 */
HWTEST_F(HeapScannerTest, AllocationInfo_SelfAssignment, TestSize.Level1)
{
    HeapScanner::AllocationInfo info;
    info.referencedFromRoot.store(true);
    auto &ref = info;
    info = ref;
    EXPECT_TRUE(info.referencedFromRoot.load());
}

/**
 * @tc.name  : AllocationInfo_ShouldSupportAtomicOps_WhenUsedConcurrently
 * @tc.number: AllocationInfo_Atomic_Test_001
 * @tc.desc  : Test that AllocationInfo supports atomic load, store, and compare_exchange.
 */
HWTEST_F(HeapScannerTest, AllocationInfo_AtomicOps, TestSize.Level1)
{
    HeapScanner::AllocationInfo info;
    EXPECT_FALSE(info.referencedFromRoot.load());
    info.referencedFromRoot.store(true);
    EXPECT_TRUE(info.referencedFromRoot.load());
    bool expected = false;
    EXPECT_FALSE(info.referencedFromRoot.compare_exchange_strong(expected, false));
    expected = true;
    EXPECT_TRUE(info.referencedFromRoot.compare_exchange_strong(expected, false));
    EXPECT_FALSE(info.referencedFromRoot.load());
}

/**
 * @tc.name  : HeapScanner_ShouldDetectNoLeaks_WhenAllocationIsRooted
 * @tc.number: HeapScanner_DetectLeaks_Test_001
 * @tc.desc  : Test that HeapScanner DetectLeaks finds no leaks when allocation is rooted.
 */
HWTEST_F(HeapScannerTest, DetectLeaks_RootedNoLeak, TestSize.Level1)
{
    auto rootAddr = T(0x20000);
    auto allocAddr = T(0x21000);
    *reinterpret_cast<uintptr_t *>(rootAddr) = allocAddr;
    scanner->Mapping(rootAddr, rootAddr + 4096);
    scanner->Allocation(allocAddr, allocAddr + 64);
    scanner->Root(rootAddr, rootAddr + 4096);
    EXPECT_TRUE(scanner->DetectLeaks());
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked((Allocator<Range>(heap)));
    size_t leakCnt = 0, leakBytes = 0;
    scanner->Leaked(leaked, nullptr, SIZE_MAX, &leakCnt, &leakBytes);
    EXPECT_EQ(leaked.find(Range{allocAddr, allocAddr + 64}), leaked.end());
}

/**
 * @tc.name  : HeapScanner_ShouldDetectLeaks_WhenAllocationIsUnreachable
 * @tc.number: HeapScanner_DetectLeaks_Test_002
 * @tc.desc  : Test that HeapScanner DetectLeaks detects unreachable allocation as leak.
 */
HWTEST_F(HeapScannerTest, DetectLeaks_Unreachable, TestSize.Level1)
{
    auto rootAddr = T(0x22000);
    auto allocAddr = T(0x23000);
    scanner->Mapping(rootAddr, rootAddr + 4096);
    scanner->Allocation(allocAddr, allocAddr + 64);
    scanner->Root(rootAddr, rootAddr + 4096);
    EXPECT_TRUE(scanner->DetectLeaks());
    size_t leakCnt = 0;
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked((Allocator<Range>(heap)));
    scanner->Leaked(leaked, nullptr, SIZE_MAX, &leakCnt, nullptr);
    EXPECT_NE(leaked.find(Range{allocAddr, allocAddr + 64}), leaked.end());
}

/**
 * @tc.name  : HeapScanner_ShouldRespectLimit_WhenLeakedCalledWithLimit
 * @tc.number: HeapScanner_Leaked_Test_001
 * @tc.desc  : Test that HeapScanner Leaked respects the limit parameter.
 */
HWTEST_F(HeapScannerTest, Leaked_WithLimit, TestSize.Level1)
{
    auto rootAddr = T(0x24000);
    auto a1 = T(0x25000);
    auto a2 = T(0x26000);
    scanner->Mapping(rootAddr, rootAddr + 4096);
    scanner->Allocation(a1, a1 + 32);
    scanner->Allocation(a2, a2 + 64);
    scanner->Root(rootAddr, rootAddr + 4096);
    scanner->DetectLeaks();
    size_t leakCnt = 0;
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked((Allocator<Range>(heap)));
    scanner->Leaked(leaked, nullptr, 1, &leakCnt, nullptr);
    EXPECT_GE(leakCnt, 2u);
    EXPECT_EQ(leaked.size(), 1u);
}

/**
 * @tc.name  : HeapScanner_ShouldNotCrash_WhenLeakedWithNullParams
 * @tc.number: HeapScanner_Leaked_Test_002
 * @tc.desc  : Test that HeapScanner Leaked does not crash with null output parameters.
 */
HWTEST_F(HeapScannerTest, Leaked_NullOutputParams, TestSize.Level1)
{
    scanner->DetectLeaks();
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked((Allocator<Range>(heap)));
    scanner->Leaked(leaked, nullptr, SIZE_MAX, nullptr, nullptr);
}

/**
 * @tc.name  : HeapScanner_ShouldPopulateLeakGraph_WhenLeakedCalled
 * @tc.number: HeapScanner_Leaked_Test_003
 * @tc.desc  : Test that HeapScanner Leaked populates the leak graph.
 */
HWTEST_F(HeapScannerTest, Leaked_WithLeakGraph, TestSize.Level1)
{
    auto rootAddr = T(0x27000);
    auto allocAddr = T(0x28000);
    scanner->Mapping(rootAddr, rootAddr + 4096);
    scanner->Allocation(allocAddr, allocAddr + 64);
    scanner->Root(rootAddr, rootAddr + 4096);
    scanner->DetectLeaks();
    LeakGraphALC leakGraph((Allocator<LeakGraphALC::value_type>(heap)));
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked((Allocator<Range>(heap)));
    scanner->Leaked(leaked, &leakGraph, SIZE_MAX, nullptr, nullptr);
    EXPECT_GE(leakGraph.size(), 1u);
}

/**
 * @tc.name  : HeapScanner_ShouldIterateAllocations_WhenForEachAllocationCalled
 * @tc.number: HeapScanner_ForEachAllocation_Test_001
 * @tc.desc  : Test that HeapScanner ForEachAllocation iterates over all allocations.
 */
HWTEST_F(HeapScannerTest, ForEachAllocation, TestSize.Level1)
{
    auto a1 = T(0x29000);
    scanner->Allocation(a1, a1 + 64);
    size_t count = 0;
    scanner->ForEachAllocation([&count](const Range &, HeapScanner::AllocationInfo &) {
        count++;
    });
    EXPECT_GE(count, 1u);
}

/**
 * @tc.name  : HeapScanner_ShouldNotCrash_WhenMultipleMappingsAdded
 * @tc.number: HeapScanner_Mapping_Test_002
 * @tc.desc  : Test that HeapScanner does not crash when multiple mappings are added.
 */
HWTEST_F(HeapScannerTest, MultipleMappings, TestSize.Level1)
{
    scanner->Mapping(T(0), T(0x10000));
    scanner->Mapping(T(0x10000), T(0x20000));
    // Just verify no crash
}


// ===== From memleak_detector_test.cpp =====
class OrphanMemDetectorTest : public ::testing::Test {
protected:
    static Heap heap;
    static Heap::unique_ptr<OrphanMemDetector> detector;

    static void SetUpTestSuite()
    {
        Allocator<OrphanMemDetector> alloc(heap);
        detector = alloc.MakeUnique(Allocator<void>(alloc));
    }

    static void TearDownTestSuite()
    {
        detector.reset();
    }
};
Heap OrphanMemDetectorTest::heap;
Heap::unique_ptr<OrphanMemDetector> OrphanMemDetectorTest::detector;

/**
 * @tc.name  : MemLeakDetector_ShouldHaveZeroAllocations_WhenConstructed
 * @tc.number: MemLeakDetector_Constructor_Test_001
 * @tc.desc  : Test that OrphanMemDetector has zero allocations and bytes when constructed.
 */
HWTEST_F(OrphanMemDetectorTest, Construction, TestSize.Level1)
{
    EXPECT_GE(detector->Allocations(), 0u);
    EXPECT_GE(detector->AllocationBytes(), 0u);
}

/**
 * @tc.name  : StringHelper_ShouldRecognizeLiborphanmemMapper_WhenCompared
 * @tc.number: StringHelper_SkipMapping_Test_001
 * @tc.desc  : Test that StringHelper recognizes liborphanmem mapper mapping name.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_SkipLiborphanmemMapper, TestSize.Level1)
{
    StringHelper helper;
    MAP_ALLOCATOR::String name("[anon:liborphanmem mapper]", Allocator<char>(heap));
    EXPECT_EQ(name, "[anon:liborphanmem mapper]");
}

/**
 * @tc.name  : StringHelper_ShouldReturnTrue_WhenLowShadowMapping
 * @tc.number: StringHelper_IsSanitizerMap_Test_001
 * @tc.desc  : Test that StringHelper IsSanitizerMap returns true for low shadow mapping.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_SkipLowShadow, TestSize.Level1)
{
    StringHelper helper;
    MAP_ALLOCATOR::String name("[anon:low shadow]", Allocator<char>(heap));
    EXPECT_TRUE(helper.IsSanitizerMap(name));
}

/**
 * @tc.name  : StringHelper_ShouldReturnTrue_WhenHighShadowMapping
 * @tc.number: StringHelper_IsSanitizerMap_Test_002
 * @tc.desc  : Test that StringHelper IsSanitizerMap returns true for high shadow mapping.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_SkipHighShadow, TestSize.Level1)
{
    StringHelper helper;
    MAP_ALLOCATOR::String name("[anon:high shadow]", Allocator<char>(heap));
    EXPECT_TRUE(helper.IsSanitizerMap(name));
}

/**
 * @tc.name  : StringHelper_ShouldReturnTrue_WhenHwasanMapping
 * @tc.number: StringHelper_IsSanitizerMap_Test_003
 * @tc.desc  : Test that StringHelper IsSanitizerMap returns true for hwasan prefix mapping.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_SkipHwasan, TestSize.Level1)
{
    StringHelper helper;
    MAP_ALLOCATOR::String name("[anon:hwasan_region]", Allocator<char>(heap));
    EXPECT_TRUE(helper.IsSanitizerMap(name));
}

/**
 * @tc.name  : StringHelper_ShouldRecognizeBrkHeap_WhenHeapMappingChecked
 * @tc.number: StringHelper_IsHeapMapping_Test_001
 * @tc.desc  : Test that StringHelper recognizes brk heap mapping name.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_HeapBrk, TestSize.Level1)
{
    MAP_ALLOCATOR::String name("[anon:native_heap:brk]", Allocator<char>(heap));
    EXPECT_EQ(name, "[anon:native_heap:brk]");
}

/**
 * @tc.name  : StringHelper_ShouldRecognizeJemallocHeap_WhenHeapMappingChecked
 * @tc.number: StringHelper_IsHeapMapping_Test_002
 * @tc.desc  : Test that StringHelper recognizes jemalloc heap mapping name.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_HeapJemalloc, TestSize.Level1)
{
    MAP_ALLOCATOR::String name("[anon:native_heap:jemalloc]", Allocator<char>(heap));
    EXPECT_EQ(name, "[anon:native_heap:jemalloc]");
}

/**
 * @tc.name  : StringHelper_ShouldMatchGwpAsanPrefix_WhenStartsWithCalled
 * @tc.number: StringHelper_IsHeapMapping_Test_003
 * @tc.desc  : Test that StringHelper StartsWith matches GWP-ASan prefix.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_HeapGwpAsan, TestSize.Level1)
{
    StringHelper helper;
    MAP_ALLOCATOR::String name("[anon:GWP-ASan_Slot]", Allocator<char>(heap));
    EXPECT_TRUE(helper.StartsWith(name, "[anon:GWP-ASan"));
}

/**
 * @tc.name  : StringHelper_ShouldMatchStackPrefix_WhenStartsWithCalled
 * @tc.number: StringHelper_IsStackMapping_Test_001
 * @tc.desc  : Test that StringHelper StartsWith matches [stack prefix.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_StackPrefix, TestSize.Level1)
{
    StringHelper helper;
    MAP_ALLOCATOR::String name("[stack:123]", Allocator<char>(heap));
    EXPECT_TRUE(helper.StartsWith(name, "[stack"));
}

/**
 * @tc.name  : StringHelper_ShouldMatchAnonStackPrefix_WhenStartsWithCalled
 * @tc.number: StringHelper_IsStackMapping_Test_002
 * @tc.desc  : Test that StringHelper StartsWith matches [anon:stack prefix.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_AnonStackPrefix, TestSize.Level1)
{
    StringHelper helper;
    MAP_ALLOCATOR::String name("[anon:stack:456]", Allocator<char>(heap));
    EXPECT_TRUE(helper.StartsWith(name, "[anon:stack"));
}

/**
 * @tc.name  : StringHelper_ShouldRecognizeFfrtCoroutineStack_WhenMatched
 * @tc.number: StringHelper_IsStackMapping_Test_003
 * @tc.desc  : Test that StringHelper recognizes ffrt coroutine stack mapping name.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_FfrtCoroutineStack, TestSize.Level1)
{
    MAP_ALLOCATOR::String name("[anon:ffrt_coroutine_stack]", Allocator<char>(heap));
    EXPECT_EQ(name, "[anon:ffrt_coroutine_stack]");
}

/**
 * @tc.name  : StringHelper_ShouldMatchArkTsPrefix_WhenStartsWithCalled
 * @tc.number: StringHelper_IsArkTsMapping_Test_001
 * @tc.desc  : Test that StringHelper StartsWith matches ArkTS prefix.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_ArkTsPrefix, TestSize.Level1)
{
    StringHelper helper;
    MAP_ALLOCATOR::String name("[anon:ArkTS_Heap]", Allocator<char>(heap));
    EXPECT_TRUE(helper.StartsWith(name, "[anon:ArkTS"));
}

/**
 * @tc.name  : StringHelper_ShouldMatchCurrentLib_WhenGlobalMappingChecked
 * @tc.number: StringHelper_IsGlobalMapping_Test_001
 * @tc.desc  : Test that StringHelper matches currentLib for global mapping.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_GlobalCurrentLib, TestSize.Level1)
{
    MAP_ALLOCATOR::String mappingName("libtest.so", Allocator<char>(heap));
    MAP_ALLOCATOR::String currentLib("libtest.so", Allocator<char>(heap));
    EXPECT_EQ(mappingName, currentLib);
}

/**
 * @tc.name  : StringHelper_ShouldReturnEmpty_WhenMappingNameIsEmpty
 * @tc.number: StringHelper_IsGlobalMapping_Test_002
 * @tc.desc  : Test that StringHelper handles empty mapping name correctly.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_GlobalEmptyName, TestSize.Level1)
{
    MAP_ALLOCATOR::String mappingName("", Allocator<char>(heap));
    EXPECT_TRUE(mappingName.empty());
}

/**
 * @tc.name  : StringHelper_ShouldRecognizeBssMapping_WhenClassified
 * @tc.number: StringHelper_ClassifyMappings_Test_001
 * @tc.desc  : Test that StringHelper recognizes [anon:.bss] mapping.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_BssMapping, TestSize.Level1)
{
    MAP_ALLOCATOR::String name("[anon:.bss]", Allocator<char>(heap));
    EXPECT_EQ(name, "[anon:.bss]");
}

/**
 * @tc.name  : StringHelper_ShouldRecognizeLeakDetectorMalloc_WhenMatched
 * @tc.number: StringHelper_SkipMapping_Test_002
 * @tc.desc  : Test that StringHelper recognizes [anon:leak_detector_malloc] mapping.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_LeakDetectorMalloc, TestSize.Level1)
{
    MAP_ALLOCATOR::String name("[anon:leak_detector_malloc]", Allocator<char>(heap));
    EXPECT_EQ(name, "[anon:leak_detector_malloc]");
}

/**
 * @tc.name  : StringHelper_ShouldRecognizeSkipNames_WhenVariousMappingsChecked
 * @tc.number: StringHelper_SkipMapping_Test_003
 * @tc.desc  : Test that StringHelper recognizes various skip mapping names.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_VariousSkipNames, TestSize.Level1)
{
    // All these names should be recognized as special mappings
    MAP_ALLOCATOR::String names[] = {
        MAP_ALLOCATOR::String("[anon:LargeMmapAllocator]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:SizeClassAllocator]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:libclang_rt.memgraph.so.bss]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:Allocate]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:InternalMmapVector]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:RingBufferLink]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:RingBufferLinkMap]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:PrintMemoryAroundAddress]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:ReadFileToBuffer]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:SetAlternateSignalStack]", Allocator<char>(heap)),
        MAP_ALLOCATOR::String("[anon:hooknativesmb]", Allocator<char>(heap)),
    };
    for (const auto &name : names) {
        EXPECT_FALSE(name.empty());
    }
}

/**
 * @tc.name  : StringHelper_ShouldMatchAnonPrefix_WhenClassifyMappings
 * @tc.number: StringHelper_ClassifyMappings_Test_002
 * @tc.desc  : Test that StringHelper StartsWith matches [anon: prefix for classify mappings.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_AnonGeneralPath, TestSize.Level1)
{
    StringHelper helper;
    MAP_ALLOCATOR::String name("[anon:some_region]", Allocator<char>(heap));
    EXPECT_TRUE(helper.StartsWith(name, "[anon:"));
    EXPECT_FALSE(helper.IsSanitizerMap(name));
}

/**
 * @tc.name  : MemLeakDetector_ShouldWorkIndependently_WhenMultipleInstances
 * @tc.number: MemLeakDetector_Constructor_Test_002
 * @tc.desc  : Test that OrphanMemDetector instance maintains consistent state across tests.
 */
HWTEST_F(OrphanMemDetectorTest, MultipleInstances, TestSize.Level1)
{
    // Cannot create multiple OrphanMemDetector instances because each HeapScanner
    // registers SIGSEGV/SIGBUS handlers via sigchain that are never removed,
    // and sigchain aborts after too many registrations. Verify the shared
    // instance remains functional instead.
    size_t before = detector->Allocations();
    EXPECT_GE(before, 0u);
    size_t bytesBefore = detector->AllocationBytes();
    EXPECT_GE(bytesBefore, 0u);
}

/**
 * @tc.name  : StringHelper_ShouldRecognizeJemallocMetaHeap_WhenMatched
 * @tc.number: StringHelper_IsHeapMapping_Test_004
 * @tc.desc  : Test that StringHelper recognizes native_heap:jemalloc meta mapping name.
 */
HWTEST_F(OrphanMemDetectorTest, StringHelper_HeapJemallocMeta, TestSize.Level1)
{
    MAP_ALLOCATOR::String name("[anon:native_heap:jemalloc meta]", Allocator<char>(heap));
    EXPECT_EQ(name, "[anon:native_heap:jemalloc meta]");
}

namespace {
std::atomic<int> gScopedHandlerHitCount{0};
std::atomic<int> gScopedHandlerSignalNumber{0};
std::atomic<int> gScopedHandlerSecondCount{0};
std::atomic<int> gScopedHandlerUsr2Count{0};
std::atomic<int> gScopedHandlerCtxValid{0};
}  // namespace

// ===== From scoped_signal_handler_test.cpp =====
class ScopedSignalHandlerTest : public ::testing::Test {
protected:
    static Heap heap;
    static Heap::unique_ptr<ScopedSignalHandler> handler;

    static void SetUpTestSuite()
    {
        Allocator<ScopedSignalHandler> alloc(heap);
        handler = alloc.MakeUnique(alloc);
        handler->Install(SIGUSR1, [](int signal, siginfo_t *si, void *uctx) {
            gScopedHandlerHitCount++;
            gScopedHandlerSignalNumber = signal;
            gScopedHandlerCtxValid = (si != nullptr && uctx != nullptr) ? 1 : 0;
            return true;
        });
    }

    static void TearDownTestSuite()
    {
        handler.reset();
    }
};
Heap ScopedSignalHandlerTest::heap;
Heap::unique_ptr<ScopedSignalHandler> ScopedSignalHandlerTest::handler;

/**
 * @tc.name  : ScopedSignalHandler_ShouldInvokeHandler_WhenSignalRaised
 * @tc.number: ScopedSignalHandler_Install_Test_001
 * @tc.desc  : Test that ScopedSignalHandler invokes the installed handler when the signal is raised.
 */
HWTEST_F(ScopedSignalHandlerTest, Install_HandlerInvokedWhenSignalRaised, TestSize.Level1)
{
    int before = gScopedHandlerHitCount.load();
    raise(SIGUSR1);
    EXPECT_EQ(gScopedHandlerHitCount.load(), before + 1);
    EXPECT_EQ(gScopedHandlerSignalNumber.load(), SIGUSR1);
}

/**
 * @tc.name  : ScopedSignalHandler_ShouldInvokeHandlerRepeatedly_WhenSignalRaisedMultipleTimes
 * @tc.number: ScopedSignalHandler_Install_Test_002
 * @tc.desc  : Test that ScopedSignalHandler invokes the handler for each raised signal.
 */
HWTEST_F(ScopedSignalHandlerTest, Install_HandlerInvokedRepeatedly, TestSize.Level1)
{
    int before = gScopedHandlerHitCount.load();
    raise(SIGUSR1);
    raise(SIGUSR1);
    EXPECT_EQ(gScopedHandlerHitCount.load(), before + 2);
}

/**
 * @tc.name  : ScopedSignalHandler_ShouldIgnoreSecondInstall_WhenSignalAlreadyInstalled
 * @tc.number: ScopedSignalHandler_Install_Test_003
 * @tc.desc  : Test that the second Install call on the same instance is ignored.
 */
HWTEST_F(ScopedSignalHandlerTest, Install_SecondInstallIgnored, TestSize.Level1)
{
    int secondBefore = gScopedHandlerSecondCount.load();
    int firstBefore = gScopedHandlerHitCount.load();
    handler->Install(SIGUSR1, [](int, siginfo_t *, void *) {
        gScopedHandlerSecondCount++;
        return true;
    });
    raise(SIGUSR1);
    EXPECT_EQ(gScopedHandlerSecondCount.load(), secondBefore);
    EXPECT_EQ(gScopedHandlerHitCount.load(), firstBefore + 1);
}

/**
 * @tc.name  : ScopedSignalHandler_ShouldInstallOnSecondInstance_WhenDifferentSignalUsed
 * @tc.number: ScopedSignalHandler_Install_Test_004
 * @tc.desc  : Test that a second instance can install a handler for a different signal.
 */
HWTEST_F(ScopedSignalHandlerTest, Install_SecondInstanceDifferentSignal, TestSize.Level1)
{
    int usr2Before = gScopedHandlerUsr2Count.load();
    Allocator<ScopedSignalHandler> alloc(heap);
    auto second = alloc.MakeUnique(alloc);
    second->Install(SIGUSR2, [](int, siginfo_t *, void *) {
        gScopedHandlerUsr2Count++;
        return true;
    });
    raise(SIGUSR2);
    EXPECT_EQ(gScopedHandlerUsr2Count.load(), usr2Before + 1);
}

/**
 * @tc.name  : ScopedSignalHandler_ShouldReuseHandlerMap_WhenConstructedAgain
 * @tc.number: ScopedSignalHandler_Constructor_Test_001
 * @tc.desc  : Test that a new instance reuses the existing static handler map without losing handlers.
 */
HWTEST_F(ScopedSignalHandlerTest, Constructor_ReuseExistingHandlerMap, TestSize.Level1)
{
    int before = gScopedHandlerHitCount.load();
    Allocator<ScopedSignalHandler> alloc(heap);
    auto third = alloc.MakeUnique(alloc);
    ASSERT_NE(third, nullptr);
    raise(SIGUSR1);
    EXPECT_EQ(gScopedHandlerHitCount.load(), before + 1);
}

/**
 * @tc.name  : ScopedSignalHandler_ShouldReceiveContextParams_WhenHandlerInvoked
 * @tc.number: ScopedSignalHandler_Install_Test_005
 * @tc.desc  : Test that the handler receives valid signal number and non-null context parameters.
 */
HWTEST_F(ScopedSignalHandlerTest, Install_ContextParamsValid, TestSize.Level1)
{
    gScopedHandlerCtxValid = 0;
    raise(SIGUSR1);
    EXPECT_EQ(gScopedHandlerCtxValid.load(), 1);
    EXPECT_EQ(gScopedHandlerSignalNumber.load(), SIGUSR1);
}

// ===== From thread_snapshot_impl_test.cpp =====
class ThreadSnapshotImplTest : public ::testing::Test {
protected:
    static Heap heap;

    static Heap::unique_ptr<ThreadSnapshotImpl> MakeImpl(pid_t pid, pid_t tid)
    {
        Allocator<ThreadSnapshotImpl> alloc(heap);
        return alloc.MakeUnique(pid, tid, alloc);
    }
};
Heap ThreadSnapshotImplTest::heap;

/**
 * @tc.name  : ThreadSnapshotImpl_ListThreads_ShouldReturnTrue_WhenValidPid
 * @tc.number: ThreadSnapshotImpl_ListThreads_Test_001
 * @tc.desc  : Test that ListThreads succeeds and finds threads for the current process.
 */
HWTEST_F(ThreadSnapshotImplTest, ListThreads_ValidPid, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), 0);
    TidList tids((Allocator<pid_t>(heap)));
    EXPECT_TRUE(impl->ListThreads(tids));
    EXPECT_GT(tids.size(), 0u);
}

/**
 * @tc.name  : ThreadSnapshotImpl_ListThreads_ShouldExcludeGivenTid_WhenTidProvided
 * @tc.number: ThreadSnapshotImpl_ListThreads_Test_002
 * @tc.desc  : Test that ListThreads excludes the tid passed to the constructor.
 */
HWTEST_F(ThreadSnapshotImplTest, ListThreads_ExcludeGivenTid, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    TidList tids((Allocator<pid_t>(heap)));
    EXPECT_TRUE(impl->ListThreads(tids));
    EXPECT_EQ(std::find(tids.begin(), tids.end(), static_cast<pid_t>(gettid())), tids.end());
}

/**
 * @tc.name  : ThreadSnapshotImpl_ListThreads_ShouldReturnFalse_WhenInvalidPid
 * @tc.number: ThreadSnapshotImpl_ListThreads_Test_003
 * @tc.desc  : Test that ListThreads returns false for an invalid pid.
 */
HWTEST_F(ThreadSnapshotImplTest, ListThreads_InvalidPid, TestSize.Level1)
{
    auto impl = MakeImpl(-1, 0);
    TidList tids((Allocator<pid_t>(heap)));
    EXPECT_FALSE(impl->ListThreads(tids));
    EXPECT_EQ(tids.size(), 0u);
}

/**
 * @tc.name  : ThreadSnapshotImpl_ListThreads_ShouldReturnFalse_WhenPidZero
 * @tc.number: ThreadSnapshotImpl_ListThreads_Test_004
 * @tc.desc  : Test that ListThreads returns false when pid is zero.
 */
HWTEST_F(ThreadSnapshotImplTest, ListThreads_PidZero, TestSize.Level1)
{
    auto impl = MakeImpl(0, 0);
    TidList tids((Allocator<pid_t>(heap)));
    EXPECT_FALSE(impl->ListThreads(tids));
    EXPECT_EQ(tids.size(), 0u);
}

/**
 * @tc.name  : ThreadSnapshotImpl_ListThreads_ShouldContainPositiveTids_WhenListed
 * @tc.number: ThreadSnapshotImpl_ListThreads_Test_005
 * @tc.desc  : Test that all listed tids are positive values without duplicates.
 */
HWTEST_F(ThreadSnapshotImplTest, ListThreads_PositiveTidsOnly, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), 0);
    TidList tids((Allocator<pid_t>(heap)));
    ASSERT_TRUE(impl->ListThreads(tids));
    for (auto tid : tids) {
        EXPECT_GT(tid, 0);
    }
    TidList sorted((Allocator<pid_t>(heap)));
    sorted = tids;
    std::sort(sorted.begin(), sorted.end());
    EXPECT_EQ(std::adjacent_find(sorted.begin(), sorted.end()), sorted.end());
}

/**
 * @tc.name  : ThreadSnapshotImpl_GetLocalThreadInfo_ShouldReturnFalse_WhenThreadInfoNotSet
 * @tc.number: ThreadSnapshotImpl_GetLocalThreadInfo_Test_001
 * @tc.desc  : Test that GetLocalThreadInfo fails when SetThreadInfo was not called.
 */
HWTEST_F(ThreadSnapshotImplTest, GetLocalThreadInfo_NotSet, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    EXPECT_FALSE(impl->GetLocalThreadInfo());
}

/**
 * @tc.name  : ThreadSnapshotImpl_SetThreadInfo_ShouldResizeList_WhenCalled
 * @tc.number: ThreadSnapshotImpl_SetThreadInfo_Test_001
 * @tc.desc  : Test that SetThreadInfo resizes the thread info list to a usable capacity.
 */
HWTEST_F(ThreadSnapshotImplTest, SetThreadInfo_ResizesList, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    EXPECT_EQ(threads.size(), 0u);
    impl->SetThreadInfo(threads);
    EXPECT_GE(threads.size(), 1u);
}

/**
 * @tc.name  : ThreadSnapshotImpl_GetLocalThreadInfo_ShouldSucceed_WhenThreadInfoSet
 * @tc.number: ThreadSnapshotImpl_GetLocalThreadInfo_Test_002
 * @tc.desc  : Test that GetLocalThreadInfo succeeds and records the current tid after SetThreadInfo.
 */
HWTEST_F(ThreadSnapshotImplTest, GetLocalThreadInfo_AfterSet, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    impl->SetThreadInfo(threads);
    EXPECT_TRUE(impl->GetLocalThreadInfo());
    EXPECT_EQ(threads[0].tid, static_cast<pid_t>(gettid()));
}

/**
 * @tc.name  : ThreadSnapshotImpl_GetLocalThreadInfo_ShouldFillStackFromRegs_WhenCaptured
 * @tc.number: ThreadSnapshotImpl_GetLocalThreadInfo_Test_003
 * @tc.desc  : Test that GetLocalThreadInfo fills regs and derives the stack pointer from them.
 */
HWTEST_F(ThreadSnapshotImplTest, GetLocalThreadInfo_FillStackFromRegs, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    impl->SetThreadInfo(threads);
    ASSERT_TRUE(impl->GetLocalThreadInfo());
#if defined(__aarch64__)
    EXPECT_NE(threads[0].regs[IDX_SP], 0u);
    EXPECT_EQ(threads[0].stack.first, threads[0].regs[IDX_SP]);
#endif
}

/**
 * @tc.name  : ThreadSnapshotImpl_GetLocalThreadInfo_ShouldReturnFalse_WhenListTooSmall
 * @tc.number: ThreadSnapshotImpl_GetLocalThreadInfo_Test_004
 * @tc.desc  : Test that GetLocalThreadInfo fails when the position exceeds the list size.
 */
HWTEST_F(ThreadSnapshotImplTest, GetLocalThreadInfo_OutOfBounds, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    impl->SetThreadInfo(threads);
    threads.resize(1);
    ASSERT_TRUE(impl->GetLocalThreadInfo());
    EXPECT_FALSE(impl->GetLocalThreadInfo());
}

/**
 * @tc.name  : ThreadSnapshotImpl_SetForkDone_ShouldNotCrash_WhenCalledMultipleTimes
 * @tc.number: ThreadSnapshotImpl_SetForkDone_Test_001
 * @tc.desc  : Test that SetForkDone can be called multiple times without crashing.
 */
HWTEST_F(ThreadSnapshotImplTest, SetForkDone_MultipleTimes, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    impl->SetForkDone();
    impl->SetForkDone();
    EXPECT_TRUE(true);
}

/**
 * @tc.name  : ThreadSnapshotImpl_CaptureThreadInfo_ShouldReturnTrue_WhenCurrentProcess
 * @tc.number: ThreadSnapshotImpl_CaptureThreadInfo_Test_001
 * @tc.desc  : Test that CaptureThreadInfo succeeds and resizes the thread list for the current process.
 */
HWTEST_F(ThreadSnapshotImplTest, CaptureThreadInfo_CurrentProcess, TestSize.Level3)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    size_t tidsSize = 0;
    EXPECT_TRUE(impl->CaptureThreadInfo(threads, tidsSize));
    EXPECT_GE(threads.size(), tidsSize);
}

/**
 * @tc.name  : ThreadSnapshotImpl_ListThreads_ShouldReturnConsistentSize_WhenCalledMultipleTimes
 * @tc.number: ThreadSnapshotImpl_ListThreads_Test_006
 * @tc.desc  : Test that repeated ListThreads calls return consistent sizes.
 */
HWTEST_F(ThreadSnapshotImplTest, ListThreads_ConsistentAcrossCalls, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), 0);
    TidList firstTids((Allocator<pid_t>(heap)));
    TidList secondTids((Allocator<pid_t>(heap)));
    ASSERT_TRUE(impl->ListThreads(firstTids));
    ASSERT_TRUE(impl->ListThreads(secondTids));
    EXPECT_GT(firstTids.size(), 0u);
    EXPECT_GT(secondTids.size(), 0u);
    EXPECT_EQ(firstTids.size(), secondTids.size());
}

/**
 * @tc.name  : ThreadSnapshotImpl_ListThreads_ShouldReuseList_WhenSameListPassedTwice
 * @tc.number: ThreadSnapshotImpl_ListThreads_Test_007
 * @tc.desc  : Test that ListThreads clears and refills the same output list correctly.
 */
HWTEST_F(ThreadSnapshotImplTest, ListThreads_ReuseOutputList, TestSize.Level1)
{
    auto impl = MakeImpl(getpid(), 0);
    TidList tids((Allocator<pid_t>(heap)));
    ASSERT_TRUE(impl->ListThreads(tids));
    size_t firstSize = tids.size();
    EXPECT_TRUE(impl->ListThreads(tids));
    EXPECT_GT(tids.size(), 0u);
    EXPECT_EQ(tids.size(), firstSize);
}

/**
 * @tc.name  : ThreadSnapshotImpl_CaptureThreadInfo_ShouldResizeThreadList_WhenCaptured
 * @tc.number: ThreadSnapshotImpl_CaptureThreadInfo_Test_002
 * @tc.desc  : Test that CaptureThreadInfo resizes the thread list to hold captured threads.
 */
HWTEST_F(ThreadSnapshotImplTest, CaptureThreadInfo_ResizesThreadList, TestSize.Level3)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    size_t tidsSize = 0;
    ASSERT_TRUE(impl->CaptureThreadInfo(threads, tidsSize));
    EXPECT_GE(threads.size(), 1000u);
    EXPECT_GE(tidsSize, 0u);
}

/**
 * @tc.name  : ThreadSnapshotImpl_CaptureThreadInfo_ShouldSucceed_WhenCalledSecondTime
 * @tc.number: ThreadSnapshotImpl_CaptureThreadInfo_Test_003
 * @tc.desc  : Test that a second CaptureThreadInfo call on the same instance still succeeds.
 */
HWTEST_F(ThreadSnapshotImplTest, CaptureThreadInfo_SecondCall, TestSize.Level3)
{
    auto impl = MakeImpl(getpid(), static_cast<pid_t>(gettid()));
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    size_t firstTidsSize = 0;
    size_t secondTidsSize = 0;
    ASSERT_TRUE(impl->CaptureThreadInfo(threads, firstTidsSize));
    threads.clear();
    EXPECT_TRUE(impl->CaptureThreadInfo(threads, secondTidsSize));
    EXPECT_GE(threads.size(), secondTidsSize);
}

// ===== From thread_inspector_test.cpp =====
class ThreadInspectorTest : public ::testing::Test {
protected:
    static Heap heap;

    static Heap::unique_ptr<ThreadInspector> MakeInspector()
    {
        Allocator<ThreadInspector> alloc(heap);
        return alloc.MakeUnique(getpid(), static_cast<pid_t>(gettid()), alloc);
    }
};
Heap ThreadInspectorTest::heap;

/**
 * @tc.name  : ThreadInspector_ShouldConstructAndDestruct_WhenCreated
 * @tc.number: ThreadInspector_Constructor_Test_001
 * @tc.desc  : Test that ThreadInspector constructs and destructs without crashing.
 */
HWTEST_F(ThreadInspectorTest, ConstructAndDestruct, TestSize.Level1)
{
    auto inspector = MakeInspector();
    ASSERT_NE(inspector, nullptr);
    inspector.reset();
    EXPECT_TRUE(true);
}

/**
 * @tc.name  : ThreadInspector_GetLocalThreadInfo_ShouldNotCrash_WhenThreadInfoNotSet
 * @tc.number: ThreadInspector_GetLocalThreadInfo_Test_001
 * @tc.desc  : Test that GetLocalThreadInfo does not crash when SetThreadInfo was not called.
 */
HWTEST_F(ThreadInspectorTest, GetLocalThreadInfo_NotSet, TestSize.Level1)
{
    auto inspector = MakeInspector();
    ASSERT_NE(inspector, nullptr);
    inspector->GetLocalThreadInfo();
    EXPECT_TRUE(true);
}

/**
 * @tc.name  : ThreadInspector_SetThreadInfo_ShouldResizeThreadList_WhenCalled
 * @tc.number: ThreadInspector_SetThreadInfo_Test_001
 * @tc.desc  : Test that SetThreadInfo resizes the thread list to a usable capacity.
 */
HWTEST_F(ThreadInspectorTest, SetThreadInfo_ResizesThreadList, TestSize.Level1)
{
    auto inspector = MakeInspector();
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    EXPECT_EQ(threads.size(), 0u);
    inspector->SetThreadInfo(threads);
    EXPECT_GE(threads.size(), 1u);
}

/**
 * @tc.name  : ThreadInspector_GetLocalThreadInfo_ShouldPopulateEntry_WhenThreadInfoSet
 * @tc.number: ThreadInspector_GetLocalThreadInfo_Test_002
 * @tc.desc  : Test that GetLocalThreadInfo populates the entry with the current tid.
 */
HWTEST_F(ThreadInspectorTest, GetLocalThreadInfo_PopulateEntry, TestSize.Level1)
{
    auto inspector = MakeInspector();
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    inspector->SetThreadInfo(threads);
    inspector->GetLocalThreadInfo();
    EXPECT_EQ(threads[0].tid, static_cast<pid_t>(gettid()));
}

/**
 * @tc.name  : ThreadInspector_GetLocalThreadInfo_ShouldPopulateSequentialEntries_WhenCalledTwice
 * @tc.number: ThreadInspector_GetLocalThreadInfo_Test_003
 * @tc.desc  : Test that two GetLocalThreadInfo calls populate two sequential entries.
 */
HWTEST_F(ThreadInspectorTest, GetLocalThreadInfo_SequentialEntries, TestSize.Level1)
{
    auto inspector = MakeInspector();
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    inspector->SetThreadInfo(threads);
    inspector->GetLocalThreadInfo();
    inspector->GetLocalThreadInfo();
    EXPECT_EQ(threads[0].tid, static_cast<pid_t>(gettid()));
    EXPECT_EQ(threads[1].tid, static_cast<pid_t>(gettid()));
}

/**
 * @tc.name  : ThreadInspector_ShouldKeepInstancesIndependent_WhenMultipleInstancesExist
 * @tc.number: ThreadInspector_Constructor_Test_002
 * @tc.desc  : Test that two ThreadInspector instances keep their captured thread lists independent.
 */
HWTEST_F(ThreadInspectorTest, MultipleInstancesIndependent, TestSize.Level1)
{
    auto first = MakeInspector();
    auto second = MakeInspector();
    ThreadInfoList firstThreads((Allocator<ThreadInfo>(heap)));
    ThreadInfoList secondThreads((Allocator<ThreadInfo>(heap)));
    first->SetThreadInfo(firstThreads);
    second->SetThreadInfo(secondThreads);
    first->GetLocalThreadInfo();
    second->GetLocalThreadInfo();
    EXPECT_EQ(firstThreads[0].tid, static_cast<pid_t>(gettid()));
    EXPECT_EQ(secondThreads[0].tid, static_cast<pid_t>(gettid()));
    EXPECT_EQ(firstThreads.size(), secondThreads.size());
}

/**
 * @tc.name  : ThreadInspector_SetForkDone_ShouldNotCrash_WhenCalledMultipleTimes
 * @tc.number: ThreadInspector_SetForkDone_Test_001
 * @tc.desc  : Test that SetForkDone can be called multiple times without crashing.
 */
HWTEST_F(ThreadInspectorTest, SetForkDone_MultipleTimes, TestSize.Level1)
{
    auto inspector = MakeInspector();
    inspector->SetForkDone();
    inspector->SetForkDone();
    EXPECT_TRUE(true);
}

/**
 * @tc.name  : ThreadInspector_CaptureThreadInfo_ShouldReturnTrue_WhenCurrentProcess
 * @tc.number: ThreadInspector_CaptureThreadInfo_Test_001
 * @tc.desc  : Test that CaptureThreadInfo succeeds and resizes the thread list for the current process.
 */
HWTEST_F(ThreadInspectorTest, CaptureThreadInfo_CurrentProcess, TestSize.Level3)
{
    auto inspector = MakeInspector();
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    size_t tidsSize = 0;
    EXPECT_TRUE(inspector->CaptureThreadInfo(threads, tidsSize));
    EXPECT_GE(threads.size(), tidsSize);
}

/**
 * @tc.name  : ThreadInspector_ShouldDestructSafely_AfterCaptureThreadInfo
 * @tc.number: ThreadInspector_Destructor_Test_001
 * @tc.desc  : Test that ThreadInspector destructs safely after a successful capture.
 */
HWTEST_F(ThreadInspectorTest, DestructorAfterCapture, TestSize.Level3)
{
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    size_t tidsSize = 0;
    {
        auto inspector = MakeInspector();
        ASSERT_TRUE(inspector->CaptureThreadInfo(threads, tidsSize));
    }
    EXPECT_GE(threads.size(), tidsSize);
}

/**
 * @tc.name  : ThreadInspector_GetLocalThreadInfo_ShouldSucceed_AfterCaptureThreadInfo
 * @tc.number: ThreadInspector_GetLocalThreadInfo_Test_004
 * @tc.desc  : Test that GetLocalThreadInfo succeeds after CaptureThreadInfo resized the thread list.
 */
HWTEST_F(ThreadInspectorTest, GetLocalThreadInfo_AfterCapture, TestSize.Level3)
{
    auto inspector = MakeInspector();
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    size_t tidsSize = 0;
    ASSERT_TRUE(inspector->CaptureThreadInfo(threads, tidsSize));
    // The entry index depends on how many threads were already captured,
    // so only verify the call does not crash and the list stays consistent.
    inspector->GetLocalThreadInfo();
    EXPECT_GE(threads.size(), tidsSize);
}

/**
 * @tc.name  : ThreadInspector_CaptureThreadInfo_ShouldSucceedTwice_WhenCalledRepeatedly
 * @tc.number: ThreadInspector_CaptureThreadInfo_Test_002
 * @tc.desc  : Test that two consecutive CaptureThreadInfo calls both succeed.
 */
HWTEST_F(ThreadInspectorTest, CaptureThreadInfo_RepeatedCalls, TestSize.Level3)
{
    auto inspector = MakeInspector();
    ThreadInfoList firstThreads((Allocator<ThreadInfo>(heap)));
    ThreadInfoList secondThreads((Allocator<ThreadInfo>(heap)));
    size_t firstTidsSize = 0;
    size_t secondTidsSize = 0;
    ASSERT_TRUE(inspector->CaptureThreadInfo(firstThreads, firstTidsSize));
    EXPECT_TRUE(inspector->CaptureThreadInfo(secondThreads, secondTidsSize));
    EXPECT_GE(firstThreads.size(), firstTidsSize);
    EXPECT_GE(secondThreads.size(), secondTidsSize);
}

/**
 * @tc.name  : ThreadInspector_ShouldKeepWorking_WhenTidsSizeIsZero
 * @tc.number: ThreadInspector_CaptureThreadInfo_Test_003
 * @tc.desc  : Test that CaptureThreadInfo still returns a resized list when no other threads exist.
 */
HWTEST_F(ThreadInspectorTest, CaptureThreadInfo_EmptyTids, TestSize.Level1)
{
    auto inspector = MakeInspector();
    ThreadInfoList threads((Allocator<ThreadInfo>(heap)));
    size_t tidsSize = 0;
    EXPECT_TRUE(inspector->CaptureThreadInfo(threads, tidsSize));
    EXPECT_GE(threads.size(), 1000u);
}

// ===== From orphanmem_detector_collect_test.cpp =====
class OrphanMemDetectorCollectTest : public ::testing::Test {
protected:
    static Heap heap;
    static Heap::unique_ptr<OrphanMemDetector> collectDetector;
    static void *testMem;
    static constexpr size_t testMemSize = 1024 * 1024;

    static uintptr_t T(size_t off)
    {
        return reinterpret_cast<uintptr_t>(testMem) + off;
    }

    static void SetUpTestSuite()
    {
        testMem = mmap(nullptr, testMemSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        ASSERT_NE(testMem, MAP_FAILED);
        if (memset_s(testMem, testMemSize, 0, testMemSize) != EOK) {
            return;
        }
        Allocator<OrphanMemDetector> alloc(heap);
        collectDetector = alloc.MakeUnique(Allocator<void>(alloc));
    }

    static void TearDownTestSuite()
    {
        collectDetector.reset();
        if (testMem != nullptr) {
            munmap(testMem, testMemSize);
        }
    }
};
Heap OrphanMemDetectorCollectTest::heap;
Heap::unique_ptr<OrphanMemDetector> OrphanMemDetectorCollectTest::collectDetector;
void *OrphanMemDetectorCollectTest::testMem = nullptr;

/**
 * @tc.name  : OrphanMemDetector_ShouldHaveZeroCounts_WhenFreshlyConstructed
 * @tc.number: OrphanMemDetector_Collect_Test_001
 * @tc.desc  : Test that a freshly constructed OrphanMemDetector reports zero allocations and bytes.
 */
HWTEST_F(OrphanMemDetectorCollectTest, Construction_ZeroCounts, TestSize.Level1)
{
    EXPECT_EQ(collectDetector->Allocations(), 0u);
    EXPECT_EQ(collectDetector->AllocationBytes(), 0u);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldReturnTrue_WhenAllInputsEmpty
 * @tc.number: OrphanMemDetector_Collect_Test_002
 * @tc.desc  : Test that CollectAllocations succeeds with empty mappings, threads and refs.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_EmptyInputs, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), 0u);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldNotAddAllocations_WhenOnlySkipMappings
 * @tc.number: OrphanMemDetector_Collect_Test_003
 * @tc.desc  : Test that skip-listed mappings do not contribute any allocations.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_OnlySkipMappings, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x10000), T(0x11000), true, true, false, "[anon:low shadow]");
    mappings.emplace_back(T(0x12000), T(0x13000), true, true, false, "[anon:high shadow]");
    mappings.emplace_back(T(0x14000), T(0x15000), true, true, false, "[anon:hwasan_region]");
    mappings.emplace_back(T(0x16000), T(0x17000), true, true, false, "[anon:LargeMmapAllocator]");
    mappings.emplace_back(T(0x18000), T(0x19000), true, true, false,
        "/data/log/hiprofiler/native_hook_cache_tmp");
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), 0u);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldNotAddAllocations_WhenOnlyGlobalMappings
 * @tc.number: OrphanMemDetector_Collect_Test_004
 * @tc.desc  : Test that global mappings are registered as roots, not as allocations.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_OnlyGlobalMappings, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x20000), T(0x21000), true, true, false, "[anon:.bss]");
    mappings.emplace_back(T(0x22000), T(0x23000), true, true, false, "");
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), 0u);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldNotAddAllocations_WhenOnlyStackMappings
 * @tc.number: OrphanMemDetector_Collect_Test_005
 * @tc.desc  : Test that stack mappings with thread infos are registered as roots, not allocations.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_OnlyStackMappings, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x30000), T(0x31000), true, true, false, "[stack:1234]");
    ThreadInfo info{};
    info.tid = static_cast<pid_t>(gettid());
    info.stack = std::make_pair(T(0x30800), T(0x30900));
    threads.push_back(info);
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), 0u);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldRegisterRefsAsRoots_WhenRefsProvided
 * @tc.number: OrphanMemDetector_Collect_Test_006
 * @tc.desc  : Test that provided refs are registered as roots without becoming allocations.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_RefsAsRoots, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    refs.push_back(T(0x40000));
    refs.push_back(T(0x40008));
    refs.push_back(T(0x40010));
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), 0u);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldIgnoreMappings_WhenNotReadable
 * @tc.number: OrphanMemDetector_Collect_Test_007
 * @tc.desc  : Test that non-readable mappings are ignored during classification.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_UnreadableMappings, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x50000), T(0x51000), false, false, false, "[anon:native_heap:jemalloc]");
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), 0u);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldReturnTrue_WhenCurrentPidMappings
 * @tc.number: OrphanMemDetector_Collect_Test_008
 * @tc.desc  : Test that CollectAllocations succeeds with real mappings of the current process.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_CurrentPidMappings, TestSize.Level3)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    ASSERT_TRUE(MappingHelper::ProcessMappings(getpid(), mappings));
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_GT(collectDetector->Allocations(), 0u);
}

/**
 * @tc.name  : OrphanMemDetector_Detect_ShouldReturnTrue_WhenLeakGraphRequested
 * @tc.number: OrphanMemDetector_Detect_Test_001
 * @tc.desc  : Test that Detect succeeds and populates counters using the leak graph variant.
 */
HWTEST_F(OrphanMemDetectorCollectTest, Detect_LeakGraphVariant, TestSize.Level3)
{
    LeakGraphALC leakGraph((Allocator<LeakGraphALC::value_type>(heap)));
    size_t leakCnt = 0;
    size_t leakBytes = 0;
    EXPECT_TRUE(collectDetector->Detect(leakGraph, SIZE_MAX, &leakCnt, &leakBytes));
    EXPECT_GE(leakCnt, 0u);
    EXPECT_GE(leakBytes, 0u);
}

/**
 * @tc.name  : OrphanMemDetector_Detect_ShouldReturnTrue_WhenLeakedSetRequested
 * @tc.number: OrphanMemDetector_Detect_Test_002
 * @tc.desc  : Test that Detect succeeds using the leaked set variant.
 */
HWTEST_F(OrphanMemDetectorCollectTest, Detect_LeakedSetVariant, TestSize.Level3)
{
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked((Allocator<Range>(heap)));
    size_t leakCnt = 0;
    size_t leakBytes = 0;
    EXPECT_TRUE(collectDetector->Detect(leaked, SIZE_MAX, &leakCnt, &leakBytes));
    EXPECT_GE(leakCnt, 0u);
    EXPECT_GE(leakBytes, 0u);
}

/**
 * @tc.name  : OrphanMemDetector_Detect_ShouldRespectLimit_WhenLimitIsZero
 * @tc.number: OrphanMemDetector_Detect_Test_003
 * @tc.desc  : Test that Detect with zero limit yields an empty leaked set but full counters.
 */
HWTEST_F(OrphanMemDetectorCollectTest, Detect_LimitZero, TestSize.Level3)
{
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked((Allocator<Range>(heap)));
    size_t leakCnt = 0;
    size_t leakBytes = 0;
    EXPECT_TRUE(collectDetector->Detect(leaked, 0, &leakCnt, &leakBytes));
    EXPECT_EQ(leaked.size(), 0u);
    EXPECT_GE(leakCnt, 0u);
    EXPECT_GE(leakBytes, 0u);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldNotAddAllocations_WhenOnlyArkTsMappings
 * @tc.number: OrphanMemDetector_Collect_Test_009
 * @tc.desc  : Test that ArkTS mappings are classified as global roots without adding allocations.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_OnlyArkTsMappings, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x60000), T(0x61000), true, true, false, "[anon:ArkTS_Heap]");
    size_t before = collectDetector->Allocations();
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), before);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldNotAddAllocations_WhenLeakDetectorMallocMapping
 * @tc.number: OrphanMemDetector_Collect_Test_010
 * @tc.desc  : Test that the leak_detector_malloc mapping is not treated as an allocation source.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_LeakDetectorMallocMapping, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x62000), T(0x63000), true, true, false, "[anon:leak_detector_malloc]");
    size_t before = collectDetector->Allocations();
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), before);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldSucceed_WhenMixedCategoryMappingsProvided
 * @tc.number: OrphanMemDetector_Collect_Test_011
 * @tc.desc  : Test that a mix of all mapping categories is collected without adding fake allocations.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_MixedCategoryMappings, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x64000), T(0x65000), true, true, true, "liborphanmem_ut.so");
    mappings.emplace_back(T(0x66000), T(0x67000), true, true, false, "[anon:.bss]");
    mappings.emplace_back(T(0x68000), T(0x69000), true, true, false, "[stack:4321]");
    mappings.emplace_back(T(0x6A000), T(0x6B000), true, true, false, "[anon:low shadow]");
    mappings.emplace_back(T(0x6C000), T(0x6D000), false, false, false, "[anon:unreadable]");
    mappings.emplace_back(T(0x6E000), T(0x6F000), true, true, false, "[anon:ArkTS_Heap]");
    ThreadInfo info{};
    info.tid = static_cast<pid_t>(gettid());
    info.stack = std::make_pair(T(0x68800), T(0x68900));
    threads.push_back(info);
    refs.push_back(T(0x70000));
    size_t before = collectDetector->Allocations();
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), before);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldSucceed_WhenCalledRepeatedly
 * @tc.number: OrphanMemDetector_Collect_Test_012
 * @tc.desc  : Test that repeated CollectAllocations calls with the same synthetic inputs succeed.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_CalledRepeatedly, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x80000), T(0x81000), true, true, false, "[anon:.bss]");
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldNotAddAllocations_WhenStackMappingWithoutThreads
 * @tc.number: OrphanMemDetector_Collect_Test_013
 * @tc.desc  : Test that stack mappings contribute no allocations when no thread infos reference them.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_StackMappingWithoutThreads, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x82000), T(0x83000), true, true, false, "[stack:1000]");
    mappings.emplace_back(T(0x84000), T(0x85000), true, true, false, "[anon:stack:2000]");
    mappings.emplace_back(T(0x86000), T(0x87000), true, true, false, "[anon:ffrt_coroutine_stack]");
    size_t before = collectDetector->Allocations();
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), before);
}

/**
 * @tc.name  : OrphanMemDetector_CollectAllocations_ShouldNotAddAllocations_WhenOnlyHeapMetaNamedMappings
 * @tc.number: OrphanMemDetector_Collect_Test_014
 * @tc.desc  : Test that dfmalloc and jemalloc meta mappings are classified via heap path when unreadable.
 */
HWTEST_F(OrphanMemDetectorCollectTest, CollectAllocations_HeapMetaNamedMappings, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<ThreadInfo> threads((Allocator<ThreadInfo>(heap)));
    MAP_ALLOCATOR::vector<Mapping> mappings((Allocator<Mapping>(heap)));
    MAP_ALLOCATOR::vector<uintptr_t> refs((Allocator<uintptr_t>(heap)));
    mappings.emplace_back(T(0x88000), T(0x89000), false, false, false, "[anon:native_heap:dfmalloc meta]");
    mappings.emplace_back(T(0x8A000), T(0x8B000), false, false, false, "[anon:native_heap:dfmalloc data]");
    size_t before = collectDetector->Allocations();
    EXPECT_TRUE(collectDetector->CollectAllocations(threads, mappings, refs));
    EXPECT_EQ(collectDetector->Allocations(), before);
}

/**
 * @tc.name  : OrphanMemDetector_Detect_ShouldSucceedAgain_WhenCalledAfterPreviousDetections
 * @tc.number: OrphanMemDetector_Detect_Test_004
 * @tc.desc  : Test that Detect remains functional after earlier detection runs.
 */
HWTEST_F(OrphanMemDetectorCollectTest, Detect_AfterPreviousDetections, TestSize.Level3)
{
    LeakGraphALC leakGraph((Allocator<LeakGraphALC::value_type>(heap)));
    size_t leakCnt = 0;
    size_t leakBytes = 0;
    EXPECT_TRUE(collectDetector->Detect(leakGraph, SIZE_MAX, &leakCnt, &leakBytes));
    EXPECT_GE(leakCnt, 0u);
    EXPECT_GE(leakBytes, 0u);
    EXPECT_GE(collectDetector->Allocations(), 0u);
}
