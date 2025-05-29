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
#include "event_param_watcher.h"

#include "event_validator.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("EventParamWatcher");
namespace {
void ParameterWatchCallback(const char* key, const char* value, void* context)
{
    if (context == nullptr) {
        HIVIEW_LOGE("context is null");
        return;
    }
    auto watcher = reinterpret_cast<EventParamWatcher*>(context);
    if (watcher == nullptr) {
        HIVIEW_LOGE("watcher is null");
        return;
    }
    constexpr size_t maxLen = 256;
    std::string testTypeStr(value);
    if (testTypeStr.size() > maxLen) {
        HIVIEW_LOGE("invalid length, strLen=%{public}zu, maxLen=%{public}zu", testTypeStr.size(), maxLen);
        return;
    }
    watcher->UpdateTestType(testTypeStr);
    HIVIEW_LOGI("test_type is set to be \"%{public}s\"", testTypeStr.c_str());
}
}

void EventParamWatcher::Init()
{
    constexpr char testTypeParamKey[] = "hiviewdfx.hiview.testtype";
    if (Parameter::WatchParamChange(testTypeParamKey, ParameterWatchCallback, this) != 0) {
        HIVIEW_LOGW("failed to watch the change of parameter %{public}s", testTypeParamKey);
    }
}

std::string EventParamWatcher::GetTestType()
{
    std::shared_lock<std::shared_mutex> lock(testTypeMutex_);
    return testType_;
}

void EventParamWatcher::UpdateTestType(const std::string& testType)
{
    std::unique_lock<std::shared_mutex> lock(testTypeMutex_);
    testType_ = testType;
}
} // namespace HiviewDFX
} // namespace OHOS
