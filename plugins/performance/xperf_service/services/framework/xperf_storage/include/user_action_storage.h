/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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
#ifndef USER_ACTION_STORAGE_H
#define USER_ACTION_STORAGE_H

#include <cstdint>
#include <string>
#include "perf_action_event.h"

namespace OHOS {
namespace HiviewDFX {

class __attribute__((visibility("default"))) UserActionStorage {
public:
    static UserActionStorage& GetInstance();
    UserActionStorage(const UserActionStorage &) = delete;
    void operator=(const UserActionStorage &) = delete;

    void UpdateFirstMove(const PerfActionEvent& fm);
    void UpdateLastUp(const PerfActionEvent& lu);

    const PerfActionEvent& GetFirstMove();
    const PerfActionEvent& GetLastUp();

private:
    UserActionStorage() = default;

    PerfActionEvent firstMove;
    PerfActionEvent lastUp;
};

}
}
#endif
