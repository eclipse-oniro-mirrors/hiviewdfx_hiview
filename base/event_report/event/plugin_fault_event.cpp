/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "plugin_fault_event.h"

#include "hiview_event_common.h"

namespace OHOS {
namespace HiviewDFX {
using namespace PluginFaultEventSpace;

PluginFaultEvent::PluginFaultEvent(const std::string &name, HiSysEvent::EventType type)
    : LoggerEvent(name, type)
{
    this->paramMap_ = {
        {std::string(KEY_OF_PLUGIN_NAME), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_REASON), std::string(DEFAULT_STRING)} };
}

void PluginFaultEvent::Report()
{
    HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, this->eventName_, this->eventType_,
        KEY_OF_PLUGIN_NAME, this->paramMap_[KEY_OF_PLUGIN_NAME].GetString(),
        KEY_OF_REASON, this->paramMap_[KEY_OF_REASON].GetString());
}
} // namespace HiviewDFX
} // namespace OHOS
