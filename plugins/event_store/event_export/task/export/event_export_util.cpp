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

#include "event_export_util.h"

#include "event.h"
#include "export_db_storage.h"
#include "file_util.h"
#include "sys_event_sequence_mgr.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "parameter.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-EventExportUtil");

std::string GenerateDeviceId()
{
    constexpr int32_t deviceIdLength = 65;
    char id[deviceIdLength] = {0};
    if (GetDevUdid(id, deviceIdLength) == 0) {
        return std::string(id);
    }
    return "";
}

bool IsExportDirEmpty(const std::string& exportDir)
{
    std::vector<std::string> eventZipFiles;
    FileUtil::GetDirFiles(exportDir, eventZipFiles);
    return !any_of(eventZipFiles.begin(), eventZipFiles.end(), [] (const std::string& file) {
        return !FileUtil::IsDirectory(file);
    });
}

void PostExportEvent(const std::string& moduleName, int16_t taskType)
{
    auto event = std::make_shared<Event>("post_export_type_event");
    event->messageType_ = Event::MessageType::EVENT_EXPORT_TYPE;
    event->SetValue("reportModule", moduleName);
    if (taskType == ALL_EVENT_TASK_TYPE) {
        event->SetValue("reportInterval", "0");
    } else {
        event->SetValue("reportInterval", std::to_string(taskType));
    }

    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        HIVIEW_LOGW("hiview context is invalid.");
        return;
    }
    context->PostUnorderedEvent(event);
}

bool IsNeedPostEvent(std::shared_ptr<ExportConfig> config)
{
    if (config == nullptr) {
        HIVIEW_LOGW("export cfg file is invalid.");
        return false;
    }
    if (!config->needPostEvent) {
        HIVIEW_LOGW("no need to post event");
        return false;
    }
    if (IsExportDirEmpty(config->exportDir)) {
        HIVIEW_LOGW("no event zip file found");
        return false;
    }
    return true;
}
}

std::string EventExportUtil::GetDeviceId()
{
    static std::string deviceId = Parameter::GetUserType() == Parameter::USER_TYPE_OVERSEA_COMMERCIAL ?
        Parameter::GetString("persist.hiviewdfx.priv.packid", "") : GenerateDeviceId();
    return deviceId;
}

bool EventExportUtil::CheckAndPostExportEvent(std::shared_ptr<ExportConfig> config)
{
    if (!IsNeedPostEvent(config)) {
        return false;
    }
    PostExportEvent(config->moduleName, config->taskType);
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS