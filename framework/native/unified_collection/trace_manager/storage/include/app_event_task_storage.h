/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_APP_EVENT_TASK_STORAGE_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_APP_EVENT_TASK_STORAGE_H

#include <cstdint>
#include <memory>
#include <string>

#include "restorable_db_store.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int8_t APP_EVENT_TASK_TYPE_JANK_EVENT = 1;
constexpr int8_t APP_EVENT_TASK_STATE_FINISH = 2;

struct AppEventTask {
    int64_t id_ = 0;
    int64_t taskDate_ = 0;
    int32_t taskType_ = 0;
    int32_t uid_ = 0;
    int32_t pid_ = 0;
    std::string bundleName_;
    std::string bundleVersion_;
    int64_t startTime_ = 0;
    int64_t finishTime_ = 0;
    std::string resourcePath_;
    int32_t resourceSize_ = 0;
    double costCpu_ = 0.0;
    int32_t state_ = 0;
};

class AppEventTaskStorage {
public:
    AppEventTaskStorage(std::shared_ptr<RestorableDbStore> dbStore);
    ~AppEventTaskStorage() = default;

public:
    bool GetAppEventTask(int32_t uid, int32_t eventDate, AppEventTask &appEventTask);
    bool InsertAppEventTask(AppEventTask &appEventTask);
    void RemoveAppEventTask(int32_t eventDate);

private:
    std::shared_ptr<RestorableDbStore> dbStore_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_APP_EVENT_TASK_STORAGE_H
