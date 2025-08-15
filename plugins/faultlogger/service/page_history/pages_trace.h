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
#ifndef PAGE_TRACE_H
#define PAGE_TRACE_H

#include <deque>

#include "page_trace_node.h"

namespace OHOS {
namespace HiviewDFX {
class PagesTrace {
public:
    void AddPageTrace(PageTraceNode node);
    std::string ToString(int32_t pid) const;

private:
    static constexpr std::size_t MAX_PAGES_NUM = 10;
    std::deque<PageTraceNode> pages_;
};
} // HiviewDFX
} // OHOS
#endif // PAGE_TRACE_H
