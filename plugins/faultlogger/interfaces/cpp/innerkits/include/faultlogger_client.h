/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_FAULTLOGGER_CLIENT_INTERFACE_H
#define HIVIEW_FAULTLOGGER_CLIENT_INTERFACE_H
#include <cstdint>
#include <map>
#include <string>
#include "faultlog_query_result.h"
#include "faultlogger_client_msg.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Add fault log
 *
 * The C style of API is used to save fault information to file
 * under the /data/log/fautlog/faultlogger directory and event database
 *
 * @param info  structure containing information about fault
*/
void AddFaultLog(FaultLogInfoInner* info);
/**
 * @brief report cpp crash event to Hiview
 *
 * report cpp crash event to Hiview and save it to event database
 *
 * @param info  structure containing information about fault
*/
void ReportCppCrashEvent(FaultLogInfoInner* info);
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace HiviewDFX {
/**
 * @brief Distinguish different types of fault
 */
enum FaultLogType {
    /** unspecific fault types */
    NO_SPECIFIC = 0,
    /** C/C++ crash at runtime*/
    CPP_CRASH = 2,
    /** js crash at runtime*/
    JS_CRASH,
    /** application happen freezing */
    APP_FREEZE,
    /** system happen freezing */
    SYS_FREEZE,
    /** system happen warning */
    SYS_WARNING,
    /** rust crash at runtime */
    RUST_PANIC,
    /** cj error at runtime */
    CJ_ERROR = 9,
};

/**
 * @brief Check faultlogger service status
 *
 * This API is used to check the faultlogger service status
 *
 * @return When faultlogger service is available,it returns true. When not,it rerurns false.
*/
bool CheckFaultloggerStatus();

/**
 * @brief Add fault log
 *
 * This API is used to save fault information to file
 * under the /data/log/fautlog/faultlogger directory and event database
 *
 * @param info  structure containing information about fault
*/

void AddFaultLog(const FaultLogInfoInner &info);

/**
 * @brief Add fault log
 *
 * This API is used to save fault information to file
 * under the /data/log/fautlog/faultlogger directory and event database
 *
 * @param time  the time of happening fault(unix timestamp of Milliseconds)
 * @param logType  the type of fault log.
 * eg: CPP_CRASH,JS_CRASH,APP_FREEZE,SYS_FREEZE,SYS_WARNING,RUST_PANIC
 * @param module name of module which happened fault
 * @param summary the summary of fault information
*/
void AddFaultLog(int64_t time, int32_t logType, const std::string &module, const std::string &summary);

/**
 * @brief query self fault log from event database
 *
 * This API is used to query fault log
 * which belong to current pid and uid from event database
 *
 * @param faultType type of fault log.
 * eg: NO_SPECIFIC,CPP_CRASH,JS_CRASH,APP_FREEZE
 * @param maxNum max number of faultlog entries
 * @return when success query return unique_ptr of FaultLogQueryResult, otherwise return nullptr
*/
std::unique_ptr<FaultLogQueryResult> QuerySelfFaultLog(FaultLogType faultType, int32_t maxNum);

/**
 * @brief enable the GWP-ASAN grayscale of your application.
 *
 * @param alwaysEnabled - Control whether to enable GWP-ASan every time.
 * @param sampleRate - sample rate of GWP-ASAN.
 * @param maxSimutaneousAllocations - the max simutaneous allocations of GWP-ASAN.
 * @param duration - The duration days of GWP-ASAN grayscale.
 * @return true - enable success, false - enable falied
*/
bool EnableGwpAsanGrayscale(bool alwaysEnabled, double sampleRate,
    double maxSimutaneousAllocations, int32_t duration);

/**
 * @brief disable the GWP-ASAN grayscale of your application.
 *
*/
void DisableGwpAsanGrayscale();

/**
 * @brief obtain the remaining days of GWP-ASan grayscale for your application.
 *
 * @returns the remaining days of GWP-ASan grayscale.
*/
uint32_t GetGwpAsanGrayscaleState();
}  // namespace HiviewDFX
}  // namespace OHOS
#endif  // HIVIEW_FAULTLOGGER_CLIENT_INTERFACE_H
