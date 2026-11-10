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

#include "performance_tracer.h"

#include <securec.h>

namespace OHOS {

PerformanceTracer::PerformanceTracer(const char *name) : name_(name), firstCall_(true) {}

void PerformanceTracer::Mark(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf_s(buf, sizeof(buf), sizeof(buf), fmt, args);
    if (ret < 0) {
        va_end(args);
        return;
    }
    va_end(args);

    auto now = std::chrono::high_resolution_clock::now();
    if (firstCall_) {
        if (name_ != nullptr) {
            HILOGI("[Tracer:%{public}s] %{public}s", name_, buf);
        } else {
            HILOGI("%{public}s", buf);
        }
        firstCall_ = false;
    } else {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime_);
        if (name_ != nullptr) {
            HILOGI("[Tracer:%{public}s] %{public}s cost: %{public}lld ms", name_, buf,
                   static_cast<long long>(duration.count()));
        } else {
            HILOGI("%{public}s cost: %{public}lld ms", buf, static_cast<long long>(duration.count()));
        }
    }
    lastTime_ = now;
}

}  // namespace OHOS
