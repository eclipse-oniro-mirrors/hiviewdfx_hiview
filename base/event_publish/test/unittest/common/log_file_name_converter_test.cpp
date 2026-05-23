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
#include <string>

#include "log_file_name_converter.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class LogFileNameConverterTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase();
    static void TearDownTestCase();
};

void LogFileNameConverterTest::SetUpTestCase() {}

void LogFileNameConverterTest::TearDownTestCase() {}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameJsHeap001
 * @tc.desc: "memleak-js-com.example.app-123-20260522113937.rawheap, lose uid
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameJsHeap001, TestSize.Level1)
{
    std::string oldFileName = "memleak-js-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "js_heap";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameJsHeap002
 * @tc.desc: "memleak-js-com.example.app-123-2020121-20260522113937.rawheap, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameJsHeap002, TestSize.Level1)
{
    std::string oldFileName = "memleak-js-com.example.app-123-2020121-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "js_heap_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameJsHeap003
 * @tc.desc: "memleak-js-com.example.app-123-2020121-20260522113937.rawheap, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameJsHeap003, TestSize.Level1)
{
    std::string oldFileName = "memleak-js-com.example.app-123-2020121-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "js_heap";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_js_heap.rawheap");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssKernelSmaps001
 * @tc.desc: "memleak-kernel-0-20260522113937, lose processName
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssKernelSmaps001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssKernelSmaps002
 * @tc.desc: "memleak-kernel-com.example.app-0-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssKernelSmaps002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssKernelSmaps003
 * @tc.desc: "memleak-kernel-com.example.app-0-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssKernelSmaps003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_rss_smaps.log");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssJsheap001
 * @tc.desc: "memleak-js-com.example.app-20260522113937.rawheap, lose pid
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssJsheap001, TestSize.Level1)
{
    std::string oldFileName = "memleak-js-com.example.app-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssJsheap002
 * @tc.desc: "memleak-js-com.example.app-123-20260522113937.rawheap, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssJsheap002, TestSize.Level1)
{
    std::string oldFileName = "memleak-js-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssJsheap003
 * @tc.desc: "memleak-js-com.example.app-123-20260522113937.rawheap, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssJsheap003, TestSize.Level1)
{
    std::string oldFileName = "memleak-js-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_rss_jsheap.rawheap");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssNativeSmaps001
 * @tc.desc: "memleak-native-hiapp-com.example.app-smaps.txt, lose pid
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssNativeSmaps001, TestSize.Level1)
{
    std::string oldFileName = "memleak-native-hiapp-com.example.app-smaps.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssNativeSmaps002
 * @tc.desc: "memleak-native-hiapp-com.example.app-1231smaps.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssNativeSmaps002, TestSize.Level1)
{
    std::string oldFileName = "memleak-native-hiapp-com.example.app-123-smaps.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssNativeSmaps003
 * @tc.desc: "memleak-native-hiapp-com.example.app-1231smaps.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssNativeSmaps003, TestSize.Level1)
{
    std::string oldFileName = "memleak-native-hiapp-com.example.app-123-smaps.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.find("RESOURCE_OVERLIMIT_") != std::string::npos);
    ASSERT_TRUE(newFileName.find("123") != std::string::npos);
    ASSERT_TRUE(newFileName.find("_rss_smaps.log") != std::string::npos);
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssAshmem001
 * @tc.desc: "memleak-kernel-hiapp-com.example.app-123-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssAshmem001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-hiapp-com.example.app-123-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssAshmem002
 * @tc.desc: "memleak-kernel-hiapp-com.example.app-0+-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssAshmem002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-hiapp-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssAshmem003
 * @tc.desc: "memleak-kernel-hiapp-com.example.app-0+-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssAshmem003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-hiapp-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_rss_ashmem.htrace");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssKotlin001
 * @tc.desc: "memleak-kotlin-com.example.app-abc-20260522113937.kdump, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssKotlin001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kotlin-com.example.app-abc-20260522113937.kdump";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssKotlin002
 * @tc.desc: "memleak-kotlin-com.example.app-123-20260522113937.kdump, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssKotlin002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kotlin-com.example.app-123-20260522113937.kdump";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssKotlin003
 * @tc.desc: "memleak-kotlin-com.example.app-123-20260522113937.kdump, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssKotlin003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kotlin-com.example.app-123-20260522113937.kdump";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_rss_kotlin.kdump");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssJsvm001
 * @tc.desc: "memleak-jsvm-com.example.app-abc-20260522113937.rawheap, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssJsvm001, TestSize.Level1)
{
    std::string oldFileName = "memleak-jsvm-com.example.app-abc-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssJsvm002
 * @tc.desc: "memleak-jsvm-com.example.app-123-20260522113937.rawheap, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssJsvm002, TestSize.Level1)
{
    std::string oldFileName = "memleak-jsvm-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssJsvm003
 * @tc.desc: "memleak-jsvm-com.example.app-123-20260522113937.rawheap, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssJsvm003, TestSize.Level1)
{
    std::string oldFileName = "memleak-jsvm-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_rss_jsvm.rawheap");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssArkwebv8001
 * @tc.desc: "memleak-arkweb_v8-com.example.app-abc-20260522113937.rawheap, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssArkwebv8001, TestSize.Level1)
{
    std::string oldFileName = "memleak-arkweb_v8-com.example.app-abc-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssArkwebv8002
 * @tc.desc: "memleak-arkweb_v8-com.example.app-123-20260522113937.rawheap, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssArkwebv8002, TestSize.Level1)
{
    std::string oldFileName = "memleak-arkweb_v8-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameRssArkwebv8003
 * @tc.desc: "memleak-arkweb_v8-com.example.app-123-20260522113937.rawheap, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameRssArkwebv8003, TestSize.Level1)
{
    std::string oldFileName = "memleak-arkweb_v8-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "rss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_rss_arkwebv8.rawheap");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssSmaps001
 * @tc.desc: "memleak-native-com.example.app-abc-smaps.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssSmaps001, TestSize.Level1)
{
    std::string oldFileName = "memleak-native-com.example.app-abc-smaps.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssSmaps002
 * @tc.desc: "memleak-native-com.example.app-123-smaps.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssSmaps002, TestSize.Level1)
{
    std::string oldFileName = "memleak-native-com.example.app-123-smaps.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssSmaps003
 * @tc.desc: "memleak-native-com.example.app-123-smaps.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssSmaps003, TestSize.Level1)
{
    std::string oldFileName = "memleak-native-com.example.app-123-smaps.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.find("RESOURCE_OVERLIMIT_") != std::string::npos);
    ASSERT_TRUE(newFileName.find("123") != std::string::npos);
    ASSERT_TRUE(newFileName.find("_extpss_smaps.log") != std::string::npos);
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssProcinfo001
 * @tc.desc: "memleak-kernel-com.example.app-123-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssProcinfo001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-hiapp-com.example.app-123-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssProcinfo002
 * @tc.desc: "memleak-kernel-com.example.app-0-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssProcinfo002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-hiapp-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssProcinfo003
 * @tc.desc: "memleak-kernel-com.example.app-0-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssProcinfo003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-hiapp-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_extpss_procinfo.log");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssNative001
 * @tc.desc: "memleak-native-com.example.app-abc-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssNative001, TestSize.Level1)
{
    std::string oldFileName = "memleak-native-com.example.app-abc-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssNative002
 * @tc.desc: "memleak-native-com.example.app-123-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssNative002, TestSize.Level1)
{
    std::string oldFileName = "memleak-native-com.example.app-123-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssNative003
 * @tc.desc: "memleak-native-com.example.app-123-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssNative003, TestSize.Level1)
{
    std::string oldFileName = "memleak-native-com.example.app-123-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_extpss_native.htrace");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssGpu001
 * @tc.desc: "memleak-kernel-gpu-abc-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssGpu001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-gpu-com.example.app-abc-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssGpu002
 * @tc.desc: "memleak-kernel-gpu-0-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssGpu002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-gpu-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssGpu003
 * @tc.desc: "memleak-kernel-gpu-0-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssGpu003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-gpu-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_extpss_gpu.htrace");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssDma001
 * @tc.desc: "memleak-kernel-ion-abc-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssDma001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-ion-abc-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssDma002
 * @tc.desc: "memleak-kernel-ion-0-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssDma002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-ion-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssDma003
 * @tc.desc: "memleak-kernel-ion-0-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssDma003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-ion-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_extpss_dma.htrace");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssJsheap001
 * @tc.desc: "memleak-js-com.example.app-abc-20260522113937.rawheap, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssJsheap001, TestSize.Level1)
{
    std::string oldFileName = "memleak-js-com.example.app-abc-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssJsheap002
 * @tc.desc: "memleak-js-com.example.app-123-20260522113937.rawheap, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssJsheap002, TestSize.Level1)
{
    std::string oldFileName = "memleak-js-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssJsheap003
 * @tc.desc: "memleak-js-com.example.app-123-20260522113937.rawheap, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssJsheap003, TestSize.Level1)
{
    std::string oldFileName = "memleak-js-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_extpss_jsheap.rawheap");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssAshmem001
 * @tc.desc: "memleak-kernel-ashmem-abc-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssAshmem001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-ashmem-abc-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssAshmem002
 * @tc.desc: "memleak-kernel-ashmem-0-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssAshmem002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-ashmem-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssAshmem003
 * @tc.desc: "memleak-kernel-ashmem-0-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssAshmem003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-ashmem-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_extpss_ashmem.htrace");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssKotlin001
 * @tc.desc: "memleak-kotlin-com.example.app-abc-20260522113937.kdump, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssKotlin001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kotlin-com.example.app-abc-20260522113937.kdump";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssKotlin002
 * @tc.desc: "memleak-kotlin-com.example.app-123-20260522113937.kdump, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssKotlin002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kotlin-com.example.app-123-20260522113937.kdump";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssKotlin003
 * @tc.desc: "memleak-kotlin-com.example.app-123-20260522113937.kdump, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssKotlin003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kotlin-com.example.app-123-20260522113937.kdump";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_extpss_kotlin.kdump");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssJsvm001
 * @tc.desc: "memleak-jsvm-com.example.app-abc-20260522113937.rawheap, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssJsvm001, TestSize.Level1)
{
    std::string oldFileName = "memleak-jsvm-com.example.app-abc-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssJsvm002
 * @tc.desc: "memleak-jsvm-com.example.app-123-20260522113937.rawheap, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssJsvm002, TestSize.Level1)
{
    std::string oldFileName = "memleak-jsvm-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssJsvm003
 * @tc.desc: "memleak-jsvm-com.example.app-123-20260522113937.rawheap, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssJsvm003, TestSize.Level1)
{
    std::string oldFileName = "memleak-jsvm-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_extpss_jsvm.rawheap");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssArkwebv8001
 * @tc.desc: "memleak-arkweb_v8-com.example.app-abc-20260522113937.rawheap, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssArkwebv8001, TestSize.Level1)
{
    std::string oldFileName = "memleak-arkweb_v8-com.example.app-abc-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssArkwebv8002
 * @tc.desc: "memleak-arkweb_v8-com.example.app-123-20260522113937.rawheap, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssArkwebv8002, TestSize.Level1)
{
    std::string oldFileName = "memleak-arkweb_v8-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameExtpssArkwebv8003
 * @tc.desc: "memleak-arkweb_v8-com.example.app-123-20260522113937.rawheap, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameExtpssArkwebv8003, TestSize.Level1)
{
    std::string oldFileName = "memleak-arkweb_v8-com.example.app-123-20260522113937.rawheap";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "pss_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_extpss_arkwebv8.rawheap");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameDmaLog001
 * @tc.desc: "memleak-kernel-com.example.app-abc-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameDmaLog001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-abc-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "ion_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameDmaLog002
 * @tc.desc: "memleak-kernel-com.example.app-0-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameDmaLog002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "ion_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameDmaLog003
 * @tc.desc: "memleak-kernel-com.example.app-0-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameDmaLog003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "ion_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_dma.log");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameDmaHtrace001
 * @tc.desc: "memleak-kernel-com.example.app-abc-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameDmaHtrace001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-abc-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "ion_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameDmaHtrace002
 * @tc.desc: "memleak-kernel-com.example.app-123-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameDmaHtrace002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-123-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "ion_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameDmaHtrace003
 * @tc.desc: "memleak-kernel-com.example.app-123-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameDmaHtrace003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-123-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "ion_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_dma.htrace");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameGpuLog001
 * @tc.desc: "memleak-kernel-com.example.app-abc-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameGpuLog001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-abc-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "gpu_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameGpuLog002
 * @tc.desc: "memleak-kernel-com.example.app-0-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameGpuLog002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "gpu_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameGpuLog003
 * @tc.desc: "memleak-kernel-com.example.app-0-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameGpuLog003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-0-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "gpu_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_gpu.log");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameGpuHtrace001
 * @tc.desc: "memleak-kernel-com.example.app-abc-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameGpuHtrace001, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-abc-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "gpu_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameGpuHtrace002
 * @tc.desc: "memleak-kernel-com.example.app-123-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameGpuHtrace002, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-123-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "gpu_memory_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameGpuHtrace003
 * @tc.desc: "memleak-kernel-com.example.app-123-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameGpuHtrace003, TestSize.Level1)
{
    std::string oldFileName = "memleak-kernel-com.example.app-123-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "gpu_memory";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_gpu.htrace");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameFdLog001
 * @tc.desc: "fdleak-com.example.app-abc-log-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameFdLog001, TestSize.Level1)
{
    std::string oldFileName = "fdleak-com.example.app-abc-log-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "fd";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameFdLog002
 * @tc.desc: "fdleak-com.example.app-123-log-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameFdLog002, TestSize.Level1)
{
    std::string oldFileName = "fdleak-com.example.app-123-log-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "fd_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameFdLog003
 * @tc.desc: "fdleak-com.example.app-123-log-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameFdLog003, TestSize.Level1)
{
    std::string oldFileName = "fdleak-com.example.app-123-log-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "fd";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_fd.log");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameFdHtrace001
 * @tc.desc: "fdleak-hiapp-com.example.app-abc-profiler-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameFdHtrace001, TestSize.Level1)
{
    std::string oldFileName = "fdleak-hiapp-com.example.app-abc-profiler-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "fd";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameFdHtrace002
 * @tc.desc: "fdleak-hiapp-com.example.app-123-profiler-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameFdHtrace002, TestSize.Level1)
{
    std::string oldFileName = "fdleak-hiapp-com.example.app-123-profiler-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "fd_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameFdHtrace003
 * @tc.desc: "fdleak-hiapp-com.example.app-123-profiler-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameFdHtrace003, TestSize.Level1)
{
    std::string oldFileName = "fdleak-hiapp-com.example.app-123-profiler-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "fd";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_fd.htrace");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameThreadLog001
 * @tc.desc: "threadleak-com.example.app-abc-log-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameThreadLog001, TestSize.Level1)
{
    std::string oldFileName = "threadleak-com.example.app-abc-log-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "thread";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameThreadLog002
 * @tc.desc: "threadleak-com.example.app-123-log-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameThreadLog002, TestSize.Level1)
{
    std::string oldFileName = "threadleak-com.example.app-123-log-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "thread_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameThreadLog003
 * @tc.desc: "threadleak-com.example.app-123-log-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameThreadLog003, TestSize.Level1)
{
    std::string oldFileName = "threadleak-com.example.app-123-log-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "thread";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_thread.log");
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameThreadHtrace001
 * @tc.desc: "threadleak-hiapp-com.example.app-abc-profiler-20260522113937.txt, wrong format
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameThreadHtrace001, TestSize.Level1)
{
    std::string oldFileName = "threadleak-hiapp-com.example.app-abc-profiler-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "thread";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameThreadHtrace002
 * @tc.desc: "threadleak-hiapp-com.example.app-123-profiler-20260522113937.txt, error resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameThreadHtrace002, TestSize.Level1)
{
    std::string oldFileName = "threadleak-hiapp-com.example.app-123-profiler-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "thread_error";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName.empty());
}

/**
 * @tc.name: ConvertLogFileName ConvertLogFileNameThreadHtrace003
 * @tc.desc: "threadleak-hiapp-com.example.app-123-profiler-20260522113937.txt, true resouceType
 * @tc.type: FUNC
*/
HWTEST_F(LogFileNameConverterTest, ConvertLogFileNameThreadHtrace003, TestSize.Level1)
{
    std::string oldFileName = "threadleak-hiapp-com.example.app-123-profiler-20260522113937.txt";
    std::string newFileName;
    int pid = 123;
    std::string resourceType = "thread";
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    ASSERT_TRUE(newFileName == "RESOURCE_OVERLIMIT_1779421177000_123_thread.htrace");
}