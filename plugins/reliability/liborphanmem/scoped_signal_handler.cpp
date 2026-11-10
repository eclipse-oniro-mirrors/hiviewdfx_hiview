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

#include "scoped_signal_handler.h"

#include "log.h"

namespace OHOS {

Allocator<SignalFnMap>::UniquePtr ScopedSignalHandler::handlerMap_;

ScopedSignalHandler::ScopedSignalHandler(Allocator<ScopedSignalHandler> allocator) : signal_(-1)
{
    if (handlerMap_ == nullptr) {
        Allocator<SignalFnMap> mapAllocator(allocator);
        handlerMap_ = mapAllocator.MakeUnique(Allocator<SignalFnMap::value_type>(allocator));
    }
}

void ScopedSignalHandler::Install(int signal, const Fn &f)
{
    if (signal_ != -1) {
        return;
    }

    if (handlerMap_ == nullptr) {
        return;
    }

    (*handlerMap_)[signal] = SignalFn(f);

    struct signal_chain_action sigchain = {
        .sca_sigaction = [](int signal, siginfo_t *si, void *uctx) -> bool {
            auto it = handlerMap_->find(signal);
            if (it != handlerMap_->end()) {
                return it->second(signal, si, uctx);
            }
            return true;
        },
        .sca_mask = {},
        .sca_flags = 0,
    };
    add_special_signal_handler(signal, &sigchain);

    signal_ = signal;
}

}  // namespace OHOS
