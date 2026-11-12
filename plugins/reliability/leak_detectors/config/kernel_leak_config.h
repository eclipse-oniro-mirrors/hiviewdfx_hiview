/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#ifndef DFR_KERNEL_LEAK_CONFIG_H
#define DFR_KERNEL_LEAK_CONFIG_H

#include <map>
#include <vector>
#include <string>

#include <json/json.h>
#include <singleton.h>
#include "leak_json_parser.h"
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {
struct PsiThresholdParam {
    int rabThreshold { DEFAULT_RAB_THRESHOLD_MB };
    int staThreshold { DEFAULT_STA_THRESHOLD_MB };
};

struct RSWatchpointParam {
    int processWarningThreshold { INT_MAX };
    int processCpuControlThreshold { INT_MAX };
    int processGpuControlThreshold { INT_MAX };
    int totalThreshold { INT_MAX };

    void Print() const;
};

struct DmaBufThresholdParam {
    int detectThreshold {INT_MAX};
    int appDefaultKillThreshold {INT_MAX};
    std::map<std::string, int> appThresholdList;
};

struct KernelTypeParam {
    std::string name;
    std::string dumpFunction;
    std::string meminfoName;
    std::string procName;
    bool enable{false};
    bool killEnable {false};
    bool dumpKernelStack {true};
    int softThreshold{INT_MAX};
    int hardThreshold{INT_MAX};
    int totalThreshold{INT_MAX};
    int appKillThreshold{INT_MAX};
    int appKillHardThreshold{INT_MAX};
    int nativeKillThreshold{INT_MAX};

    void Print() const;
};

using CommonDumpTypeMap = std::map<CommonDumpType, KernelTypeParam>;
struct KernelParam {
    bool enable { true };
    time_t uploadInterval { 18000 };
    std::unordered_map<std::string, ConfigValueStru> commonKeyMap_;
    std::vector<std::string> gpuKillWhitelist;
    std::map<std::string, KernelTypeParam> kernelType;
    RSWatchpointParam rsWatchpoint;
    DmaBufThresholdParam dmaBufThreshold;
    PsiThresholdParam psiThresholdParam;
};

class KernelLeakConfig : public Singleton<KernelLeakConfig> {
    DECLARE_SINGLETON(KernelLeakConfig);
public:
    void UpdatePolicy(const cJSON *node);
    void UpdateQuota(const cJSON *node);
    void Print();
    inline const PsiThresholdParam &GetPsiThresholdParam()
    {
        return kernelParam_.psiThresholdParam;
    }
    inline const RSWatchpointParam &GetRsWatchpoint()
    {
        return kernelParam_.rsWatchpoint;
    }
    inline const DmaBufThresholdParam &GetDmaBufThreshold()
    {
        return kernelParam_.dmaBufThreshold;
    }
    inline const KernelTypeParam &GetTypeParam(std::string name)
    {
        return kernelParam_.kernelType[name];
    }
    inline const KernelParam &GetKernelConfig()
    {
        return kernelParam_;
    }
    inline ConfigValueStru GetCommonKeyValue(const std::string &key)
    {
        return kernelParam_.commonKeyMap_[key];
    }
    inline bool IsInGpuKillWhitelist(const std::string &processName)
    {
        return std::find(kernelParam_.gpuKillWhitelist.begin(), kernelParam_.gpuKillWhitelist.end(), processName) !=
            kernelParam_.gpuKillWhitelist.end();
    }
    inline bool IsInVmaKillList(const std::string &name)
    {
        return find(vmaKillList_.begin(), vmaKillList_.end(), name) != vmaKillList_.end();
    }
    inline std::string GetDumpFunction(const std::string &name)
    {
        auto it = kernelParam_.kernelType.find(name);
        if (it == kernelParam_.kernelType.end()) {
            return "";
        }
        return it->second.dumpFunction;
    }
    inline int GetKillEnable(std::string name)
    {
        return kernelParam_.kernelType[name].killEnable;
    }
    inline bool GetDumpKernelStack(std::string name)
    {
        return kernelParam_.kernelType[name].dumpKernelStack;
    }
    inline int32_t GetProfilerTime()
    {
        return profilerTime_;
    }
    inline bool GetAsyncStackEnable()
    {
        return asyncStackEnable_;
    }
private:
    void UpdateKernelType(const cJSON *node);
    void UpdatePerType(const cJSON *node, const std::string &typeName);
    void UpdateRsWatchPoint(const cJSON *node);
    void UpdateDmaBufThreshold(const cJSON *node);
    void UpdateVmaKillList(const cJSON *node);
    void UpdatePsiThreshold(const cJSON *node);

public:
    const CommonDumpTypeMap &GetProcNodeMap();

private:
    KernelParam kernelParam_;
    CommonDumpTypeMap procNodeMap_;
    std::vector<std::string> vmaKillList_;
    int32_t profilerTime_ { 0 };
    bool asyncStackEnable_ { false };
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_KERNEL_LEAK_CONFIG_H
