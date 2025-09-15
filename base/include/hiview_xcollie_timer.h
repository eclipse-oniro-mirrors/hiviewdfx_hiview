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
#ifndef HIVIEW_BASE_XCOLLIE_TIMER_H
#define HIVIEW_BASE_XCOLLIE_TIMER_H

#include <string>

namespace OHOS {
namespace HiviewDFX {
constexpr unsigned int APP_CALLING_TIMEOUT = 10; // 10s
constexpr unsigned int SYS_CALLING_TIMEOUT = 60; // 60s

class HiviewXCollieTimer {
public:
    HiviewXCollieTimer(const std::string& name, unsigned int timeout);
    ~HiviewXCollieTimer();
    HiviewXCollieTimer(const HiviewXCollieTimer&) = delete;
    HiviewXCollieTimer& operator=(const HiviewXCollieTimer&) = delete;

private:
    int id_ = 0;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_XCOLLIE_TIMER_H
