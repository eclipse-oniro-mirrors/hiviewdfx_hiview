/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "event_delete_handler.h"

#include <vector>

#include "base_def.h"
#include "def.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");

bool EventDeleteHandler::HandleRequest(RequestPtr req)
{
    auto delReq = BaseRequest::DownCastTo<EventDelRequest>(req);
    return Delete(delReq->files, delReq->moduleName);
}

bool EventDeleteHandler::Delete(std::vector<std::string>& files, const std::string& moduleName)
{
    for (const auto& file : files) {
        // delete expired event file
        HIVIEW_LOGI("%{public}s has been deleted", StringUtil::HideDeviceIdInfo(file).c_str());
        FileUtil::RemoveFile(file);
    }
    return true;
}
} // HiviewDFX
} // OHOS