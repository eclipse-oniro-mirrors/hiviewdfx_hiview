/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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
#include "plugin_event.h"

#include "hisysevent_util.h"
#include "hiview_event_common.h"

namespace OHOS {
namespace HiviewDFX {
using namespace PluginEventSpace;

PluginEvent::PluginEvent(const std::string &name, HiSysEvent::EventType type)
    : LoggerEvent(name, type)
{
    this->paramMap_ = {
        {std::string(KEY_OF_PLUGIN_NAME), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_RESULT), DEFAULT_UINT32},
        {std::string(KEY_OF_DURATION), DEFAULT_UINT32}
    };
}

void PluginEvent::Report()
{
    std::string pluginName = paramMap_[KEY_OF_PLUGIN_NAME].GetString();
    HiSysEventParam params[] = {
        BUILD_PARAM(PLUGIN_NAME_LITERAL, HISYSEVENT_STRING, s, PARAM_STR(pluginName)),
        BUILD_PARAM(RESULT_LITERAL, HISYSEVENT_UINT32, ui32, this->paramMap_[KEY_OF_RESULT].GetUint32()),
        BUILD_PARAM(DURATION_LITERAL, HISYSEVENT_UINT32, ui32, this->paramMap_[KEY_OF_DURATION].GetUint32()),
    };
    (void)OH_HiSysEvent_Write(HiSysEvent::Domain::HIVIEWDFX, this->eventName_.c_str(),
        TranslateEventType(this->eventType_),
        params, sizeof(params) / sizeof(HiSysEventParam));
}
} // namespace HiviewDFX
} // namespace OHOS
