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
#include <map>
#include <set>
#include <string>

#include "constants.h"
#include "faultlog_cppcrash.h"
#include "faultlog_event_ipc.h"
#include "faultlog_freeze.h"
#include "faultlog_info_inner.h"
#include "faultlog_util.h"
#include "hisysevent_c.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
namespace {
using ExpectedParams = std::map<std::string, HiSysEventParamType>;

constexpr int32_t TEST_PID = 1000;
constexpr int32_t TEST_UID = 10000;
constexpr int64_t TEST_TIME = 1700000000;

FaultLogInfo CreateBaseInfo(int32_t faultType)
{
    FaultLogInfo info;
    info.faultLogType = faultType;
    info.pid = TEST_PID;
    info.id = TEST_UID;
    info.module = "com.example.test";
    info.reason = "test_reason";
    info.summary = "test_summary";
    info.logPath = "/data/log/faultlog/test.log";
    info.time = TEST_TIME;
    info.sectionMap["PROCESS_NAME"] = "com.example.test";
    info.sectionMap["SUB_EVENT_TYPE"] = "test_sub";
    info.sectionMap["EVENT_TIME"] = "2026-01-01 00:00:00";
    info.sectionMap["DIAG_INFO"] = "diag";
    info.sectionMap["HIVIEW_LOG_FILE_PATHS"] = "/data/log/hiview.log";
    info.sectionMap["DOMAIN"] = "test_domain";
    info.sectionMap["STRING_ID"] = "test_string_id";
    info.sectionMap[FaultKey::PACKAGE_NAME] = "com.example.test";
    info.sectionMap["MSG"] = "test_msg";
    info.sectionMap[FaultKey::FIRST_FRAME] = "#01 frame";
    info.sectionMap[FaultKey::SECOND_FRAME] = "#02 frame";
    info.sectionMap[FaultKey::LAST_FRAME] = "#03 frame";
    info.sectionMap[FaultKey::FINGERPRINT] = "abc123";
    info.sectionMap[FaultKey::STACK] = "stack trace";
    info.sectionMap[FaultKey::MODULE_VERSION] = "1.0.0";
    info.sectionMap[FaultKey::VERSION_CODE] = "100";
    info.sectionMap[FaultKey::IS_SYSTEM_APP] = "No";
    info.sectionMap[FaultKey::PRE_INSTALL] = "No";
    info.sectionMap[FaultKey::FOREGROUND] = "Yes";
    info.sectionMap[FaultKey::TELEMETRY_ID] = "tel_001";
    info.sectionMap[FaultKey::TRACE_NAME] = "trace_001";
    info.sectionMap[FaultKey::FOCUS_MODE] = "1";
    info.sectionMap[FaultKey::APP_RUNNING_UNIQUE_ID] = "app_001";
    info.sectionMap[FaultKey::TASK_NAME] = "task_001";
    info.sectionMap[FaultKey::THERMAL_LEVEL] = "1";
    info.sectionMap[FaultKey::FREEZE_INFO_PATH] = "/data/log/freeze.log";
    info.sectionMap["LOG_SOURCE"] = "log_src";
    info.sectionMap[FaultKey::PROCESS_LIFETIME] = "100";
    info.sectionMap[FaultKey::DEVICE_RUNNING_TIME] = "200";
    info.sectionMap[FaultKey::HOST_RESOURCE_WARNING] = "TRUE";
    info.sectionMap["HITRACE_TIME"] = "1000";
    info.sectionMap["SYSRQ_TIME"] = "2000";
    info.sectionMap[FaultKey::IS_SIG_ACTION] = "Yes";
    info.sectionMap["SAMPLER_COUNT"] = "10";
    info.sectionMap["LOG_VALIDITY"] = "valid";
    return info;
}

const HiSysEventParam* FindCapturedParam(const std::string& name)
{
    for (const auto& p : g_capturedEvent.params) {
        if (std::string(p.name) == name) {
            return &p;
        }
    }
    return nullptr;
}
}

class StrongValidateTest : public testing::Test {
protected:
    void SetUp() override
    {
        g_capturedEvent.params.clear();
        g_capturedEvent.name.clear();
    }

    void ExpectNoDuplicates() const
    {
        std::set<std::string> names;
        for (const auto& p : g_capturedEvent.params) {
            names.insert(p.name);
        }
        EXPECT_EQ(names.size(), g_capturedEvent.params.size()) << "Duplicate param names found";
    }

    void ExpectParamSet(const ExpectedParams& expected) const
    {
        EXPECT_EQ(g_capturedEvent.params.size(), expected.size())
            << "Param count mismatch";
        for (const auto& [name, type] : expected) {
            const auto* param = FindCapturedParam(name);
            EXPECT_NE(param, nullptr) << "Missing param: " << name;
            if (param != nullptr) {
                EXPECT_EQ(param->t, type) << "Type mismatch for param: " << name;
            }
        }
    }
};

HWTEST_F(StrongValidateTest, AppFreezeParamSet001, TestSize.Level1)
{
    auto info = CreateBaseInfo(FaultLogType::APP_FREEZE);
    FaultLogFreeze::WriteEvent(info);
    EXPECT_EQ(g_capturedEvent.name, "APP_FREEZE");
    ExpectNoDuplicates();
    ExpectParamSet({
        {"PID", HISYSEVENT_INT32}, {"UID", HISYSEVENT_INT32},
        {"MODULE", HISYSEVENT_STRING}, {"REASON", HISYSEVENT_STRING},
        {"PNAME", HISYSEVENT_STRING}, {"VERSION", HISYSEVENT_STRING},
        {"FOREGROUND", HISYSEVENT_STRING},         {"FOCUS_MODE", HISYSEVENT_INT32}, {"HAPPEN_TIME", HISYSEVENT_INT64},
        {"FIRST_FRAME", HISYSEVENT_STRING}, {"SECOND_FRAME", HISYSEVENT_STRING},
        {"LAST_FRAME", HISYSEVENT_STRING}, {"FINGERPRINT", HISYSEVENT_STRING},
        {"STACK", HISYSEVENT_STRING}, {"LOG_PATH", HISYSEVENT_STRING},
        {"SUB_EVENT_TYPE", HISYSEVENT_STRING}, {"EVENT_TIME", HISYSEVENT_STRING},
        {"DIAG_INFO", HISYSEVENT_STRING}, {"HIVIEW_LOG_FILE_PATHS", HISYSEVENT_STRING},
        {"DOMAIN", HISYSEVENT_STRING}, {"STRING_ID", HISYSEVENT_STRING},
        {"PACKAGE_NAME", HISYSEVENT_STRING}, {"PROCESS_NAME", HISYSEVENT_STRING},
        {"MSG", HISYSEVENT_STRING},
        {"SUMMARY", HISYSEVENT_STRING}, {"FAULT_TYPE", HISYSEVENT_STRING},
        {"VERSION_CODE", HISYSEVENT_STRING}, {"IS_SYSTEM_APP", HISYSEVENT_STRING},
        {"PRE_INSTALL", HISYSEVENT_STRING}, {"TELEMETRY_ID", HISYSEVENT_STRING},
        {"TRACE_NAME", HISYSEVENT_STRING}, {"APP_RUNNING_UNIQUE_ID", HISYSEVENT_STRING},
        {"THERMAL_LEVEL", HISYSEVENT_STRING},
        {"SAMPLER_COUNT", HISYSEVENT_INT32}, {"LOG_VALIDITY", HISYSEVENT_STRING},
        {"FREEZE_INFO_PATH", HISYSEVENT_STRING},
    });
}

HWTEST_F(StrongValidateTest, SysFreezeParamSet001, TestSize.Level1)
{
    auto info = CreateBaseInfo(FaultLogType::SYS_FREEZE);
    FaultLogFreeze::WriteEvent(info);
    EXPECT_EQ(g_capturedEvent.name, "SYS_FREEZE");
    ExpectNoDuplicates();
    ExpectParamSet({
        {"PID", HISYSEVENT_INT32}, {"UID", HISYSEVENT_INT32},
        {"MODULE", HISYSEVENT_STRING}, {"REASON", HISYSEVENT_STRING},
        {"PNAME", HISYSEVENT_STRING}, {"VERSION", HISYSEVENT_STRING},
        {"FOREGROUND", HISYSEVENT_STRING},         {"FOCUS_MODE", HISYSEVENT_INT32}, {"HAPPEN_TIME", HISYSEVENT_INT64},
        {"FIRST_FRAME", HISYSEVENT_STRING}, {"SECOND_FRAME", HISYSEVENT_STRING},
        {"LAST_FRAME", HISYSEVENT_STRING}, {"FINGERPRINT", HISYSEVENT_STRING},
        {"STACK", HISYSEVENT_STRING}, {"LOG_PATH", HISYSEVENT_STRING},
        {"SUB_EVENT_TYPE", HISYSEVENT_STRING}, {"EVENT_TIME", HISYSEVENT_STRING},
        {"DIAG_INFO", HISYSEVENT_STRING}, {"HIVIEW_LOG_FILE_PATHS", HISYSEVENT_STRING},
        {"DOMAIN", HISYSEVENT_STRING}, {"STRING_ID", HISYSEVENT_STRING},
        {"PACKAGE_NAME", HISYSEVENT_STRING}, {"PROCESS_NAME", HISYSEVENT_STRING},
        {"MSG", HISYSEVENT_STRING},
        {"VERSION_CODE", HISYSEVENT_STRING}, {"APP_RUNNING_UNIQUE_ID", HISYSEVENT_STRING},
        {"TASK_NAME", HISYSEVENT_STRING},
        {"LIFETIME", HISYSEVENT_INT64}, {"DEVICE_RUNNING_TIME", HISYSEVENT_INT64},
        {"HOST_RESOURCE_WARNING", HISYSEVENT_STRING},
    });
}

HWTEST_F(StrongValidateTest, FreezeWarningParamSet001, TestSize.Level1)
{
    auto info = CreateBaseInfo(FaultLogType::SYS_WARNING);
    FaultLogFreeze::WriteEvent(info);
    ExpectNoDuplicates();
    ExpectParamSet({
        {"PID", HISYSEVENT_INT32}, {"UID", HISYSEVENT_INT32},
        {"MODULE", HISYSEVENT_STRING}, {"REASON", HISYSEVENT_STRING},
        {"PNAME", HISYSEVENT_STRING}, {"VERSION", HISYSEVENT_STRING},
        {"FOREGROUND", HISYSEVENT_STRING}, {"FOCUS_MODE", HISYSEVENT_INT32},
        {"HAPPEN_TIME", HISYSEVENT_INT64},
        {"FIRST_FRAME", HISYSEVENT_STRING}, {"SECOND_FRAME", HISYSEVENT_STRING},
        {"LAST_FRAME", HISYSEVENT_STRING}, {"FINGERPRINT", HISYSEVENT_STRING},
        {"STACK", HISYSEVENT_STRING}, {"LOG_PATH", HISYSEVENT_STRING},
        {"FAULT_TYPE", HISYSEVENT_STRING}, {"SUMMARY", HISYSEVENT_STRING},
        {"PRE_INSTALL", HISYSEVENT_STRING},
        {"HITRACE_TIME", HISYSEVENT_INT64},
        {"SYSRQ_TIME", HISYSEVENT_INT64}, {"TASK_NAME", HISYSEVENT_STRING},
    });
}

HWTEST_F(StrongValidateTest, CppCrashParamSet001, TestSize.Level1)
{
    auto info = CreateBaseInfo(FaultLogType::CPP_CRASH);
    FaultLogCppCrash::WriteEvent(info);
    EXPECT_EQ(g_capturedEvent.name, "CPP_CRASH");
    ExpectNoDuplicates();
    ExpectParamSet({
        {"MODULE", HISYSEVENT_STRING}, {"VERSION", HISYSEVENT_STRING},
        {"VERSION_CODE", HISYSEVENT_STRING}, {"REASON", HISYSEVENT_STRING},
        {"PID", HISYSEVENT_INT32}, {"UID", HISYSEVENT_INT32},
        {"SUMMARY", HISYSEVENT_STRING}, {"FAULT_TYPE", HISYSEVENT_STRING},
        {"LOG_PATH", HISYSEVENT_STRING}, {"FINGERPRINT", HISYSEVENT_STRING},
        {"HAPPEN_TIME", HISYSEVENT_INT64}, {"LIFETIME", HISYSEVENT_INT64},
        {"PNAME", HISYSEVENT_STRING},
        {"FIRST_FRAME", HISYSEVENT_STRING}, {"SECOND_FRAME", HISYSEVENT_STRING},
        {"LAST_FRAME", HISYSEVENT_STRING}, {"PRE_INSTALL", HISYSEVENT_STRING},
        {"FOREGROUND", HISYSEVENT_STRING},
        {"FOCUS_MODE", HISYSEVENT_INT32}, {"APP_RUNNING_UNIQUE_ID", HISYSEVENT_STRING},
        {"IS_SYSTEM_APP", HISYSEVENT_STRING}, {"FG", HISYSEVENT_INT32},
        {"LOG_SOURCE", HISYSEVENT_STRING},
    });
}
} // namespace HiviewDFX
} // namespace OHOS
