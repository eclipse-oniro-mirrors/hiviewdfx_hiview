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

#include "page_history_recorder.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
std::string PageHistoryRecorder::GetPageTrace(const std::string& boundleName, int32_t pid) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = lruCache_.find(boundleName); it != lruCache_.end()) {
        return it->second->second.ToString(pid);
    }
    HIVIEW_LOGE("save process num is %{public}zu", lruCache_.size());
    return "";
}

void PageHistoryRecorder::PutPageTrace(const std::string& boundleName, PageTraceNode node)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = lruCache_.find(boundleName); it != lruCache_.end()) {
        pagesList_.splice(pagesList_.begin(), pagesList_, it->second);
        it->second->second.AddPageTrace(std::move(node));
        return;
    }

    PagesTrace trace;
    trace.AddPageTrace(std::move(node));
    pagesList_.emplace_front(boundleName, trace);
    lruCache_[boundleName] = pagesList_.begin();

    while (lruCache_.size() > MAX_RECORDED_PROCESS_NUM) {
        lruCache_.erase(pagesList_.back().first);
        pagesList_.pop_back();
    }
}
} // HiviewDFX
} // OHOS
