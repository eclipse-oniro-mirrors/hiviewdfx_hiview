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

#include "pages_trace.h"
#include <cstdint>

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
void PagesTrace::AddPageTrace(PageTraceNode node)
{
    pages_.emplace_back(std::move(node));
    while (pages_.size() > MAX_PAGES_NUM) {
        pages_.pop_front();
    }
}

std::string PagesTrace::ToString(int32_t pid) const
{
    std::string result;
    for (auto it = pages_.rbegin(); it != pages_.rend(); ++it) {
        if (it->pid_ == pid) {
            result += "  " + it->ToString() + "\n";
        }
    }
    if (result.empty()) {
        HIVIEW_LOGE("pages is %{public}zu", pages_.size());
    }
    return result;
}
} // HiviewDFX
} // OHOS
