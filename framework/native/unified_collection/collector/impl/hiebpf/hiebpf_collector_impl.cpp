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


#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hiebpf_collector_impl.h"
#include "hiebpf_decorator.h"
#include "hiview_logger.h"

using namespace OHOS::HiviewDFX::UCollect;
namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil-HiebpfCollectorImpl");

std::shared_ptr<HiebpfCollector> HiebpfCollector::Create()
{
    return std::make_shared<HiebpfDecorator>(std::make_shared<HiebpfCollectorImpl>());
}

CollectResult<bool> HiebpfCollectorImpl::StartHiebpf(int duration,
    const std::string processName,
    const std::string outFile)
{
    CollectResult<bool> result;
    int childPid = fork();
    if (childPid < 0) {
        result.data = false;
        result.retCode = UCollect::UcError::UNSUPPORT;
        return result;
    } else if (childPid == 0) {
        std::string timestr = std::to_string(duration);
        execl("/system/bin/hmpsf", "hmpsf", "--events", "thread", "--duration", timestr.c_str(),
            "--target_proc_name", processName.c_str(), "--start", "true", "--outprase_file", outFile.c_str(), NULL);
    } else {
        result.retCode = UCollect::UcError::SUCCESS;
        if (waitpid(childPid, nullptr, 0) != childPid) {
            HIVIEW_LOGE("waitpid fail, pid: = %{public}d, errno = %{public}d", childPid, errno);
        } else {
            HIVIEW_LOGE("waitpid success, pid: = %{public}d", childPid);
        }
    }
    return result;
}

CollectResult<bool> HiebpfCollectorImpl::StopHiebpf()
{
    CollectResult<bool> result;
    int childPid = fork();
    if (childPid < 0) {
        result.data = false;
        result.retCode = UCollect::UcError::UNSUPPORT;
        return result;
    } else if (childPid == 0) {
        execl("/system/bin/hmpsf", "hmpsf", "--stop", "true", NULL);
    } else {
        result.retCode = UCollect::UcError::SUCCESS;
        if (waitpid(childPid, nullptr, 0) != childPid) {
            HIVIEW_LOGE("waitpid fail, pid: = %{public}d, errno = %{public}d", childPid, errno);
        } else {
            HIVIEW_LOGE("waitpid success, pid: = %{public}d", childPid);
        }
    }
    return result;
}
} // UCollectUtil
} // HivewDFX
} // OHOS
