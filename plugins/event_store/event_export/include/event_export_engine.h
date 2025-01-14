/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_EVENT_EXPORT_ENGINE_H
#define HIVIEW_BASE_EVENT_EXPORT_ENGINE_H

#include <memory>
#include <mutex>
#include <vector>
#include <functional>

#include "export_base_task.h"
#include "export_config_manager.h"
#include "export_db_manager.h"

namespace OHOS {
namespace HiviewDFX {
class EventExportEngine {
public:
    static EventExportEngine& GetInstance();

public:
    void Start();
    void Stop();

private:
    EventExportEngine();
    ~EventExportEngine();

private:
    void Init();
    void InitAndRunTasks();
    void HandleExportSwitchOn(const std::string& moduleName);
    void HandleExportSwitchOff(const std::string& moduleName);
    bool RegistSettingObserver(std::shared_ptr<ExportConfig> config);
    void InitAndRunTask(std::shared_ptr<ExportConfig> config);

private:
    bool isTaskRunning_ = false;
    std::shared_ptr<ExportDbManager> dbMgr_ = nullptr;
    std::mutex mgrMutex_;
    std::vector<std::shared_ptr<ExportConfig>> configs_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_ENGINE_H