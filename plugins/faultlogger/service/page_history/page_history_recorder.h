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
#ifndef PAGE_HISTORY_RECORDER_H
#define PAGE_HISTORY_RECORDER_H

#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

#include "event.h"
#include "pages_trace.h"

namespace OHOS {
namespace HiviewDFX {

class PageHistoryRecorder {
public:
    std::string GetPageTrace(const std::string& boundleName, int32_t pid) const;
    void PutPageTrace(const std::string& boundleName, PageTraceNode node);

private:
    static constexpr std::size_t MAX_RECORDED_PROCESS_NUM = 20;
    std::list<std::pair<std::string, PagesTrace>> pagesList_;
    std::unordered_map<std::string, typename std::list<std::pair<std::string, PagesTrace>>::iterator> lruCache_;
    mutable std::mutex mutex_;
};

} // HiviewDFX
} // OHOS
#endif // PAGE_HISTORY_RECORDER_H
