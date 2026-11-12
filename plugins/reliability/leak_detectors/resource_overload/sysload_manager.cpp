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

#include "sysload_manager.h"

#include <fstream>

#include "fault_common.h"
#include "dfr_util.h"
#include "old_log_cleaner.h"
#include "native_leak_detector.h"

namespace {
    constexpr const char *FAULT_SYSLOAD_TYPE = "131072";
    constexpr const char *SYSLOAD_DEVICE_PATH = "/dev/sysload";
    const std::vector<std::string> g_sysloadModules = {"sysload_cpu", "sysload_mem"};
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "SysloadManager");
SysloadManager::SysloadManager()
{
    sysloadFd_ = open(SYSLOAD_DEVICE_PATH, O_RDWR);
    if (sysloadFd_ < 0) {
        HIVIEW_LOGE("open device failed.");
        return;
    }
    fdsan_exchange_owner_tag(sysloadFd_, 0, FDSAN_TAG);
    qos_ = ffrt_this_task_get_qos();
    EpollInit();
}

SysloadManager::~SysloadManager()
{
    if (sysloadFd_ >= 0) {
        ffrt_epoll_ctl(qos_, EPOLL_CTL_DEL, sysloadFd_, 0, nullptr, nullptr);
        fdsan_close_with_tag(sysloadFd_, FDSAN_TAG);
        sysloadFd_ = -1;
    }
}

void SysloadManager::MainBusiness()
{
    if (sysloadFd_ < 0) {
        HIVIEW_LOGE("open device failed.");
        return;
    }
    if (SysloadThresholdsIssued(sysloadFd_)) {
        if (!StartSysloadMainWork(sysloadFd_)) {
            HIVIEW_LOGE("start sysload failed.");
        }
    } else {
        HIVIEW_LOGE("Sysload Thresholds not used.");
    }
}

SysloadAcquisition SysloadManager::GetAcquisitionInfo(const std::string &faultType, const std::string &faultLevel)
{
    SysloadAcquisition ret = {0};
    auto acquisitions = SysloadConfigParser::GetInstance().GetAcquisitionInfo("sysload_" + faultType);

    for (int i = LEVEL_NORMAL; i < LEVEL_NUMBER; ++i) {
        if (faultLevel == LEVEL_KEYS[i]) {
            ret = acquisitions.at(i);
            break;
        }
    }
    return ret;
}

void SysloadManager::UpdateConfigInfo(const std::string &valuePair,
    SysloadAcquisition &acquisitions, MonitorParams &params)
{
    std::vector<std::string> tmps = DfrUtil::GetInstance().Split(valuePair, ',');

    for (const std::string& str : tmps) {
        if (str == "only_summary_log") {
            params.onlySummaryLog = 1;
            continue;
        }
        if (str == "enable_kmsg") {
            params.enableKmsg = 1;
            continue;
        }
        if (str == "enable_trace") {
            acquisitions.enableTrace = 1;
            continue;
        }
        if (str == "enable_perf") {
            acquisitions.enablePerf = 1;
            continue;
        }
        if (str == "enable_profiler") {
            acquisitions.enableProfiler = 1;
            continue;
        }
    }

    if (!params.isSave) {
        params.isSave = params.onlySummaryLog || params.enableKmsg;
    }
}

void SysloadManager::HandleTelemetryStart(const std::map<std::string, std::string> &valuePairs)
{
    int fd = open(SYSLOAD_DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        HIVIEW_LOGE("open device failed.");
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);

    for (const std::string& moduleName : g_sysloadModules) {
        auto& acquisitions = SysloadConfigParser::GetInstance().GetAcquisitionInfo(moduleName);
        if (acquisitions.size() != LEVEL_NUMBER) {
            continue;
        }
        ModuleParams& module = SysloadConfigParser::GetInstance().GetModuleInfo(moduleName);

        for (int i = LEVEL_NORMAL; i < LEVEL_NUMBER; ++i) {
            acquisitions.at(i).isTelemetry = true;
            std::string key = moduleName + "-" + LEVEL_KEYS[i];
            if (valuePairs.find(key) != valuePairs.end()) {
                UpdateConfigInfo(valuePairs.at(key), acquisitions.at(i), module.monitors[i]);
            }
        }
        HIVIEW_LOGI("start ThresholdDistribution modele: %{public}s", moduleName.c_str());
        if (!ThresholdDistribution(module, fd)) {
            HIVIEW_LOGW("ThresholdDistribution failed for module: %{public}s", moduleName.c_str());
        }
    }
    fdsan_close_with_tag(fd, FDSAN_TAG);
}

void SysloadManager::HandleTelemetryStop(void)
{
    int fd = open(SYSLOAD_DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        HIVIEW_LOGE("open device failed.");
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    SysloadThresholdsIssued(fd);
    fdsan_close_with_tag(fd, FDSAN_TAG);
}

void SysloadManager::OnUnorderedEvent(std::map<std::string, std::string> valuePairs)
{
    std::string telemetryId = valuePairs["telemetryId"];
    std::string reliabilityType = valuePairs["reliabilityType"];
    if (telemetryId.empty() || valuePairs["fault"] != FAULT_SYSLOAD_TYPE || reliabilityType != "1") {
        return;
    }
    if (valuePairs["telemetryStatus"] == "off") {
        // 关闭灰度
        HandleTelemetryStop();
    } else if (valuePairs["telemetryStatus"] == "on") {
        // 开启灰度
        HandleTelemetryStart(valuePairs);
    } else {
        HIVIEW_LOGE("unknown telemetryStatus, value: %{public}s", valuePairs["telemetryStatus"].c_str());
        return;
    }
}

bool SysloadManager::SysloadThresholdsIssued(int fd)
{
    size_t successfulNumber = 0;
    for (const std::string& moduleName : g_sysloadModules) {
        ModuleParams& param = SysloadConfigParser::GetInstance().GetModuleInfo(moduleName);
        if (!param.isEnable) {
            HIVIEW_LOGI("module %{public}s is not enabled.", moduleName.c_str());
            continue;
        }

        if (!ThresholdDistribution(param, fd)) {
            HIVIEW_LOGE("send threshold info for module %{public}s failed.", moduleName.c_str());
            continue;
        }
        successfulNumber++;
    }
    if (successfulNumber == 0) {
        return false;
    }
    return true;
}

bool SysloadManager::StartSysloadMainWork(int fd)
{
    if (ioctl(fd, IOCTL_START_RUN_MAIN_WORK) < 0) {
        HIVIEW_LOGE("sysload issue ioctl failed.");
        return false;
    }
    return true;
}

bool SysloadManager::ThresholdDistribution(const ModuleParams &params, int fd)
{
    if (ioctl(fd, IOCTL_SET_MONITOR_PARAMS, params) < 0) {
        HIVIEW_LOGE("threshold distribution failed.");
        return false;
    }
    HIVIEW_LOGI("threshold distribution success");
    return true;
}

std::string SysloadManager::SafeCStrToString(const char* cstr, size_t maxLen)
{
    if (cstr == nullptr || maxLen == 0) {
        return "";
    }
    size_t actualLen = 0;
    while (actualLen < maxLen && cstr[actualLen] != '\0') {
        actualLen++;
    }
    return std::string(cstr, actualLen);
}

void SysloadManager::ProcessSingleSysloadEvent(const struct SysloadEventUnit& eventUnit)
{
    std::string faultType = SafeCStrToString(eventUnit.faultType, sizeof(eventUnit.faultType));
    std::string faultLevel = SafeCStrToString(eventUnit.faultLevel, sizeof(eventUnit.faultLevel));
    std::string kmsgPath = SafeCStrToString(eventUnit.kmsgPath, sizeof(eventUnit.kmsgPath));
    std::string pidlist = SafeCStrToString(eventUnit.pidlist, sizeof(eventUnit.pidlist));

    if (faultType != "mem" && faultType != "cpu") {
        HIVIEW_LOGE("invalid event type, type:%{public}s", faultType.c_str());
        return;
    }

    if (faultLevel.empty()) {
        HIVIEW_LOGE("invalid event fields, level:%{public}s", faultLevel.c_str());
        return;
    }

    OldLogCleaner::GetInstance().AgingSysloadLog(faultType);
    SysloadAcquisition sysloadAcquisition = SysloadManager::GetInstance().GetAcquisitionInfo(faultType, faultLevel);
    HIVIEW_LOGI("received sysload event, type:%{public}s, level:%{public}s, pidlist:%{public}s",
        faultType.c_str(), faultLevel.c_str(), pidlist.c_str());
    NativeLeakDetector::GetInstance().ProcessSysloadDetectorInfo(pidlist, faultType, sysloadAcquisition);
}

void SysloadManager::HandleSysloadEvent(void* token, uint32_t event)
{
    if (!token) {
        HIVIEW_LOGE("token is null");
        return;
    }

    int fd = *reinterpret_cast<int*>(token);
    if (fd < 0) {
        HIVIEW_LOGE("open device failed.");
        return;
    }

    SysloadEventCombined sysloadEvent = {0};
    sysloadEvent.magic = SYSLOAD_DETECTOR_MAGIC;
    sysloadEvent.structSize = sizeof(struct SysloadEventCombined);

    int res = ioctl(fd, IOCTL_GET_EVENT_INFO, &sysloadEvent);
    if (res < 0) {
        HIVIEW_LOGE("ioctl failed, res:%{public}d, errno:%{public}d, err desc:%{public}s",
                    res, errno, strerror(errno));
        return;
    }

    if (sysloadEvent.memEventValid) {
        SysloadManager::GetInstance().ProcessSingleSysloadEvent(sysloadEvent.memEvent);
    }
    if (sysloadEvent.cpuEventValid) {
        SysloadManager::GetInstance().ProcessSingleSysloadEvent(sysloadEvent.cpuEvent);
    }
}

void SysloadManager::EpollInit(void)
{
    int ret = ffrt_epoll_ctl(qos_, EPOLL_CTL_ADD, sysloadFd_, EPOLLIN, (void*)&sysloadFd_,
        SysloadManager::HandleSysloadEvent);
    if (ret != 0) {
        HIVIEW_LOGE("recv sysload event failed fd:%{public}d ret:%{public}d end.", sysloadFd_, ret);
        return;
    }

    HIVIEW_LOGI("create recv sysload event success");
}

} // namespace HiviewDFX
} // namespace OHOS