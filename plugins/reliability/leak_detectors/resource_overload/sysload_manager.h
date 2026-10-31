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

#ifndef DFR_SYSLOAD_MANAGER_H
#define DFR_SYSLOAD_MANAGER_H

#include "singleton.h"
#include <map>
#include <unordered_map>
#include <memory>
#include <string>
#include <cJSON.h>
#include "c/executor_task.h"
#include "leak_json_parser.h"
#include "sysload_config_parser.h"
#include "event.h"

#define SUMMARY_LOG_BASE 'S'
#define START_RUN_MAIN_WORK 0x03
#define SET_MONITOR_PARAMS 0x02
#define GET_EVENT_INFO 0x08
#define IOCTL_START_RUN_MAIN_WORK _IO(SUMMARY_LOG_BASE, START_RUN_MAIN_WORK)
#define IOCTL_SET_MONITOR_PARAMS _IOW(SUMMARY_LOG_BASE, SET_MONITOR_PARAMS, ModuleParams)
#define IOCTL_GET_EVENT_INFO _IOWR(SUMMARY_LOG_BASE, GET_EVENT_INFO, int32_t)

namespace OHOS {
namespace HiviewDFX {

/* The class that controls Sysload startup and behaviors such as threshold distribution. */
class SysloadManager final : public Singleton<SysloadManager> {
    DECLARE_SINGLETON(SysloadManager);

public:
    void MainBusiness(); // External interface
    void OnUnorderedEvent(std::map<std::string, std::string> valuePairs);
    SysloadAcquisition GetAcquisitionInfo(const std::string &faultType, const std::string &faultLevel);

private:
    bool SysloadThresholdsIssued(int fd); // Threshold distribution
    bool StartSysloadMainWork(int fd); // Main thread started
    bool ThresholdDistribution(const ModuleParams &params, int fd); // Thresholds successfully obtained
    void HandleTelemetryStart(const std::map<std::string, std::string> &valuePairs);
    void HandleTelemetryStop(void);
    void UpdateConfigInfo(const std::string &valuePair, SysloadAcquisition &acquisitions, MonitorParams &params);
    static void HandleSysloadEvent(void* token, uint32_t event);
    std::string SafeCStrToString(const char* cstr, size_t maxLen);
    void ProcessSingleSysloadEvent(const struct SysloadEventUnit &eventUnit);
    void EpollInit(void);

    ffrt_qos_t qos_ = ffrt_qos_default;
    int sysloadFd_ = -1;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_SYSLOAD_MANAGER_H
