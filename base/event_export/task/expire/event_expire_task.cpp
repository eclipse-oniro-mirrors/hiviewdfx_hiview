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

#include "event_expire_task.h"

#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExpireTask");
namespace {
constexpr char SYSEVENT_EXPORT_TMP_DIR[] = "tmp";
constexpr char SYSEVENT_EXPORT_DIR[] = "sys_event_export";

std::string GetExpireFileScanDir(const std::string moduleName)
{
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        return "";
    }
    std::string workDir = context->GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY);
    workDir = FileUtil::IncludeTrailingPathDelimiter(workDir.append(SYSEVENT_EXPORT_DIR));
    workDir = FileUtil::IncludeTrailingPathDelimiter(workDir.append(SYSEVENT_EXPORT_TMP_DIR));
    auto scanDir = FileUtil::IncludeTrailingPathDelimiter(workDir.append(moduleName));
    if (!FileUtil::IsDirectory(scanDir) && !FileUtil::ForceCreateDirectory(scanDir)) {
        return "";
    }
    HIVIEW_LOGD("scan directory is %{public}s", scanDir.c_str());
    return scanDir;
}
}
void EventExpireTask::OnTaskRun()
{
    if (config_ == nullptr || dbMgr_ == nullptr) {
        HIVIEW_LOGE("config manager or db manager is invalid");
        return;
    }
    // init delete handler
    auto deleteHandler = std::make_shared<EventDeleteHandler>();
    // init scan handler
    auto scanHandler = std::make_shared<EventScanHandler>();
    // init handler chain
    scanHandler->SetNextHandler(deleteHandler);
    // start handler chain, try to scan expired event file
    auto scanReq = std::make_shared<EventScanRequest>();
    scanReq->moduleName = config_->moduleName;
    scanReq->scanDir = GetExpireFileScanDir(config_->moduleName);
    scanReq->storedDayCnt = static_cast<uint8_t>(config_->dayCnt);
    if (!scanHandler->HandleRequest(scanReq)) {
        HIVIEW_LOGI("failed to run expire task");
    }
}
} // HiviewDFX
} // OHOS