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
#ifndef PAGE_TRACE_NODE_H
#define PAGE_TRACE_NODE_H

#include <cstdint>
#include <string>

#include "faultlog_util.h"

namespace OHOS {
namespace HiviewDFX {
class PageTraceNode {
public:
    PageTraceNode() = default;
    PageTraceNode(int32_t pid, uint64_t ts, const std::string& url, const std::string& name)
        : pid_(pid), timestamp_(ts)
    {
        SetPageInfo(url, name);
    }

    void SetPageInfo(const std::string& url, const std::string& name)
    {
        pagePath_ = url + (name.empty() ? "" : ":" + name);
    }

    std::string ToString() const
    {
        return GetFormatedTimeHHMMSS(timestamp_, true) + " " + pagePath_;
    }

    int32_t pid_{0};
    uint64_t timestamp_{0};
    std::string pagePath_;
};
} // HiviewDFX
} // OHOS
#endif // PAGE_TRACE_NODE_H
