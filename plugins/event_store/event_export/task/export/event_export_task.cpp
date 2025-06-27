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

#include "event_export_task.h"

#include "event_export_util.h"
#include "event_json_parser.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "setting_observer_manager.h"
#include "sys_event_sequence_mgr.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");
using  ExportEventListParsers = std::map<std::string, std::shared_ptr<ExportEventListParser>>;
namespace {
constexpr int64_t BYTE_TO_MB = 1024 * 1024;

std::shared_ptr<ExportEventListParser> GetParser(ExportEventListParsers& parsers,
    const std::string& path)
{
    auto iter = parsers.find(path);
    if (iter == parsers.end()) {
        parsers.emplace(path, std::make_shared<ExportEventListParser>(path));
        return parsers[path];
    }
    return iter->second;
}

bool IsExportSwitchOff(std::shared_ptr<ExportConfig> config, std::shared_ptr<ExportDbManager> dbMgr)
{
    bool isSwitchOff = (SettingObserverManager::GetInstance()->GetStringValue(config->exportSwitchParam.name) !=
        config->exportSwitchParam.enabledVal);
    if (isSwitchOff) {
        HIVIEW_LOGI("export switch for module %{public}s is off", config->moduleName.c_str());
        int64_t enabledSeq = dbMgr->GetExportEnabledSeq(config->moduleName);
        // handle setting parameter listening error
        if (enabledSeq != INVALID_SEQ_VAL &&
            !FileUtil::FileExists(dbMgr->GetEventInheritFlagPath(config->moduleName))) {
            dbMgr->HandleExportSwitchChanged(config->moduleName, INVALID_SEQ_VAL);
        }
        return true;
    }
    HIVIEW_LOGI("export switch for module %{public}s is on", config->moduleName.c_str());
    int64_t enabledSeq = dbMgr->GetExportEnabledSeq(config->moduleName);
    if (enabledSeq == INVALID_SEQ_VAL) { // handle setting parameter listening error
        enabledSeq = EventExportUtil::GetModuleExportStartSeq(dbMgr, config);
        dbMgr->HandleExportSwitchChanged(config->moduleName, enabledSeq);
    }
    return false;
}
}

void EventExportTask::OnTaskRun()
{
    if (config_ == nullptr || dbMgr_ == nullptr) {
        HIVIEW_LOGE("config manager or db manager is invalid");
        return;
    }
    if (IsExportSwitchOff(config_, dbMgr_)) {
        return;
    }
    if (FileUtil::GetFolderSize(config_->exportDir) >= static_cast<uint64_t>(config_->maxCapcity * BYTE_TO_MB)) {
        HIVIEW_LOGE("event export directory is full");
        return;
    }
    // init handler request
    auto readReq = std::make_shared<EventReadRequest>();
    if (!InitReadRequest(readReq)) {
        HIVIEW_LOGE("failed to init read request");
        return;
    }
    // init write handler
    auto writeHandler = std::make_shared<EventWriteHandler>();
    // init read handler
    auto readHandler = std::make_shared<EventReadHandler>();
    readHandler->SetEventExportedListener([this] (int64_t beginSeq, int64_t endSeq) {
        HIVIEW_LOGW("finished exporting events in range [%{public}" PRId64 ", %{public}" PRId64 ")",
            beginSeq, endSeq);
        // sync export progress to db
        dbMgr_->HandleExportTaskFinished(config_->moduleName, endSeq);
    });
    // init handler chain
    readHandler->SetNextHandler(writeHandler);
    // start handler chain
    if (!readHandler->HandleRequest(readReq)) {
        HIVIEW_LOGE("failed to export all events in range [%{public}" PRId64 ",%{public}" PRId64 ")",
            readReq->beginSeq, readReq->endSeq);
        return;
    }
    // record export progress
    HIVIEW_LOGI("succeed to export all events in range [%{public}" PRId64 ",%{public}" PRId64 ")", readReq->beginSeq,
        readReq->endSeq);
}

bool EventExportTask::ParseExportEventList(ExportEventList& list) const
{
    if (config_->eventsConfigFiles.empty()) {
        // if export event list file isn't configured, use export info configured in hisysevent.def
        EventJsonParser::GetInstance()->GetAllCollectEvents(list);
        return true;
    }
    ExportEventListParsers parsers;
    auto iter = std::max_element(config_->eventsConfigFiles.begin(), config_->eventsConfigFiles.end(),
        [&parsers] (const std::string& path1, const std::string& path2) {
            auto parser1 = GetParser(parsers, path1);
            auto parser2 = GetParser(parsers, path2);
            return parser1->GetConfigurationVersion() < parser2->GetConfigurationVersion();
        });
    if (iter == config_->eventsConfigFiles.end()) {
        HIVIEW_LOGE("no event list file path is configured.");
        return false;
    }
    HIVIEW_LOGD("event list file path is %{public}s", (*iter).c_str());
    auto parser = GetParser(parsers, *iter);
    parser->GetExportEventList(list);
    return true;
}

bool EventExportTask::InitReadRequest(std::shared_ptr<EventReadRequest> readReq) const
{
    if (readReq == nullptr) {
        return false;
    }
    readReq->beginSeq = dbMgr_->GetExportBeginSeq(config_->moduleName);
    if (readReq->beginSeq == INVALID_SEQ_VAL) {
        HIVIEW_LOGE("invalid export: begin sequence:%{public}" PRId64 "", readReq->beginSeq);
        return false;
    }
    readReq->endSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    if (readReq->beginSeq >= readReq->endSeq) {
        HIVIEW_LOGE("invalid export range: [%{public}" PRId64 ",%{public}" PRId64 ")",
            readReq->beginSeq, readReq->endSeq);
        return false;
    }
    if (!ParseExportEventList(readReq->eventList) || readReq->eventList.empty()) {
        HIVIEW_LOGE("failed to get a valid event export list");
        return false;
    }
    readReq->moduleName = config_->moduleName;
    readReq->maxSize = config_->maxSize;
    readReq->exportDir = config_->exportDir;
    readReq->taskType = config_->taskType;
    return true;
}
} // HiviewDFX
} // OHOS
