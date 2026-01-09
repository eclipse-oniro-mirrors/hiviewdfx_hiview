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

#ifndef OHOS_HIVIEWDFX_DATA_PUBLISHER_H
#define OHOS_HIVIEWDFX_DATA_PUBLISHER_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "data_share_dao.h"
#include "event_loop.h"
#include "event_query_wrapper_builder.h"
#include "sys_event.h"
#include "sys_event_query.h"
#include "sys_event_query_wrapper.h"

namespace OHOS {
namespace HiviewDFX {

class BaseEventQueryWrapper;

class DataPublisher {
public:
    DataPublisher();
    ~DataPublisher() {};

public:
    int32_t AddSubscriber(int32_t uid, const std::vector<std::string> &events);
    int32_t RemoveSubscriber(int32_t uid);
    void OnSysEvent(std::shared_ptr<OHOS::HiviewDFX::SysEvent> &event);
    void AddExportTask(std::shared_ptr<BaseEventQueryWrapper> queryWrapper, int64_t timestamp, int32_t uid);
    int64_t GetTimeStampByUid(int32_t uid);
    void SetWorkLoop(std::shared_ptr<EventLoop> looper);

private:
    void InitSubscriber();
    bool CreateHiviewTempDir();
    void HandleAppUninstallEvent(std::shared_ptr<OHOS::HiviewDFX::SysEvent> &event);
    void HandleSubscribeTask(std::shared_ptr<OHOS::HiviewDFX::SysEvent> &event,
        std::string srcPath, std::string timeStr);
    std::shared_ptr<DataShareDao> GetDataShareDao();

private:
    std::map<std::string, std::set<int>> eventRelationMap_;
    std::map<int32_t, int64_t> uidTimeStampMap_;
    std::shared_ptr<EventLoop> looper_;
};
}  // namespace HiviewDFX
}  // namespace OHOS

#endif  // OHOS_HIVIEWDFX_DATA_PUBLISHER_H