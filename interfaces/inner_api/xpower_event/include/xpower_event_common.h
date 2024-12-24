/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef DFX_DUMPCATCH_TRACE_COMMON_H
#define DFX_DUMPCATCH_TRACE_COMMON_H

namespace OHOS {
namespace HiviewDFX {

// when success, return 0. Other error codes see @HiSysEventWrite
static constexpr int ERR_SUCCESS = 0; // 0 means success, same with @OHOS::HiviewDFX::SUCCESS in hisysevent.h
static constexpr int ERR_PROP_NOT_ENABLE = 1; // propertity @PROP_XPOWER_OPTIMIZE_ENABLE not enabled
static constexpr int ERR_DUMP_STACK_FAILED = 2;
static constexpr int ERR_PARAM_INVALID = 3;

// tag name for XPower stack trace
#define TAG_XPOWER_STACKTRACE "XPOWER_STACKTRACE"

// the propertity is an integer and used to control report stack info to XPower
// if set to 1, then it will report c++ stack and js stack info
// see @ReportXPowerStackSysEvent and @ReportXPowerJsStackSysEvent
#define PROP_XPOWER_OPTIMIZE_ENABLE "persist.hiviewdfx.optimize.enable"
} // HiviewDFX
} // OHOS

#endif