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
DEFINE_LOG_TAG("UCollectUtil");
std::shared_ptr<GpuCollector> GpuCollector::Create()
{
    return std::make_shared<GpuDecorator>(std::make_shared<GpuCollectorImpl>());
}

CollectResult<GpuFreq> GpuCollectorImpl::CollectGpuFrequency()
{
    CollectResult<GpuFreq> result;
    GpuFreq& gpuFreq = result.data;
    gpuFreq.curFeq = CommonUtil::ReadNodeWithOnlyNumber(GPU_CUR_FREQ);
    gpuFreq.maxFeq = CommonUtil::ReadNodeWithOnlyNumber(GPU_MAX_FREQ);
    gpuFreq.minFeq = CommonUtil::ReadNodeWithOnlyNumber(GPU_MIN_FREQ);
    HIVIEW_LOGD("curFeq=%{public}d,maxFeq=%{public}d,minFeq=%{public}d",
        gpuFreq.curFeq, gpuFreq.maxFeq, gpuFreq.minFeq);
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<SysGpuLoad> GpuCollectorImpl::CollectSysGpuLoad()
{
    CollectResult<SysGpuLoad> result;
    SysGpuLoad& sysGpuLoad = result.data;
    sysGpuLoad.gpuLoad = CommonUtil::ReadNodeWithOnlyNumber(GPU_LOAD);
    HIVIEW_LOGD("gpuLoad=%{public}f", sysGpuLoad.gpuLoad);
    result.retCode = UcError::SUCCESS;
    return result;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
