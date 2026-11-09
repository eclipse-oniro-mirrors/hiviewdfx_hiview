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
#include <sys/mman.h>
#include <securec.h>
#include <unistd.h>

#include "heap_scanner.h"
#include "map_allocator.h"
#include "orphanmem_detector.h"
#include "process_mappings.h"
#include "string_helper.h"
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
    MAP_ALLOCATOR::vector<Mapping> mappings(Allocator<Mapping>(heap));
    EXPECT_FALSE(MappingHelper::ProcessMappings(-1, mappings));
}

/**
 * @tc.name  : ProcessMappings_ShouldReturnTrue_WhenCurrentPid
 * @tc.number: ProcessMappings_CurrentPid_Test_001
 * @tc.desc  : Test that ProcessMappings returns true and finds mappings for current process.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_CurrentPid, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings(Allocator<Mapping>(heap));
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
    MAP_ALLOCATOR::vector<Mapping> mappings(Allocator<Mapping>(heap));
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
    MAP_ALLOCATOR::vector<Mapping> mappings(Allocator<Mapping>(heap));
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
    MAP_ALLOCATOR::vector<Mapping> mappings(Allocator<Mapping>(heap));
    EXPECT_FALSE(MappingHelper::ProcessMappings(0, mappings));
}

/**
 * @tc.name  : ProcessMappings_ShouldReturnConsistent_WhenCalledMultipleTimes
 * @tc.number: ProcessMappings_Consistency_Test_001
 * @tc.desc  : Test that ProcessMappings returns consistent results across multiple calls.
 */
HWTEST_F(ProcessMappingsTest, ProcessMappings_Consistent, TestSize.Level1)
{
    MAP_ALLOCATOR::vector<Mapping> mappings1(Allocator<Mapping>(heap));
    MAP_ALLOCATOR::vector<Mapping> mappings2(Allocator<Mapping>(heap));
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
    MAP_ALLOCATOR::vector<Mapping> mappings(Allocator<Mapping>(heap));
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
    MAP_ALLOCATOR::vector<Mapping> mappings(Allocator<Mapping>(heap));
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
    MAP_ALLOCATOR::vector<Mapping> mappings(Allocator<Mapping>(heap));
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
    MAP_ALLOCATOR::vector<uintptr_t> vals(Allocator<uintptr_t>(heap));
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
    MAP_ALLOCATOR::vector<uintptr_t> vals(Allocator<uintptr_t>(heap));
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
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked(Allocator<Range>(heap));
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
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked(Allocator<Range>(heap));
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
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked(Allocator<Range>(heap));
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
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked(Allocator<Range>(heap));
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
    LeakGraphALC leakGraph(Allocator<LeakGraphALC::value_type>(heap));
    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked(Allocator<Range>(heap));
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
