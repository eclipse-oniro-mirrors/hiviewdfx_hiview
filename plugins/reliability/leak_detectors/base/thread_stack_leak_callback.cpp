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

#include "thread_stack_leak_callback.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "ThreadStackLeakCallback");

void ThreadStackLeakCallback::OnMemDumpDone(const std::string &dumpResult)
{
    std::lock_guard<std::mutex> lock(mtx_);
    dumpResult_ = dumpResult;
    completed_ = true;
    cv_.notify_one();
}

bool ThreadStackLeakCallback::WaitForResult(std::string &dumpResult, int32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(mtx_);
    if (!cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return completed_; })) {
        return false;
    }
    dumpResult = dumpResult_;
    return true;
}

} // namespace HiviewDFX
} // namespace OHOS
