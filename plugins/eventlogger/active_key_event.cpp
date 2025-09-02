/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifdef MULTIMODALINPUT_INPUT_ENABLE
#include "active_key_event.h"

#include <vector>

#include "event_log_task.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "sys_event.h"
#include "time_util.h"
#ifdef HITRACE_CATCHER_ENABLE
#include "trace_collector.h"
#endif // HITRACE_CATCHER_ENABLE
#include "parameter_ex.h"
#include "freeze_manager.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    static const inline char* CMD_LIST[] = {
        "cmd:w",
        "cmd:rs",
        "cmd:a",
        "k:SysRqFile",
        "cmd:p",
        "cmd:d",
        "cmd:c",
    };
    static constexpr int REPORT_LIMIT = 3;
}
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-ActiveKeyEvent");
ActiveKeyEvent::ActiveKeyEvent()
{
    triggeringTime_ = 0;
}

ActiveKeyEvent::~ActiveKeyEvent()
{
    std::unique_lock<ffrt::mutex> uniqueLock(mutex_);
    for (auto it = subscribeIds_.begin(); it != subscribeIds_.end(); it = subscribeIds_.erase(it)) {
        if (*it >= 0) {
            MMI::InputManager::GetInstance()->UnsubscribeKeyEvent(*it);
            HIVIEW_LOGI("~ActiveKeyEvent subscribeId_: %{public}d", *it);
        }
    }
}

int64_t ActiveKeyEvent::SystemTimeMillisecond()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)((t.tv_sec) * TimeUtil::SEC_TO_NANOSEC + t.tv_nsec) / TimeUtil::SEC_TO_MICROSEC;
}

void ActiveKeyEvent::InitSubscribe(std::set<int32_t> preKeys, int32_t finalKey, int32_t count, int32_t holdTime)
{
    const int32_t maxCount = 5;
    if (++count > maxCount) {
        return;
    }
    std::shared_ptr<MMI::KeyOption> keyOption = std::make_shared<MMI::KeyOption>();
    if (keyOption == nullptr) {
        HIVIEW_LOGE("Invalid key option");
        return;
    }

    keyOption->SetPreKeys(preKeys);
    keyOption->SetFinalKey(finalKey);
    keyOption->SetFinalKeyDown(true);
    keyOption->SetFinalKeyDownDuration(holdTime);
    auto keyEventCallBack = [this] (std::shared_ptr<MMI::KeyEvent> keyEvent) {
        this->CombinationKeyCallback(keyEvent);
    };
    int32_t subscribeId = MMI::InputManager::GetInstance()->SubscribeKeyEvent(keyOption, keyEventCallBack);
    if (subscribeId < 0) {
        HIVIEW_LOGE("SubscribeKeyEvent failed, finalKey: %{public}d,"
            "subscribeId: %{public}d option failed.", finalKey, subscribeId);
        auto task = [this, preKeys, finalKey, count, holdTime] {
            this->InitSubscribe(preKeys, finalKey, count, holdTime);
            taskOutDeps++;
        };
        std::string taskName("InitSubscribe" + std::to_string(finalKey) + "_" + std::to_string(count));
        ffrt::submit(task, {}, {&taskOutDeps}, ffrt::task_attr().name(taskName.c_str()));
    }
    std::unique_lock<ffrt::mutex> uniqueLock(mutex_);
    subscribeIds_.emplace_back(subscribeId);
    HIVIEW_LOGI("CombinationKeyInit finalKey: %{public}d subscribeId_: %{public}d",
        finalKey, subscribeId);
}

void ActiveKeyEvent::Init()
{
    HIVIEW_LOGI("CombinationKeyInit");

    std::set<int32_t> prePowerKeys;
    prePowerKeys.insert(MMI::KeyEvent::KEYCODE_VOLUME_DOWN);
    auto initSubscribePower = [this, prePowerKeys] {
        this->InitSubscribe(prePowerKeys, MMI::KeyEvent::KEYCODE_POWER, 0, 500);
    };
    std::set<int32_t> preOnlyPowerKeys;
    auto initSubscribeOnlyPower = [this, preOnlyPowerKeys] {
        this->InitSubscribe(preOnlyPowerKeys, MMI::KeyEvent::KEYCODE_POWER, 0, 3000);
    };
    ffrt::submit(initSubscribePower, {}, {}, ffrt::task_attr().name("initSubscribePower").qos(ffrt::qos_default));
    ffrt::submit(initSubscribeOnlyPower, {}, {},
        ffrt::task_attr().name("initSubscribeOnlyPower").qos(ffrt::qos_default));
}

#ifdef HITRACE_CATCHER_ENABLE
void ActiveKeyEvent::HitraceCapture()
{
    std::shared_ptr<UCollectUtil::TraceCollector> collector = UCollectUtil::TraceCollector::Create();
    UCollect::TraceCaller caller = UCollect::TraceCaller::RELIABILITY;
    auto result = collector->DumpTrace(caller);
    if (result.retCode != 0) {
        HIVIEW_LOGE("get hitrace fail! error code: %{public}d", result.retCode);
        return;
    }
}
#endif // HITRACE_CATCHER_ENABLE

void ActiveKeyEvent::SysMemCapture(int fd)
{
    FileUtil::SaveStringToFd(fd, "\n\ncatcher cmd : /proc/meminfo\n");
    std::string content;
    FileUtil::LoadStringFromFile("/proc/meminfo", content);
    FileUtil::SaveStringToFd(fd, content);
}

void ActiveKeyEvent::DumpCapture(int fd)
{
    SysEventCreator sysEventCreator("HIVIEWDFX", "ACTIVE_KEY", SysEventCreator::FAULT);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("ActiveKeyEvent", nullptr, sysEventCreator);
    int noNeedJsonFd = -1;
    std::unique_ptr<EventLogTask> logTask = std::make_unique<EventLogTask>(fd, noNeedJsonFd, sysEvent);
    for (const std::string& cmd : CMD_LIST) {
        if (cmd == "k:SysRqFile") {
            auto sysRqTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
            std::string formatSysRqTime = TimeUtil::TimestampFormatToDate(sysRqTime, "%Y%m%d%H%M%S");
            sysEvent->SetEventValue("SYSRQ_TIME", formatSysRqTime);
        }
        logTask->AddLog(cmd);
    }

    auto ret = logTask->StartCompose();
    if (ret != EventLogTask::TASK_SUCCESS) {
        HIVIEW_LOGE("capture fail %{public}d", ret);
    }
    SysMemCapture(fd);
}

void ActiveKeyEvent::CombinationKeyHandle(std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    HIVIEW_LOGI("Receive CombinationKeyHandle.");
    if (!Parameter::IsBetaVersion() || reportLimit_ > REPORT_LIMIT) {
        HIVIEW_LOGI("Don't report ACTIVE_KEY_EVENT");
        return;
    }

    std::string logFile = "ACTIVE_KEY_EVENT-0-" +
        TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
        "%Y%m%d%H%M%S") + ".log";
    if (FileUtil::FileExists("/data/log/eventlog/" + logFile)) {
        HIVIEW_LOGW("filename: %{public}s is existed, direct use.", logFile.c_str());
        return;
    }
    int fd = FreezeManager::GetInstance()->GetFreezeLogFd(FreezeLogType::EVENTLOG, logFile);
    if (fd < 0) {
        HIVIEW_LOGE("failed to create file=%{public}s, errno=%{public}d", logFile.c_str(), errno);
        return;
    }

    auto sysStart = ActiveKeyEvent::SystemTimeMillisecond();
    const uint32_t placeholder = 3;
    auto start = TimeUtil::GetMilliseconds();
    uint64_t startTime = start / TimeUtil::SEC_TO_MILLISEC;
    std::ostringstream startTimeStr;
    startTimeStr << "start time: " << TimeUtil::TimestampFormatToDate(startTime, "%Y/%m/%d-%H:%M:%S");
    startTimeStr << ":" << std::setw(placeholder) << std::setfill('0') <<
        std::to_string(start % TimeUtil::SEC_TO_MILLISEC) << std::endl;
    std::vector<int32_t> keys = keyEvent->GetPressedKeys();
    for (auto& i : keys) {
        startTimeStr << "CombinationKeyCallback key : ";
        startTimeStr << MMI::KeyEvent::KeyCodeToString(i) << std::endl;
    }
    FileUtil::SaveStringToFd(fd, startTimeStr.str());

#ifdef HITRACE_CATCHER_ENABLE
    auto hitraceCapture = [this] { this->HitraceCapture(); };
    ffrt::submit(hitraceCapture, {}, {}, ffrt::task_attr().name("HitraceCapture").qos(ffrt::qos_user_initiated));
#endif // HITRACE_CATCHER_ENABLE

    DumpCapture(fd);
    auto end = ActiveKeyEvent::SystemTimeMillisecond();
    std::string totalTime = "\n\nCatcher log total time is " + std::to_string(end - sysStart) + "ms\n";
    FileUtil::SaveStringToFd(fd, totalTime);
    close(fd);
    reportLimit_++;
}

void ActiveKeyEvent::CombinationKeyCallback(std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    HIVIEW_LOGI("Receive CombinationKeyCallback key id: %{public}d.", keyEvent->GetId());
    uint64_t now = (uint64_t)ActiveKeyEvent::SystemTimeMillisecond();
    const uint64_t interval = 10000;
    if (now - triggeringTime_ < interval) {
        return;
    }
    triggeringTime_ = now;
    auto combinationKeyHandle = [this, keyEvent] { this->CombinationKeyHandle(keyEvent); };
    ffrt::submit(combinationKeyHandle, {}, {},
        ffrt::task_attr().name("ActiveKeyEvent").qos(ffrt::qos_user_initiated));
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // MULTIMODALINPUT_INPUT_ENABLE
