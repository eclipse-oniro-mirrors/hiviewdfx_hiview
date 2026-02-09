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

#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TEST_TRACE_STATE_MACHINE_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TEST_TRACE_STATE_MACHINE_H
#include <charconv>

#include "singleton.h"
#include "trace_common.h"
#include "time_util.h"

namespace OHOS::HiviewDFX {
const std::string TEST_DB_PATH = "/data/test/trace_db/";
const std::string TEST_SRC_PATH = "/data/test/trace_src/test_traces/";
const std::string TEST_CONFIG_PATH = "/data/test/trace_config/";
const std::string TEST_SHARED_PATH = "/data/test/trace_shared/";
const std::string TEST_SHARED_TEMP_PATH = "/data/test/trace_shared/temp";
const std::string TEST_SPECIAL_PATH = "/data/test/trace_special/";
const std::string TEST_TELEMETRY_PATH = "/data/test/trace_telemetry/";
const std::string TRACE_TEST_ID1 = "trace_20170928220220@75724-2015";
const std::string TRACE_TEST_ID2 = "trace_20170928220222@75726-992";
const std::string TRACE_TEST_ID3 = "trace_20170928223217@77520-2883";
const std::string TRACE_TEST_ID4 = "trace_20170928223909@77932-4731";
const std::string TRACE_TEST_ID5 = "trace_20170928223913@77937-148363";
const uint32_t FILE_SIZE_DEFAULT = 200 * 1024;
const std::string TRACE_TEST_SRC1 = TEST_SRC_PATH + TRACE_TEST_ID1 + ".sys";
const std::string TRACE_TEST_SRC2 = TEST_SRC_PATH + TRACE_TEST_ID2 + ".sys";
const std::string TRACE_TEST_SRC3 = TEST_SRC_PATH + TRACE_TEST_ID3 + ".sys";
const std::string TRACE_TEST_SRC4 = TEST_SRC_PATH + TRACE_TEST_ID4 + ".sys";
const std::string TRACE_TEST_SRC5 = TEST_SRC_PATH + TRACE_TEST_ID5 + ".sys";
const std::string TOTAL = "Total";
constexpr double TOLERATION = 1.1;

inline bool IsContainSrcTrace(const std::string& tracePath, const std::string& srcPath)
{
    std::vector<std::string> files;
    FileUtil::GetDirFiles(tracePath, files);
    for (auto& file : files) {
        if (file.find(srcPath) != std::string::npos) {
            return true;
        }
    }
    return false;
}

inline size_t GetDirFileCount(const std::string& dirPath)
{
    std::vector<std::string> files;
    FileUtil::GetDirFiles(dirPath, files);
    return files.size();
}

const std::map<std::string, int64_t> FLOW_CONTROL_MAP {
    {CallerName::XPERF, 500}, // telemetry trace threshold
    {CallerName::XPOWER, 500},
    {CallerName::RELIABILITY, 500},
    {TOTAL, 1000} // telemetry total trace threshold
};

inline void CreateTraceFile(const std::string& traceName)
{
    std::ofstream file(traceName, std::ios::out | std::ios::binary);
    if (!file) {
        std::cout << traceName << " create failed" << std::endl;
        return;
    }
    std::string data(FILE_SIZE_DEFAULT, 'A');
    file.write(data.data(), FILE_SIZE_DEFAULT);
    file.close();
}

inline UCollectClient::AppCaller CreateAppCaller(int32_t uid, int32_t pid, uint64_t happendTime)
{
    UCollectClient::AppCaller appCaller;
    appCaller.bundleVersion = "2.0.1";
    appCaller.uid = uid;
    appCaller.pid = pid;
    appCaller.happenTime = happendTime;
    return appCaller;
}

inline AppEventTask CreateAppEventTask(int32_t uid, int32_t pid, uint64_t happendTime)
{
    AppEventTask appEventTask;
    appEventTask.pid_ = pid;
    appEventTask.uid_ = uid;
    uint64_t happenTimeInSecond = happendTime / TimeUtil::SEC_TO_MILLISEC;
    std::string date = TimeUtil::TimestampFormatToDate(happenTimeInSecond, "%Y%m%d");
    int64_t dateNum = 0;
    std::from_chars(date.c_str(), date.c_str() + date.size(), dateNum);
    appEventTask.taskDate_ = dateNum;
    return appEventTask;
}

class MockTraceStateMachine : public DelayedRefSingleton<MockTraceStateMachine> {
public:
    TraceRet DumpTraceAsync(const DumpTraceArgs &args, int64_t fileSizeLimit,
        TraceRetInfo &info, const DumpTraceCallback &callback)
    {
        info = info_;
        if (info_.fileSize > static_cast<int64_t>(static_cast<double>(fileSizeLimit) * TOLERATION)) {
            info.isOverflowControl = true;
        }
        callback_ = callback;
        return TraceRet(info.errorCode);
    }

    DumpTraceCallback GetAsyncCallback()
    {
        return callback_;
    }

    void SetTraceInfo(const TraceRetInfo &info)
    {
        info_ = info;
    }

    std::pair<int32_t, uint64_t> GetCurrentAppInfo()
    {
        return {appid_, taskBeginTime_};
    }

    void SetCurrentAppPid(int32_t appid)
    {
        appid_ = appid;
        taskBeginTime_ = TimeUtil::GetMilliseconds();
    }

private:
    DumpTraceCallback callback_;
    TraceRetInfo info_ = {};
    int32_t appid_ = 0;
    uint64_t taskBeginTime_ = 0;
};
}

#endif
