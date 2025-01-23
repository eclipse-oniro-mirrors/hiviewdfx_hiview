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

#ifndef APP_EVENT_ELAPSED_TIME_H
#define APP_EVENT_ELAPSED_TIME_H

#include <string>

namespace OHOS {
namespace HiviewDFX {
class ElapsedTime {
public:
    ElapsedTime();
    ElapsedTime(uint64_t limitCostMilliseconds, std::string printContent);
    ~ElapsedTime();
    ElapsedTime(const ElapsedTime&) = delete;
    ElapsedTime& operator=(const ElapsedTime&) = delete;
    void MarkElapsedTime(const std::string& markContent);

private:
    uint64_t begin_ = 0;
    uint64_t lastMarkTime_ = 0;
    uint64_t limitCostMilliseconds_ = 0;
    std::string printContent_ = "";
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // APP_EVENT_ELAPSED_TIME_H