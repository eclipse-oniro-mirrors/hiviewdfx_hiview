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
#include "user_action_storage.h"
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

UserActionStorage& UserActionStorage::GetInstance()
{
    static UserActionStorage instance;
    return instance;
}

void UserActionStorage::UpdateFirstMove(const PerfActionEvent& fm)
{
    firstMove = fm;
    LOGD("UpdateFirstMove bundle:%{public}s", firstMove.bundleName.c_str());
}

void UserActionStorage::UpdateLastUp(const PerfActionEvent& lu)
{
    lastUp = lu;
    LOGD("UpdateLastUp bundle:%{public}s", lastUp.bundleName.c_str());
}

const PerfActionEvent& UserActionStorage::GetFirstMove()
{
    return firstMove;
}

const PerfActionEvent& UserActionStorage::GetLastUp()
{
    return lastUp;
}
}
}
