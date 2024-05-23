/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include <climits>
#include <gtest/gtest.h>
#include <iostream>

#include "file_util.h"
#include "trace_collector.h"
#include "trace_manager.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

namespace {
TraceManager g_traceManager;
constexpr uint32_t TIME_0S = 0;
constexpr uint32_t TIME_10S = 10;
constexpr uint32_t TIME_20S = 20;
constexpr uint32_t TIME_30S = 30;
}

class TraceCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: TraceCollectorTest001
 * @tc.desc: used to test TraceCollector for xperf dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest001, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    TraceCollector::Caller caller = TraceCollector::Caller::XPERF;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    g_traceManager.OpenSnapshotTrace(tagGroups);
    sleep(10);
    std::cout << "caller : " << caller << std::endl;
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTrace(caller);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultDumpTrace.data.size() >= 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest002
 * @tc.desc: used to test TraceCollector for xpower dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest002, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    TraceCollector::Caller caller = TraceCollector::Caller::XPOWER;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    g_traceManager.OpenSnapshotTrace(tagGroups);
    sleep(10);
    std::cout << "caller : " << caller << std::endl;
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTrace(caller);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultDumpTrace.data.size() >= 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest003
 * @tc.desc: used to test TraceCollector for reliability dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest003, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    TraceCollector::Caller caller = TraceCollector::Caller::RELIABILITY;
    g_traceManager.OpenSnapshotTrace(tagGroups);
    sleep(10);
    std::cout << "caller : " << caller << std::endl;
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTrace(caller);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultDumpTrace.data.size() >= 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest004
 * @tc.desc: used to test TraceCollector for command
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest004, TestSize.Level1)
{
    const std::string args = "tags:sched clockType:boot bufferSize:1024 overwrite:1";
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    ASSERT_TRUE(g_traceManager.OpenRecordingTrace(args) == 0);

    CollectResult<int32_t> resultTraceOn = collector->TraceOn();
    std::cout << "collect TraceOn result " << resultTraceOn.retCode << std::endl;
    ASSERT_TRUE(resultTraceOn.retCode == UcError::SUCCESS);
    sleep(10);
    CollectResult<std::vector<std::string>> resultTraceOff = collector->TraceOff();
    std::vector<std::string> items = resultTraceOff.data;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect TraceOff result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultTraceOff.retCode == UcError::SUCCESS);
    ASSERT_TRUE(resultTraceOff.data.size() > 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest005
 * @tc.desc: used to test TraceCollector for BetaClub dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest005, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    TraceCollector::Caller caller = TraceCollector::Caller::BETACLUB;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    g_traceManager.OpenSnapshotTrace(tagGroups);
    sleep(10);
    std::cout << "caller : " << caller << std::endl;
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTrace(caller);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultDumpTrace.data.size() >= 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest006
 * @tc.desc: used to test TraceCollector for Other dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest006, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    TraceCollector::Caller caller = TraceCollector::Caller::OTHER;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    g_traceManager.OpenSnapshotTrace(tagGroups);
    sleep(10);
    std::cout << "caller : " << caller << std::endl;
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTrace(caller);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultDumpTrace.data.size() >= 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest007
 * @tc.desc: used to test g_traceManager
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest007, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    g_traceManager.OpenSnapshotTrace(tagGroups);
    const std::string args = "tags:sched clockType:boot bufferSize:1024 overwrite:1";
    ASSERT_TRUE(g_traceManager.OpenRecordingTrace(args) == 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest008
 * @tc.desc: used to test g_traceManager
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest008, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    g_traceManager.OpenSnapshotTrace(tagGroups);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest009
 * @tc.desc: used to test g_traceManager
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest009, TestSize.Level1)
{
    const std::string args = "tags:sched clockType:boot bufferSize:1024 overwrite:1";
    ASSERT_TRUE(g_traceManager.OpenRecordingTrace(args) == 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest010
 * @tc.desc: used to test g_traceManager
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest010, TestSize.Level1)
{
    const std::string args = "tags:sched clockType:boot bufferSize:1024 overwrite:1";
    ASSERT_TRUE(g_traceManager.OpenRecordingTrace(args) == 0);
    ASSERT_TRUE(g_traceManager.RecoverTrace() == 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest011
 * @tc.desc: used to test g_traceManager
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest011, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    g_traceManager.OpenSnapshotTrace(tagGroups);
    ASSERT_TRUE(g_traceManager.RecoverTrace() == 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest0012
 * @tc.desc: used to test TraceCollector for Develop dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest012, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    TraceCollector::Caller caller = TraceCollector::Caller::DEVELOP;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    g_traceManager.OpenSnapshotTrace(tagGroups);
    sleep(10);
    std::cout << "caller : " << caller << std::endl;
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTrace(caller);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultDumpTrace.data.size() >= 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest0013
 * @tc.desc: used to test trace file in /share is zipped
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest013, TestSize.Level1)
{
    std::vector<std::string> traceFiles;
    FileUtil::GetDirFiles("/data/log/hiview/unified_collection/trace/share/", traceFiles, false);
    for (auto &path : traceFiles) {
        if (path.find("temp") == std::string::npos) {
            std::cout << "trace in share path: " << path.c_str() << std::endl;
            ASSERT_TRUE(path.find("zip") != std::string::npos);
        }
    }
}

/**
 * @tc.name: TraceCollectorTest0014
 * @tc.desc: test interface DumpTraceWithDuration for 0 second
 * @tc.type: FUNC
 */
HWTEST_F(TraceCollectorTest, TraceCollectorTest014, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    TraceCollector::Caller caller = TraceCollector::Caller::DEVELOP;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    g_traceManager.OpenSnapshotTrace(tagGroups);
    sleep(TIME_10S);
    std::cout << "caller : " << caller << std::endl;
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTraceWithDuration(caller, TIME_0S);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultDumpTrace.retCode != UcError::SUCCESS);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest0015
 * @tc.desc: test interface DumpTraceWithDuration for 20 second
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest015, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    TraceCollector::Caller caller = TraceCollector::Caller::DEVELOP;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    g_traceManager.OpenSnapshotTrace(tagGroups);
    sleep(TIME_30S);
    std::cout << "caller : " << caller << std::endl;
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTraceWithDuration(caller, TIME_20S);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultDumpTrace.data.size() >= 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}

/**
 * @tc.name: TraceCollectorTest0016
 * @tc.desc: test interface DumpTraceWithDuration for UINT32_MAX
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest016, TestSize.Level1)
{
    const std::vector<std::string> tagGroups = {"scene_performance"};
    TraceCollector::Caller caller = TraceCollector::Caller::DEVELOP;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    g_traceManager.OpenSnapshotTrace(tagGroups);
    sleep(TIME_30S);
    std::cout << "caller : " << caller << std::endl;
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTraceWithDuration(caller, UINT32_MAX);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    ASSERT_TRUE(resultDumpTrace.data.size() >= 0);
    ASSERT_TRUE(g_traceManager.CloseTrace() == 0);
}
