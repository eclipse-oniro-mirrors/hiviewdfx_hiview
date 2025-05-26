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

#ifndef HIVIEW_PLUGINS_EVENT_VALIDATOR_H
#define HIVIEW_PLUGINS_EVENT_VALIDATOR_H

#include "event_delayed_util.h"
#include "event_verify_util.h"
#include "i_controller.h"
#include "plugin.h"

namespace OHOS {
namespace HiviewDFX {
class EventValidator : public Plugin {
public:
    bool OnEvent(std::shared_ptr<Event>& event) override;
    void OnLoad() override;
    void OnUnload() override;
    void OnConfigUpdate(const std::string& localCfgPath, const std::string& cloudCfgPath) override;

private:
    void Init();
    void InitWorkLoop(HiviewContext* context);
    void InitEventVerifyUtil(HiviewContext* context);
    void InitController(HiviewContext* context);
    bool CheckEvent(std::shared_ptr<Event> event);
    void UpdateConfig();

private:
    EventDelayedUtil eventDelayedUtil_;
    EventVerifyUtil eventVerifyUtil_;
    std::shared_ptr<IController> controller_;
    std::atomic<bool> isConfigUpdated_ { false };
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_EVENT_VALIDATOR_H
