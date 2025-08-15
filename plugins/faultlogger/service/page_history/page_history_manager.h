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
#ifndef PAGE_HISTORY_MANAGER_H
#define PAGE_HISTORY_MANAGER_H

#include "event.h"
#include "page_history_recorder.h"
#include "pages_trace.h"

namespace OHOS {
namespace HiviewDFX {
class PageHistoryManager {
public:
    static PageHistoryManager& GetInstance();
    PageHistoryManager(const PageHistoryManager&) = delete;
    PageHistoryManager &operator=(const PageHistoryManager&) = delete;

    void HandleEvent(const Event& event);
    std::string GetPageHistory(const std::string& processName, int64_t pid);

private:
    PageHistoryManager() = default;
    PageHistoryRecorder recorder_;
};
} // HiviewDFX
} // OHOS

#endif // PAGE_HISTORY_MANAGER_H
