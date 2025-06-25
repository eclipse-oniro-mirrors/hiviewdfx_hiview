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

#include "data_publisher.h"

#include <cerrno>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "data_publisher_sys_event_callback.h"
#include "data_share_common.h"
#include "data_share_dao.h"
#include "data_share_store.h"
#include "data_share_util.h"
#include "event_publish.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_event_common.h"
#include "iquery_base_callback.h"
#include "json/json.h"
#include "hiview_logger.h"
#include "ret_code.h"
#include "string_util.h"
#include "sys_event_query.h"
#include "time_util.h"

using namespace OHOS::HiviewDFX::SubscribeStore;
using namespace OHOS::HiviewDFX::BaseEventSpace;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-DataPublisher");
namespace {
std::string GetBundleNameFromJsonStr(const std::string& jsonInfo)
{
    std::string bundleName = "";
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(jsonInfo, root)) {
        HIVIEW_LOGE("failed to parse jsonInfo.");
        return bundleName;
    }
    if (root[BUNDLE_NAME].isString()) {
        bundleName = root[BUNDLE_NAME].asString();
    }
    return bundleName;
}
}  // namespace

DataPublisher::DataPublisher()
{
    this->InitSubscriber();
}

int32_t DataPublisher::AddSubscriber(int32_t uid, const std::vector<std::string> &eventList)
{
    std::string events;
    std::shared_ptr<DataShareDao> dataShareDao = GetDataShareDao();
    auto ret = dataShareDao->GetEventListByUid(uid, events);
    if (ret != DB_SUCC) {
        HIVIEW_LOGE("query DB failed.");
        return ret;
    }
    std::vector<std::string> oldEventList;
    StringUtil::SplitStr(events, ";", oldEventList);
    for (auto &event : oldEventList) {
        eventRelationMap_[event].erase(uid);
    }
    for (auto &event : eventList) {
        eventRelationMap_[event].insert(uid);
    }
    auto newEvents = StringUtil::ConvertVectorToStr(eventList, ";");
    ret = dataShareDao->SaveSubscriberInfo(uid, newEvents);
    if (ret != DB_SUCC) {
        HIVIEW_LOGE("query DB failed.");
        return ret;
    }
    return IPC_CALL_SUCCEED;
}

int32_t DataPublisher::RemoveSubscriber(int32_t uid)
{
    std::string events;
    std::shared_ptr<DataShareDao> dataShareDao = GetDataShareDao();
    auto ret = dataShareDao->GetEventListByUid(uid, events);
    if (ret != DB_SUCC) {
        HIVIEW_LOGE("failed to get events by uid");
        return ERR_REMOVE_SUBSCRIBE;
    }
    if (events.empty()) {
        HIVIEW_LOGE("events list is empty");
        return ERR_REMOVE_SUBSCRIBE;
    }
    std::vector<std::string> eventList;
    StringUtil::SplitStr(events, ";", eventList);
    for (auto &event : eventList) {
        eventRelationMap_[event].erase(uid);
    }
    ret = dataShareDao->DeleteSubscriberInfo(uid);
    if (ret != DB_SUCC) {
        HIVIEW_LOGE("failed to delete subscriberInfo");
        return ERR_REMOVE_SUBSCRIBE;
    }
    return IPC_CALL_SUCCEED;
}

void DataPublisher::InitSubscriber()
{
    std::map<int, std::string> uidToEventsMap;
    std::shared_ptr<DataShareDao> dataShareDao = GetDataShareDao();
    int ret = dataShareDao->GetTotalSubscriberInfo(uidToEventsMap);
    if (ret != DB_SUCC) {
        HIVIEW_LOGE("failed to get total subscriberInfo");
        return;
    }
    for (auto it = uidToEventsMap.begin(); it != uidToEventsMap.end(); ++it) {
        int uid = it->first;
        std::string events = it->second;
        std::vector<std::string> eventList;
        StringUtil::SplitStr(events, ";", eventList);
        for (auto &event : eventList) {
            eventRelationMap_[event].insert(uid);
        }
    }
}

void DataPublisher::OnSysEvent(std::shared_ptr<OHOS::HiviewDFX::SysEvent> &event)
{
    HandleAppUninstallEvent(event);
    HandleAppStartEvent(event);
    if (eventRelationMap_.find(event->eventName_) == eventRelationMap_.end()) {
        return;
    }
    if (!CreateHiviewTempDir()) {
        HIVIEW_LOGE("failed to create resourceFile.");
        return;
    }
    int64_t timestamp = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    std::string timeStr = std::to_string(timestamp);
    std::string srcPath = TEMP_SRC_DIR;
    if (looper_ != nullptr) {
        auto task = std::bind(&DataPublisher::HandleSubscribeTask, this, event, srcPath, timeStr);
        looper_->AddTimerEvent(nullptr, nullptr, task, DELAY_TIME, false);
    } else {
        HIVIEW_LOGW("looper_ is null, call the subscribe function directly.");
        HandleSubscribeTask(event, srcPath, timeStr);
    }
}

void DataPublisher::HandleSubscribeTask(std::shared_ptr<OHOS::HiviewDFX::SysEvent> &event,
    std::string srcPath, std::string timeStr)
{
    std::string eventJson = event->AsJsonStr();
    if (!FileUtil::SaveStringToFile(srcPath, eventJson + ",", true)) {
        HIVIEW_LOGE("failed to persist eventJson to file.");
        return;
    }
    std::set<int> uidSet = eventRelationMap_[event->eventName_];
    std::string desPath;
    for (auto uid : uidSet) {
        desPath = OHOS::HiviewDFX::DataShareUtil::GetSandBoxPathByUid(uid);
        desPath.append("/")
            .append(event->domain_)
            .append("-")
            .append(timeStr)
            .append("-")
            .append(SUCCESS_CODE)
            .append(FILE_SUFFIX);
        auto res = OHOS::HiviewDFX::DataShareUtil::CopyFile(srcPath.c_str(), desPath.c_str());
        if (res == -1) {
            HIVIEW_LOGE("failed to move file to desPath.");
        }
        if (chmod(desPath.c_str(), FileUtil::FILE_PERM_666)) {
            HIVIEW_LOGE("Failed to chmod socket.");
        }
    }
}

void DataPublisher::HandleAppUninstallEvent(std::shared_ptr<OHOS::HiviewDFX::SysEvent> &event)
{
    if (event->eventName_ != BUNDLE_UNINSTALL) {
        return;
    }
    std::string jsonExtraInfo = event->AsJsonStr();
    std::string bundleName = GetBundleNameFromJsonStr(jsonExtraInfo);
    if (bundleName.empty()) {
        return;
    }
    int32_t uid = -1;
    std::shared_ptr<DataShareDao> dataShareDao = GetDataShareDao();
    auto ret = dataShareDao->GetUidByBundleName(bundleName, uid);
    if (ret != DB_SUCC) {
        HIVIEW_LOGE("failed to query from DB.");
        return;
    }
    ret = RemoveSubscriber(uid);
    if (ret != DB_SUCC) {
        HIVIEW_LOGE("failed to remove from DB.");
    }
}

void DataPublisher::HandleAppStartEvent(std::shared_ptr<OHOS::HiviewDFX::SysEvent> &event)
{
    if (event->eventName_ != "APP_START") {
        return;
    }
    std::string jsonExtraInfo = event->AsJsonStr();
    std::string bundleName = GetBundleNameFromJsonStr(jsonExtraInfo);
    if (bundleName.empty()) {
        HIVIEW_LOGW("bundleName empty.");
        return;
    }
    int32_t uid = OHOS::HiviewDFX::DataShareUtil::GetUidByBundleName(bundleName);
    EventPublish::GetInstance().PushEvent(uid, event->eventName_, HiSysEvent::EventType::BEHAVIOR, jsonExtraInfo);
}

void DataPublisher::SetWorkLoop(std::shared_ptr<EventLoop> looper)
{
    if (looper == nullptr) {
        HIVIEW_LOGW("SetWorkLoop failed, looper is null.");
        return;
    }
    looper_ = looper;
}

void DataPublisher::AddExportTask(std::shared_ptr<BaseEventQueryWrapper> queryWrapper, int64_t timestamp, int32_t uid)
{
    auto iter = uidTimeStampMap_.find(uid);
    if (iter != uidTimeStampMap_.end()) {
        iter->second = timestamp;
    } else {
        uidTimeStampMap_[uid] = timestamp;
    }
    if (!CreateHiviewTempDir()) {
        HIVIEW_LOGE("failed to create resourceFile.");
        return;
    }
    std::string timeStr = std::to_string(timestamp);
    std::string srcPath = TEMP_EXPORT_SRC_DIR;
    if (!FileUtil::RemoveFile(srcPath)) {
        HIVIEW_LOGE("failed to remove resourceFile.");
    }
    std::string desPath = OHOS::HiviewDFX::DataShareUtil::GetSandBoxPathByUid(uid);
    desPath.append(DOMAIN_PATH);
    desPath.append(timeStr);
    if (looper_ != nullptr) {
        auto task = [queryWrapper, srcPath, desPath] {
            OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback> exportCallback =
                new(std::nothrow) DataPublisherSysEventCallback(srcPath, desPath, 0, 0);
            int32_t queryResult = 0;
            queryWrapper->Query(exportCallback, queryResult);
        };
        looper_->AddTimerEvent(nullptr, nullptr, task, DELAY_TIME, false);
    } else {
        static OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback> exportCallback =
            new(std::nothrow) DataPublisherSysEventCallback(srcPath, desPath, 0, 0);
        int32_t queryResult = 0;
        queryWrapper->Query(exportCallback, queryResult);
    }
}

bool DataPublisher::CreateHiviewTempDir()
{
    if (!FileUtil::FileExists(PATH_DIR) && !FileUtil::ForceCreateDirectory(PATH_DIR)) {
        HIVIEW_LOGE("failed to create events dir.");
        return false;
    }
    return true;
}

int64_t DataPublisher::GetTimeStampByUid(int32_t uid)
{
    int64_t timeStamp;
    auto iter = uidTimeStampMap_.find(uid);
    if (iter != uidTimeStampMap_.end()) {
        timeStamp = iter->second;
    } else {
        timeStamp = 0;
    }
    return timeStamp;
}

std::shared_ptr<DataShareDao> DataPublisher::GetDataShareDao()
{
    std::shared_ptr<DataShareStore> dataShareStore = std::make_shared<DataShareStore>(DATABASE_DIR);
    return std::make_shared<DataShareDao>(dataShareStore);
}

}  // namespace HiviewDFX
}  // namespace OHOS