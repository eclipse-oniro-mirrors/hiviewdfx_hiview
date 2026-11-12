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

#ifndef LIBORPHANMEM_PERFORMANCE_TRACER_H
#define LIBORPHANMEM_PERFORMANCE_TRACER_H

#include <chrono>
#include <cstdarg>

#include "log.h"

namespace OHOS {

class PerformanceTracer {
public:
    explicit PerformanceTracer(const char *name = nullptr);

    void Mark(const char *fmt, ...);

private:
    const char *name_;
    std::chrono::high_resolution_clock::time_point lastTime_;
    bool firstCall_;
};

}  // namespace OHOS

#endif  // LIBORPHANMEM_PERFORMANCE_TRACER_H