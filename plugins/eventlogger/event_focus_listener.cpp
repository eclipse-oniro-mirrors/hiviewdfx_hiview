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
#ifdef WINDOW_MANAGER_ENABLE
#include "event_focus_listener.h"

#include "ffrt.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D01, "EventFocusListener");
sptr<EventFocusListener> EventFocusListener::instance_ = nullptr;
std::recursive_mutex EventFocusListener::mutex_;
EventFocusListener::REGISTER_STATE EventFocusListener::registerState_ = UNREGISTERED;
uint64_t EventFocusListener::lastChangedTime_ = TimeUtil::GetMilliseconds();

sptr<EventFocusListener> EventFocusListener::GetInstance()
{
    std::lock_guard<std::recursive_mutex> lock_l(mutex_);
    if (instance_ == nullptr) {
        instance_ = new (std::nothrow) EventFocusListener();
    }
    return instance_;
}

void EventFocusListener::RegisterFocusListener()
{
    if (!Parameter::IsBetaVersion()) {
        return;
    }

    if (registerState_ == REGISTERED) {
        HIVIEW_LOGD("eventFocusListener is registered");
        return;
    }
    if (GetInstance() == nullptr) {
        HIVIEW_LOGE("register eventFocusListener failed, listener is null");
        return;
    }
    if (registerState_ == REGISTERING) {
        HIVIEW_LOGI("register eventFocusListener task is executing");
        return;
    }

    auto registerTask = [] {
        registerState_ = REGISTERING;
        Rosen::WMError ret = Rosen::WindowManager::GetInstance().RegisterFocusChangedListener(GetInstance());
        if (ret == Rosen::WMError::WM_OK) {
            HIVIEW_LOGI("register eventFocusListener succeed.");
            registerState_ = REGISTERED;
        } else {
            HIVIEW_LOGI("register eventFocusListener failed.");
            registerState_ = UNREGISTERED;
        }
    };
    HIVIEW_LOGI("before submit registerFocusListener task to ffrt");
    ffrt::submit(registerTask, {}, {}, ffrt::task_attr().name("rgs_fcs_lst"));
    HIVIEW_LOGI("after submit registerFocusListener task to ffrt");
}

void EventFocusListener::UnRegisterFocusListener()
{
    if (registerState_ != REGISTERED) {
        HIVIEW_LOGD("eventFocusListener not need unRegistered");
        return;
    }
    if (GetInstance() == nullptr) {
        HIVIEW_LOGE("unRegister eventFocusListener failed, listener is null");
        return;
    }

    Rosen::WMError ret = Rosen::WindowManager::GetInstance().UnregisterFocusChangedListener(GetInstance());
    if (ret == Rosen::WMError::WM_OK) {
        HIVIEW_LOGI("unRegister eventFocusListener succeed");
        registerState_ = UNREGISTERED;
    }
}

void EventFocusListener::OnFocused(const sptr<Rosen::FocusChangeInfo>& focusChangeInfo)
{
    lastChangedTime_ = TimeUtil::GetMilliseconds();
}

void EventFocusListener::OnUnfocused(const sptr<Rosen::FocusChangeInfo>& focusChangeInfo)
{
    lastChangedTime_ = TimeUtil::GetMilliseconds();
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // WINDOW_MANAGER_ENABLE
