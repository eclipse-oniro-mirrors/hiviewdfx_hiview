/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "faultevent_listener.h"

#include <cstring>
#include <iostream>

namespace OHOS {
namespace HiviewDFX {

void FaultEventListener::SetKeyWords(const std::vector<std::string>& keyWords)
{
    this->keyWords = keyWords;
    {
        std::lock_guard<std::mutex> lock(setFlagMutex);
        allFindFlag = false;
    }
}

void FaultEventListener::OnEvent(std::shared_ptr<HiSysEventRecord> sysEvent)
{
    if (sysEvent == nullptr) {
        return;
    }
    auto str = sysEvent->AsJson();
    for (const auto& keyWord : keyWords) {
        if (str.find(keyWord) == std::string::npos) {
            return;
        }
    }

    // find all keywords, set allFindFlag to true
    {
        std::lock_guard<std::mutex> lock(setFlagMutex);
        allFindFlag = true;
        keyWordCheckCondition.notify_all();
    }
}

bool FaultEventListener::CheckKeyWords()
{
    std::unique_lock<std::mutex> lock(setFlagMutex);
    if (allFindFlag) {
        return true;
    }

    auto flagCheckFunc = [&]() {
        return allFindFlag;
    };

    // 8: wait allFindFlag set true for 8 seconds
    if (keyWordCheckCondition.wait_for(lock, std::chrono::seconds(8), flagCheckFunc)) {
        return true;
    } else {
        std::cout << "match keywords timeout"  << std::endl;
        return false;
    }
}
} // namespace HiviewDFX
} // namespace OHOS
