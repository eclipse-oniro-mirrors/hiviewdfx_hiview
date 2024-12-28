/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_PLUGINS_CRASH_VALIDATOR_H
#define HIVIEW_PLUGINS_CRASH_VALIDATOR_H

#include <memory>
#include <mutex>
#include <string>

#include "event.h"
#include "plugin.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
class CrashValidator : public Plugin {
public:
    CrashValidator();
    ~CrashValidator();
    void OnLoad() override;
    void OnUnload() override;
    bool OnEvent(std::shared_ptr<Event>& event) override;
    bool IsInterestedPipelineEvent(std::shared_ptr<Event> event) override;
private:
    void InitWorkLoop();
    bool MatchEvent(int32_t pid);
    bool MatchKernelSnapshotEvent(int32_t pid);
    void RemoveMatchEvent(int32_t pid);
    void AddEventToMap(int32_t pid, std::shared_ptr<SysEvent> sysEvent);
    void ReportMatchEvent(std::string eventName, std::shared_ptr<SysEvent> sysEvent);
    void ReportDisMatchEvent(std::shared_ptr<SysEvent> sysEvent);

    std::shared_ptr<SysEvent> Convert2SysEvent(std::shared_ptr<Event>& event);
    std::atomic<bool> hasLoaded_;
    std::mutex mutex_;
    std::map<int32_t, std::shared_ptr<SysEvent>> processExitEvents_;
    std::map<int32_t, std::shared_ptr<SysEvent>> cppCrashEvents_;
    std::map<int32_t, std::shared_ptr<SysEvent>> cppCrashExceptionEvents_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif