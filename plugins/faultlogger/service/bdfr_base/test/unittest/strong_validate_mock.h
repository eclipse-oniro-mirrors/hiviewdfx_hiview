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
#ifndef STRONG_VALIDATE_MOCK_H
#define STRONG_VALIDATE_MOCK_H

#include "hisysevent_c.h"
#include <string>
#include <vector>

struct CapturedEvent {
    std::string domain;
    std::string name;
    HiSysEventEventType type;
    std::vector<HiSysEventParam> params;
};

extern CapturedEvent g_capturedEvent;

#undef OH_HiSysEvent_Write
#define OH_HiSysEvent_Write(domain, name, type, params, size) \
    CaptureHiSysEvent(domain, name, type, params, size)

int CaptureHiSysEvent(const char* domain, const char* name,
    HiSysEventEventType type, const HiSysEventParam* params, size_t size);

#endif
