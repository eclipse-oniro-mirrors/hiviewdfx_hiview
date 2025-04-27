/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#ifndef HIVIEWDFX_HIVIEW_FAULTLOGGER_H
#define HIVIEWDFX_HIVIEW_FAULTLOGGER_H

#include <string>
#include <unordered_map>

#include "plugin.h"

#include "faultlog_info.h"
#include "faultlog_manager.h"
#include "faultlog_bootscan.h"

#include "faultlog_manager_service.h"

namespace OHOS {
namespace HiviewDFX {

class Faultlogger : public Plugin {
public:
    ~Faultlogger() override = default;
    bool OnEvent(std::shared_ptr<Event>& event) override;
    bool IsInterestedPipelineEvent(std::shared_ptr<Event> event) override;
    bool CanProcessEvent(std::shared_ptr<Event> event) override;
    bool ReadyToLoad() override;
    void OnLoad() override;

    void Dump(int fd, const std::vector<std::string>& cmds) override;
private:
    bool hasInit_{false};
    std::shared_ptr<FaultLogManager> faultLogManager_;
    std::shared_ptr<FaultLogBootScan> faultLogBootScan_;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif
