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

#include "wm_collector_impl.h"

#include <mutex>

#include <fcntl.h>
#include <unistd.h>

#include "common_util.h"
#include "common_utils.h"
#include "hiview_logger.h"
#include "string_ex.h"
#include "wm_decorator.h"

using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("UCollectUtil-WmCollector");
constexpr int32_t MAX_FILE_NUM = 10;
const std::string COLLECTION_WM_PATH = "/data/log/hiview/unified_collection/wm/";
const std::string GPU_MEMORY_PATH = "/proc/gpu_memory";
std::mutex g_memMutex;

std::string CreateExportFileName(const std::string& filePrefix, const std::string& ext)
{
    std::unique_lock<std::mutex> lock(g_memMutex);
    return CommonUtil::CreateExportFile(COLLECTION_WM_PATH, MAX_FILE_NUM, filePrefix, ext);
}
}

std::shared_ptr<WmCollector> WmCollector::Create()
{
    return std::make_shared<WmDecorator>(std::make_shared<WmCollectorImpl>());
}

CollectResult<std::string> WmCollectorImpl::ExportWindowsInfo()
{
    CollectResult<std::string> result;
    result.retCode = UcError::UNSUPPORT;
    std::vector<std::string> args;
    args.push_back("hidumper");
    args.push_back("-s");
    args.push_back("WindowManagerService");
    args.push_back("-a");
    args.push_back("-a");
    std::string fileName = CreateExportFileName("windows_info_", ".txt");
    if (fileName.empty()) {
        return result;
    }
    int fd = open(fileName.c_str(), O_WRONLY);
    if (fd < 0) {
        HIVIEW_LOGE("create fileName=%{public}s failed.", fileName.c_str());
        return result;
    }
    if (CommonUtils::WriteCommandResultToFile(fd, "/system/bin/hidumper", args) == -1) {
        HIVIEW_LOGE("write cmd to file=%{public}s failed.", fileName.c_str());
        close(fd);
        return result;
    }
    close(fd);
    result.retCode = UcError::SUCCESS;
    result.data = fileName;
    return result;
}

CollectResult<std::string> WmCollectorImpl::ExportWindowsMemory()
{
    CollectResult<std::string> result;
    result.retCode = UcError::UNSUPPORT;
    std::vector<std::string> args;
    args.push_back("hidumper");
    args.push_back("-s");
    args.push_back("RenderService");
    args.push_back("-a");
    args.push_back("dumpMem");
    std::string fileName = CreateExportFileName("windows_memory_", ".txt");
    if (fileName.empty()) {
        return result;
    }
    int fd = open(fileName.c_str(), O_WRONLY);
    if (fd < 0) {
        HIVIEW_LOGE("create fileName=%{public}s failed.", fileName.c_str());
        return result;
    }
    if (CommonUtils::WriteCommandResultToFile(fd, "/system/bin/hidumper", args) == -1) {
        HIVIEW_LOGE("write cmd to file=%{public}s failed.", fileName.c_str());
        close(fd);
        return result;
    }
    close(fd);
    result.retCode = UcError::SUCCESS;
    result.data = fileName;
    return result;
}

CollectResult<std::string> WmCollectorImpl::ExportGpuMemory()
{
    CollectResult<std::string> result;
    std::string content;
    bool ret = FileUtil::LoadStringFromFile(GPU_MEMORY_PATH, content);
    if (!ret) {
        return result;
    }
    if (content.empty()) {
        result.retCode = READ_FAILED;
        return result;
    }
    std::string fileName = CreateExportFileName("gpu_memory_", ".txt");
    if (fileName.empty()) {
        result.retCode = SYSTEM_ERROR;
        return result;
    }
    bool isSuccess = FileUtil::SaveStringToFile(fileName, content, true);
    if (!isSuccess) {
        result.retCode = WRITE_FAILED;
        return result;
    }
    result.retCode = UcError::SUCCESS;
    result.data = fileName;
    return result;
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS