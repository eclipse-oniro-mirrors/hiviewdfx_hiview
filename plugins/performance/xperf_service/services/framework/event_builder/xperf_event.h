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

#ifndef XPERF_EVENT_H
#define XPERF_EVENT_H

#include <string>
#include "hisysevent_c.h"

namespace OHOS {
namespace HiviewDFX {
struct XperfEvent {
    std::string evtName;
    HiSysEventEventType evtType{HISYSEVENT_BEHAVIOR};
    HiSysEventParam* params{nullptr};
    int paramSize{0};

    XperfEvent()
    {
    }

    XperfEvent(const std::string& name, const HiSysEventEventType& type, const int& size)
    {
        evtName = name;
        evtType = type;
        paramSize = size;
        params = new HiSysEventParam[paramSize];
    }

    ~XperfEvent()
    {
        for (int i = 0; i < paramSize; i++) {
            if ((params[i].t == HISYSEVENT_STRING) && (params[i].v.s != nullptr)) {
                delete[] params[i].v.s;
                params[i].v.s = nullptr;
            }
        }
        if (params != nullptr) {
            delete[] params;
            params = nullptr;
        }
    }
};
} // HiviewDFX
} // OHOS
#endif //XPERF_EVENT_H