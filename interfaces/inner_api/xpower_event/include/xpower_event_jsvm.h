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
#ifndef XPOWER_EVENT_JSVM_H
#define XPOWER_EVENT_JSVM_H

#include <string>
#include "ecmascript/napi/include/dfx_jsnapi.h"

namespace OHOS {
namespace HiviewDFX {
using panda::ecmascript::EcmaVM;
/**
 * Used to report js stack info to XPower
 *
 * @param tagName tag name used to track the stack
 * @param info additional informations
 * @return 0 when success. see @ERR_SUCCESS
 * @return 1 when propertity @PROP_XPOWER_OPTIMIZE_ENABLE is not enabled. see @ERR_PROP_NOT_ENABLE
 * @return 2 when dump stack failed. see @ERR_DUMP_STACK_FAILED
 * @return other error codes. see @HiSysEventWrite
 */
int ReportXPowerJsStackSysEvent(EcmaVM *vm, const std::string &tagName, const std::string &info = "");
} // HiviewDFX
} // OHOS

#endif