/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_WORKER_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_WORKER_H

#include <functional>
#include <string>

#include "ffrt.h"

using UcollectionTask = std::function<void ()>;
using UcollectionWorker = std::function<void (UcollectionTask)>;

namespace OHOS {
namespace HiviewDFX {
class TraceWorker {
public:
    static TraceWorker& GetInstance();

public:
    void HandleUcollectionTask(UcollectionTask ucollectionTask);

private:
    TraceWorker() {};
    ~TraceWorker() {};
    static TraceWorker traceWorker_;
    std::unique_ptr<ffrt::queue> ffrtQueue_ = std::make_unique<ffrt::queue>("dft_trace_worker");
};
} // HiViewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_WORKER_H
