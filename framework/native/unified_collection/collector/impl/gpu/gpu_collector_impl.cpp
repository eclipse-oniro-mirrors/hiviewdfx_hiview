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

#include "gpu_collector_impl.h"

#include <sstream>

#include "common_util.h"
#include "file_util.h"
#include "gpu_decorator.h"
#include "hiview_logger.h"

using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("UCollectUtil");
constexpr char GPU_LOAD[] = "/sys/class/devfreq/gpufreq/gpu_scene_aware/utilisation";
constexpr char DEVFREQ_BASE[]    = "/sys/class/devfreq";
}

void GpuCollectorImpl::DetectGpuDevfreqPath()
{
    if (pathDetected_) {
        return;
    }
    
    // 第一级: 精确匹配已知路径
    const std::vector<std::string> knownPaths = {
        "/sys/class/devfreq/gpufreq",           // 默认
        "/sys/class/devfreq/fde60000.gpu",      // RK3568
        "/sys/class/devfreq/23140000.gpu",      // P7885
    };
    
    for (const auto& path : knownPaths) {
        std::string curFreqPath = path + "/cur_freq";
        if (FileUtil::FileExists(curFreqPath)) {
            gpuDevfreqPath_ = path;
            HIVIEW_LOGI("GPU path detected (exact): %{public}s", path.c_str());
            pathDetected_ = true;
            return;
        }
    }
    
    // 第二级: 模糊匹配 - 扫描 devfreq 目录找含 gpu 的目录
    DIR* dp = opendir(DEVFREQ_BASE);
    if (dp == nullptr) {
        HIVIEW_LOGW("Cannot open devfreq directory");
        gpuDevfreqPath_ = "";
        pathDetected_ = true;
        return;
    }
    
    struct dirent* dirp;
    while ((dirp = readdir(dp)) != nullptr) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) {
            continue;
        }
        
        std::string name(dirp->d_name);
        if (name.find("gpu") != std::string::npos || name.find("GPU") != std::string::npos) {
            std::string fullPath = std::string(DEVFREQ_BASE) + "/" + name;
            std::string curFreqPath = fullPath + "/cur_freq";
            if (FileUtil::FileExists(curFreqPath)) {
                gpuDevfreqPath_ = fullPath;
                HIVIEW_LOGI("GPU path detected (fuzzy): %{public}s", fullPath.c_str());
                closedir(dp);
                pathDetected_ = true;
                return;
            }
        }
    }
    closedir(dp);
    
    // 第三级: 未找到
    gpuDevfreqPath_ = "";
    HIVIEW_LOGW("GPU devfreq path not found");
    pathDetected_ = true;
}

std::shared_ptr<GpuCollector> GpuCollector::Create()
{
    return std::make_shared<GpuDecorator>(std::make_shared<GpuCollectorImpl>());
}

CollectResult<GpuFreq> GpuCollectorImpl::CollectGpuFrequency()
{
    CollectResult<GpuFreq> result;
    GpuFreq& gpuFreq = result.data;
    
    if (!pathDetected_) {
        DetectGpuDevfreqPath();
    }

    if (gpuDevfreqPath_.empty()) {
        HIVIEW_LOGW("GPU Devfreq path not available, return 0");
        gpuFreq.curFeq = 0;
        gpuFreq.maxFeq = 0;
        gpuFreq.minFeq = 0;
        result.retCode = UcError::SUCCESS;
        return result;
    }

    std::string curFreqPath = gpuDevfreqPath_ + "/cur_freq";
    std::string maxFreqPath = gpuDevfreqPath_ + "/max_freq";
    std::string minFreqPath = gpuDevfreqPath_ + "/min_freq";
    
    gpuFreq.curFeq = CommonUtil::ReadNodeWithOnlyNumber(curFreqPath);
    gpuFreq.maxFeq = CommonUtil::ReadNodeWithOnlyNumber(maxFreqPath);
    gpuFreq.minFeq = CommonUtil::ReadNodeWithOnlyNumber(minFreqPath);
    HIVIEW_LOGD("curFeq=%{public}d,maxFeq=%{public}d,minFeq=%{public}d",
        gpuFreq.curFeq, gpuFreq.maxFeq, gpuFreq.minFeq);
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<SysGpuLoad> GpuCollectorImpl::CollectSysGpuLoad()
{
    CollectResult<SysGpuLoad> result;
    SysGpuLoad& sysGpuLoad = result.data;
    
    if (!pathDetected_) {
        DetectGpuDevfreqPath();
    }
    
    if (gpuDevfreqPath_.empty()) {
        HIVIEW_LOGW("GPU Devfreq path not available, return 0");
        sysGpuLoad.gpuLoad = 0;
        result.retCode = UcError::SUCCESS;
        return result;
    }
    
    // load 节点路径处理:
    // 默认逻辑: /sys/class/devfreq/gpufreq/gpu_scene_aware/utilisation (直接数字)
    // RK3568/7885: /sys/class/devfreq/xxx.gpu/load (格式: "负载@频率Hz")
    
    std::string loadPath;
    if (gpuDevfreqPath_ == "/sys/class/devfreq/gpufreq") {
        loadPath = GPU_LOAD;
    } else {
        // 其他设备使用标准 load 节点
        loadPath = gpuDevfreqPath_ + "/load";
    }
    
    if (!FileUtil::FileExists(loadPath)) {
        HIVIEW_LOGW("GPU load node not found: %{public}s", loadPath.c_str());
        sysGpuLoad.gpuLoad = 0;
        result.retCode = UcError::SUCCESS;
        return result;
    }
    
    std::string content;
    if (!FileUtil::LoadStringFromFile(loadPath, content)) {
        HIVIEW_LOGW("Failed to read GPU load node");
        sysGpuLoad.gpuLoad = 0;
        result.retCode = UcError::SUCCESS;
        return result;
    }
    
    // 解析格式: "负载@频率Hz" 或 "负载"
    size_t atPos = content.find('@');
    if (atPos != std::string::npos) {
        content = content.substr(0, atPos);
    }
    
    // 去除换行符
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
        content.pop_back();
    }
    
    std::stringstream ss(content);
    ss >> sysGpuLoad.gpuLoad;
    
    HIVIEW_LOGD("gpuLoad=%{public}f", sysGpuLoad.gpuLoad);
    result.retCode = UcError::SUCCESS;
    return result;
}

} // UCollectUtil
} // HiViewDFX
} // OHOS
