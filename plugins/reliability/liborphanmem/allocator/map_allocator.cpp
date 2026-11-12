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

#include <cstdlib>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <sys/cdefs.h>
#include <sys/mman.h>
#include <sys/prctl.h>

#include "map_allocator.h"
#include "chunk.h"
#include "dfx_signal_handler.h"
#include "heap_impl.h"
#include "linked_list.h"
#include "log.h"

namespace OHOS {

Heap::Heap()
{
    impl_ = new HeapImpl();
    ownsImpl_ = true;
}

Heap::~Heap()
{
    if (ownsImpl_) {
        delete impl_;
    }
}

void *Heap::Allocate(size_t size) const
{
    return impl_->Alloc(size);
}

void Heap::Deallocate(HeapImpl *impl, void *ptr)
{
    impl->Free(ptr);
}

void Heap::Deallocate(void *ptr) const
{
    impl_->Free(ptr);
}

bool Heap::Empty()
{
    return impl_->Empty();
}
}  // namespace OHOS
