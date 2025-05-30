/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef XPERF_EVENT_BUILDER_H
#define XPERF_EVENT_BUILDER_H

#include <string>
#include <vector>
#include "xperf_event.h"

namespace OHOS {
namespace HiviewDFX {

class XperfEventBuilder {
public:
    explicit XperfEventBuilder();
    ~XperfEventBuilder();

    XperfEventBuilder& EventName(const std::string& eventName);
    XperfEventBuilder& EventType(const HiSysEventEventType& eventType);
    XperfEventBuilder& Param(const char* name, bool value);
    XperfEventBuilder& Param(const char* name, int8_t value);
    XperfEventBuilder& Param(const char* name, uint8_t value);
    XperfEventBuilder& Param(const char* name, int16_t value);
    XperfEventBuilder& Param(const char* name, uint16_t value);
    XperfEventBuilder& Param(const char* name, int32_t value);
    XperfEventBuilder& Param(const char* name, uint32_t value);
    XperfEventBuilder& Param(const char* name, int64_t value);
    XperfEventBuilder& Param(const char* name, uint64_t value);
    XperfEventBuilder& Param(const char* name, float value);
    XperfEventBuilder& Param(const char* name, double value);
    XperfEventBuilder& Param(const char* name, const std::string value);
    XperfEventBuilder& Param(const char* name, char* value);
    XperfEventBuilder& Param(const char* name, const std::vector<uint16_t>& value);
    OHOS::HiviewDFX::XperfEvent Build();

private:
    std::string evtName;
    HiSysEventEventType evtType{HISYSEVENT_BEHAVIOR};
    std::vector<HiSysEventParam> paramList;

    bool Check();
    void SetParamName(HiSysEventParam& param, const char* name);
    void Release();
};
}
}
#endif //XPERF_EVENT_BUILDER_H