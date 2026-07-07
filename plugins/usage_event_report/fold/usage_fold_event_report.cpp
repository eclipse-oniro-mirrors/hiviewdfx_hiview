/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
#include "usage_fold_event_report.h"

#include "display_manager_lite.h"
#include "hiview_logger.h"
#include "logger_event.h"
#include "parameter_ex.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("UsageFoldEventReport");
namespace {
constexpr char BOOT_COMPLETED_KEY[] = "bootevent.boot.completed";
}

void UsageFoldEventReport::Init(const std::string& workPath)
{
    workPath_ = workPath;
#if FOLD_PC_COUNT_DURATION_ENABLE
    Load(workPath);
#else
    Parameter::WatchParamChange(BOOT_COMPLETED_KEY, OnBootCompletedChange, this);
    std::string bootCompleted = Parameter::GetString(BOOT_COMPLETED_KEY, "");
    if (bootCompleted == "true") {
        Load(workPath);
    }
#endif // FOLD_PC_COUNT_DURATION_ENABLE
}

void UsageFoldEventReport::OnBootCompletedChange(const char *key, const char *value, void *context)
{
    if (context == nullptr || key == nullptr || value == nullptr) {
        HIVIEW_LOGE("invalid param.");
        return;
    }
    if (strcmp(value, "true") != 0) {
        return;
    }
    UsageFoldEventReport *report = static_cast<UsageFoldEventReport *>(context);
    if (report == nullptr) {
        HIVIEW_LOGE("invalid param.");
        return;
    }
    report->Load(report->workPath_);
}

void UsageFoldEventReport::Load(const std::string& workPath)
{
    static std::once_flag initFlag;
    std::call_once(initFlag, [this, &workPath]() {
        if (!OHOS::Rosen::DisplayManagerLite::GetInstance().IsFoldable()) {
            HIVIEW_LOGI("non-foldable device");
            return;
        }
        foldEventCacher_ = std::make_unique<FoldEventCacher>(workPath);
        foldAppUsageFactory_ = std::make_unique<FoldAppUsageEventFactory>(workPath);
        Parameter::RemoveParameterWatcherEx(BOOT_COMPLETED_KEY, OnBootCompletedChange, nullptr);
    });
}

void UsageFoldEventReport::ProcessEvent(std::shared_ptr<Event> event)
{
    if (foldEventCacher_ != nullptr) {
        auto sysEvent = Event::DownCastTo<SysEvent>(event);
        foldEventCacher_->ProcessEvent(sysEvent);
    }
}

void UsageFoldEventReport::ReportEvent()
{
    if (foldAppUsageFactory_ == nullptr) {
        HIVIEW_LOGI("foldAppUsageFactory is nullptr");
        return;
    }
    std::vector<std::unique_ptr<LoggerEvent>> foldAppUsageEvents;
    foldAppUsageFactory_->Create(foldAppUsageEvents);
    HIVIEW_LOGI("report fold app usage event num: %{public}zu", foldAppUsageEvents.size());
    for (size_t i = 0; i < foldAppUsageEvents.size(); ++i) {
        foldAppUsageEvents[i]->Report();
    }
}
}  // namespace HiviewDFX
}  // namespace OHOS
