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
#ifndef DFR_SYSLOAD_CONFIG_PARSER_H
#define DFR_SYSLOAD_CONFIG_PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <singleton.h>
#include "cJSON.h"

#define MODULE_PARAMS_MAGIC 0xE7E3A150
#define MODULE_PARAMS_SIZE static_cast<uint32_t>(sizeof(ModuleParams))
#define MODULE_NAME_SIZE 32
#define BF_SIZE_16 16 // byte
#define BF_SIZE_64 64
#define BF_SIZE_512 512

inline const std::vector<std::string> LEVEL_KEYS = {
    "UNKNOWN",
    "NORMAL",
    "WARNING",
    "FAULTY",
    "CRITICAL"
};

namespace OHOS {
namespace HiviewDFX {

enum LevelRandEnum {
    LEVEL_UNKNOWN = 0,
    LEVEL_NORMAL,
    LEVEL_WARNING,
    LEVEL_FAULTY,
    LEVEL_CRITICAL,
    LEVEL_NUMBER
};

struct SysloadAcquisition {
    int enableTrace {0};
    int enablePerf {0};
    int enableProfiler {0};
    bool isTelemetry {false};
};

struct MonitorParams {
    int logTopProcess {0};
    int logTopThread {0};
    int checkInterval {120};
    int isSave {0};
    long upperThreshold {70};
    long lowerThreshold {0};
    int onlySummaryLog {0};
    int enableKmsg {0};
};

struct ModuleParams {
    uint32_t magic {MODULE_PARAMS_MAGIC};
    bool isEnable {false};
    int handlePeriod {0};
    int validWindowTime {0};
    int resetAfterTime {0};
    char name[MODULE_NAME_SIZE];
    MonitorParams monitors[LEVEL_NUMBER];
    uint32_t magicSize {MODULE_PARAMS_SIZE};
};

struct SysloadEventUnit {
    char faultType[BF_SIZE_16];
    char faultLevel[BF_SIZE_16];
    char kmsgPath[BF_SIZE_64];
    char pidlist[BF_SIZE_512];
};

struct SysloadEventCombined {
    unsigned int magic;
    int structSize;
    bool memEventValid;
    bool cpuEventValid;
    struct SysloadEventUnit memEvent;
    struct SysloadEventUnit cpuEvent;
};

class SysloadConfigParser : public Singleton<SysloadConfigParser> {
    DECLARE_SINGLETON(SysloadConfigParser);
public:
    ModuleParams& GetModuleInfo(const std::string &moduleName);
    std::vector<SysloadAcquisition>& GetAcquisitionInfo(const std::string &moduleName);
    void DoParseSysload(const cJSON *node);

private:
    void UpdateModule(const cJSON *node, ModuleParams &module, std::vector<SysloadAcquisition> &acquisitions);
    void UpdateMonitor(const cJSON *node, MonitorParams &monitor);
    void UpdateAcquisition(const cJSON *node, SysloadAcquisition &acquisition);

private:
    ModuleParams cpuModule_ { MODULE_PARAMS_MAGIC, false, 10, 300, 600, "cpu_module" };
    ModuleParams memModule_ { MODULE_PARAMS_MAGIC, false, 2, 300, 600, "mem_module" };
    std::vector<SysloadAcquisition> cpuAcquisition_;
    std::vector<SysloadAcquisition> memAcquisition_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_SYSLOAD_CONFIG_PARSER_H
