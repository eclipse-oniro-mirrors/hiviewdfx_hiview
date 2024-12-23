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

#include "process_collector_impl.h"

#include <dlfcn.h>

#include "common_util.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "process_decorator.h"

using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("UCollectUtil-ProcessCollector");
const std::string LIB_NAME = "libucollection_utility_ex.z.so";
const std::string GET_MEM_CG_PROCESSES_FUNC_NAME = "GetMemCgProcesses";
const std::string PROCESS_COLLECTOT_DIR = "/data/log/hiview/unified_collection/process/";
constexpr int32_t MAX_FILE_NUM = 10;
const std::string PREFIX = "memcg_process_";
const std::string SUFFIX = ".txt";
}

ProcessCollectorImpl::ProcessCollectorImpl()
{
    handle_ = dlopen(LIB_NAME.c_str(), RTLD_LAZY);
    if (handle_ == nullptr) {
        HIVIEW_LOGW("dlopen failed, error: %{public}s", dlerror());
    }
}

ProcessCollectorImpl::~ProcessCollectorImpl()
{
    if (handle_ != nullptr) {
        dlclose(handle_);
    }
}

std::shared_ptr<ProcessCollector> ProcessCollector::Create()
{
    static std::shared_ptr<ProcessCollector> instance_ =
        std::make_shared<ProcessDecorator>(std::make_shared<ProcessCollectorImpl>());
    return instance_;
}

CollectResult<std::unordered_set<int32_t>> ProcessCollectorImpl::GetMemCgProcesses()
{
    CollectResult<std::unordered_set<int32_t>> result;
    if (handle_ == nullptr) {
        return result;
    }

    using GetMemCgProcesses = bool (*)(std::unordered_set<int32_t>&);
    GetMemCgProcesses getMemCgProcesses =
        reinterpret_cast<GetMemCgProcesses>(dlsym(handle_, GET_MEM_CG_PROCESSES_FUNC_NAME.c_str()));
    if (!getMemCgProcesses) {
        HIVIEW_LOGW("dlsym failed, %{public}s.", dlerror());
        return result;
    }

    std::unordered_set<int32_t> memCgProcs;
    if (!getMemCgProcesses(memCgProcs)) {
        result.retCode = UcError::READ_FAILED;
        return result;
    }
    result.data = memCgProcs;
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<bool> ProcessCollectorImpl::IsMemCgProcess(int32_t pid)
{
    CollectResult<bool> result;
    auto memCgProcsResult = GetMemCgProcesses();
    if (memCgProcsResult.retCode != UcError::SUCCESS) {
        result.retCode = memCgProcsResult.retCode;
        return result;
    }

    auto memCgProcs = memCgProcsResult.data;
    result.data = memCgProcs.find(pid) != memCgProcs.end();
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::string> ProcessCollectorImpl::ExportMemCgProcesses()
{
    CollectResult<std::string> result;
    auto memCgProcsResult = GetMemCgProcesses();
    if (memCgProcsResult.retCode != UcError::SUCCESS) {
        result.retCode = memCgProcsResult.retCode;
        return result;
    }

    std::string filePath;
    {
        std::unique_lock<std::mutex> lock(fileMutex_);
        filePath = CommonUtil::CreateExportFile(PROCESS_COLLECTOT_DIR, MAX_FILE_NUM, PREFIX, SUFFIX);
    }

    FILE* fp = fopen(filePath.c_str(), "w");
    if (fp == nullptr) {
        HIVIEW_LOGW("open %{public}s failed.",  FileUtil::ExtractFileName(filePath).c_str());
        result.retCode = UcError::WRITE_FAILED;
        return result;
    }
    for (const auto& proc : memCgProcsResult.data) {
        (void)fprintf(fp, "%d\n", proc);
    }
    (void)fclose(fp);

    result.data = filePath;
    result.retCode = UcError::SUCCESS;
    return result;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
