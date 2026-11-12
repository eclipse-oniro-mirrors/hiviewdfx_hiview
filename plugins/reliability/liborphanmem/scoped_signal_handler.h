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

#ifndef LIBORPHANMEM_SCOPED_SIGNAL_HANDLER_H
#define LIBORPHANMEM_SCOPED_SIGNAL_HANDLER_H

#include <csignal>
#include <functional>
#include <sigchain.h>

#include "map_allocator.h"

namespace OHOS {
using SignalFn = std::function<bool(int, siginfo_t *, void *)>;
using SignalFnMap = MAP_ALLOCATOR::unordered_map<int, SignalFn>;

class ScopedSignalHandler {
public:
    using Fn = std::function<bool(int, siginfo_t *, void *)>;

    explicit ScopedSignalHandler(Allocator<ScopedSignalHandler> allocator);
    ~ScopedSignalHandler() = default;

    void Install(int signal, const Fn &f);

private:
    ScopedSignalHandler(const ScopedSignalHandler &) = delete;
    ScopedSignalHandler &operator=(const ScopedSignalHandler &) = delete;
    int signal_;
    static Allocator<SignalFnMap>::UniquePtr handlerMap_;
};

}  // namespace OHOS

#endif  // LIBORPHANMEM_SCOPED_SIGNAL_HANDLER_H
