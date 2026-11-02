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

#ifndef DFR_THREAD_STACK_LEAK_CALLBACK_H
#define DFR_THREAD_STACK_LEAK_CALLBACK_H

#include <condition_variable>
#include <mutex>
#include "mem_dump_callback_stub.h"

namespace OHOS {
namespace HiviewDFX {

// Callback stub for receiving thread stack leak detection results from the
// target app process through the ability manager service over IPC. Uses a
// condition variable so the caller can block until the result arrives or a
// timeout expires.
class ThreadStackLeakCallback : public AppExecFwk::MemDumpCallbackStub {
public:
    ThreadStackLeakCallback() = default;
    ~ThreadStackLeakCallback() override = default;
    void OnMemDumpDone(const std::string &dumpResult) override;
    bool WaitForResult(std::string &dumpResult, int32_t timeoutMs);

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::string dumpResult_;
    bool completed_ = false;
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // DFR_THREAD_STACK_LEAK_CALLBACK_H
