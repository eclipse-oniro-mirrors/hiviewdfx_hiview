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
#include "strong_validate_mock.h"

CapturedEvent g_capturedEvent;

int CaptureHiSysEvent(const char* domain, const char* name,
    HiSysEventEventType type, const HiSysEventParam* params, size_t size)
{
    g_capturedEvent.domain = domain;
    g_capturedEvent.name = name;
    g_capturedEvent.type = type;
    g_capturedEvent.params.clear();
    for (size_t i = 0; i < size; ++i) {
        g_capturedEvent.params.push_back(params[i]);
    }
    return 0;
}
