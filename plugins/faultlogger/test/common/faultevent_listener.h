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

#ifndef FAULTEVENT_LISTENER_H
#define FAULTEVENT_LISTENER_H

#include "hisysevent_listener.h"
#include <vector>

namespace OHOS {
namespace HiviewDFX {
class FaultEventListener : public HiSysEventListener {
public:
    explicit FaultEventListener() : HiSysEventListener() {}
    virtual ~FaultEventListener() {}

public:
    void OnEvent(std::shared_ptr<HiSysEventRecord> sysEvent);
    void SetKeyWords(const std::vector<std::string>& keyWords);
    bool CheckKeyWords();
    void OnServiceDied() {};
private:
    std::vector<std::string> keyWords;
    bool allFindFlag = false;
    std::mutex setFlagMutex;
    std::condition_variable keyWordCheckCondition;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
