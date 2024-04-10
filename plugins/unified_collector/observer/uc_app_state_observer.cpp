/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "uc_app_state_observer.h"

#include "logger.h"
#include "process_status.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-UnifiedCollector");
using namespace OHOS::HiviewDFX::UCollectUtil;

void UcAppStateObserver::OnProcessCreated(const AppExecFwk::ProcessData& processData)
{
    HIVIEW_LOGD("process=%{public}d created", processData.pid);
    ProcessStatus::GetInstance().NotifyProcessState(processData.pid, CREATED);
}

void UcAppStateObserver::OnProcessDied(const AppExecFwk::ProcessData& processData)
{
    HIVIEW_LOGD("process=%{public}d died", processData.pid);
}
}  // namespace HiviewDFX
}  // namespace OHOS
