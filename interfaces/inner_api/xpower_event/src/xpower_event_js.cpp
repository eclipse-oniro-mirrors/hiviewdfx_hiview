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

#include "xpower_event_js.h"
#include "xpower_event_jsvm.h"
#include "xpower_event_common.h"
#include <parameters.h>
#include "hisysevent.h"
#include "native_engine/native_engine.h"

namespace OHOS {
namespace HiviewDFX {

int ReportXPowerJsStackSysEvent(napi_env env, const std::string &tagName, const std::string &info)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    return ReportXPowerJsStackSysEvent(engine, tagName, info);
}

int ReportXPowerJsStackSysEvent(NativeEngine *engine, const std::string &tagName, const std::string &info)
{
    if (engine == nullptr) {
        return ERR_PARAM_INVALID;
    }
    if ((OHOS::system::GetIntParameter(PROP_XPOWER_OPTIMIZE_ENABLE, 0) != 1)) {
        return ERR_PROP_NOT_ENABLE;
    }
    std::string stack = "";
    bool succ = engine->BuildJsStackTrace(stack);
    if (!succ) {
        return ERR_DUMP_STACK_FAILED;
    }
    int ret = HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, TAG_XPOWER_STACKTRACE,
        HiSysEvent::EventType::STATISTIC, "TAGNAME", tagName, "INFO", info, "STACKTRACE", stack);
    return ret; // 0 success or error code in HiSysEventWrite
}

int ReportXPowerJsStackSysEvent(EcmaVM *vm, const std::string &tagName, const std::string &info)
{
    if (vm == nullptr) {
        return ERR_PARAM_INVALID;
    }
    if ((OHOS::system::GetIntParameter(PROP_XPOWER_OPTIMIZE_ENABLE, 0) != 1)) {
        return ERR_PROP_NOT_ENABLE;
    }
    std::string stack = "";
    bool succ = panda::DFXJSNApi::BuildJsStackTrace(vm, stack);
    if (!succ) {
        return ERR_DUMP_STACK_FAILED;
    }
    int ret = HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, TAG_XPOWER_STACKTRACE,
        HiSysEvent::EventType::STATISTIC, "TAGNAME", tagName, "INFO", info, "STACKTRACE", stack);
    return ret; // 0 success or error code in HiSysEventWrite
}

}
}