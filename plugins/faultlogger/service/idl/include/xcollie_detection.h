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
#ifndef XCOLLOE_DETECTION_H
#define XCOLLOE_DETECTION_H
#include <string.h>

#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"

namespace OHOS {
namespace HiviewDFX {
constexpr const uint32_t XCOLLIE_MAIN_THREAD_TIMEOUT_SECOND = 10;
constexpr const uint32_t XCOLLIE_OTHER_THREAD_TIMEOUT_SECOND = 60;

class XCollieDetector {
public:
    XCollieDetector(const std::string& name, unsigned int timeout = XCOLLIE_MAIN_THREAD_TIMEOUT_SECOND)
    {
        timerId_ = XCollie::GetInstance()
            .SetTimer(name, timeout, nullptr, nullptr, XCOLLIE_FLAG_LOG | XCOLLIE_FLAG_RECOVERY);
    }
    XCollieDetector(const XCollieDetector&) = delete;
    XCollieDetector &operator=(const XCollieDetector&) = delete;

    ~XCollieDetector()
    {
        if (timerId_ >= 0) {
            XCollie::GetInstance().CancelTimer(timerId_);
        }
    }

private:
    int timerId_{-1};
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // XCOLLOE_DETECTION_H
