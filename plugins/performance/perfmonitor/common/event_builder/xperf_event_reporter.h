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
#ifndef XPERF_EVENT_REPORTER_H
#define XPERF_EVENT_REPORTER_H

#include <string>
#include "xperf_event.h"

namespace OHOS {
namespace HiviewDFX {

constexpr char ACE_DOMAIN[] = "ACE";

class XperfEventReporter {
public:
    explicit XperfEventReporter();
    ~XperfEventReporter();

    void Report(const char* domain, const OHOS::HiviewDFX::XperfEvent& event);
};
}
}
#endif //XPERF_EVENT_REPORTER_H