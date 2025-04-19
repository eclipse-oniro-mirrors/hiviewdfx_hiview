/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#ifndef FAULTLOG_DATABASE_H
#define FAULTLOG_DATABASE_H

#include "event_loop.h"
#include "i_faultlog_database.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogDatabase : public IFaultLogDatabase {
public:
    explicit FaultLogDatabase(const std::shared_ptr<EventLoop>& eventLoop);
    void SaveFaultLogInfo(FaultLogInfo& info) override;
    std::list<FaultLogInfo> GetFaultInfoList(
        const std::string& module, int32_t id, int32_t faultType, int32_t maxNum) override;
    bool IsFaultExist(int32_t pid, int32_t uid, int32_t faultType) override;
private:
    std::shared_ptr<EventLoop> eventLoop_;
    std::mutex mutex_;
    bool ParseFaultLogInfoFromJson(std::shared_ptr<EventRaw::RawData> rawData, FaultLogInfo& info);
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif
