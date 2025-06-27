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

#include "export_db_storage.h"
#include "sys_event_sequence_mgr.h"
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
}

std::string EventExportUtil::GetDeviceId()
{
    static std::string deviceId = Parameter::GetUserType() == Parameter::USER_TYPE_OVERSEA_COMMERCIAL ?
        Parameter::GetString("persist.hiviewdfx.priv.packid", "") : GenerateDeviceId();
    return deviceId;
}

int64_t EventExportUtil::GetModuleExportStartSeq(std::shared_ptr<ExportDbManager> mgr,
    std::shared_ptr<ExportConfig> cfg)
{
    int64_t startSeq = EventStore::SysEventSequenceManager::GetInstance().GetStartSequence();
    HIVIEW_LOGI("start sequence is %{public}" PRId64 "", startSeq);
    if (mgr == nullptr || cfg == nullptr || !mgr->IsUnrecordedModule(cfg->moduleName) ||
        cfg->inheritedModule.empty()) {
        HIVIEW_LOGI("no need to get sequence from inherited module");
        return startSeq;
    }
    int64_t endSeq = mgr->GetExportEndSeq(cfg->inheritedModule);
    HIVIEW_LOGI("end sequence is %{public}" PRId64 "", endSeq);
    return (endSeq == INVALID_SEQ_VAL) ? startSeq : endSeq;
}
} // namespace HiviewDFX
} // namespace OHOS