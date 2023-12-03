/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef EVENT_LOGGER_DMESG_CATCHER
#define EVENT_LOGGER_DMESG_CATCHER

#include <map>
#include <string>
#include <vector>

#include "sys_event.h"

#include "event_log_catcher.h"
namespace OHOS {
namespace HiviewDFX {
class DmesgCatcher : public EventLogCatcher {
public:
    explicit DmesgCatcher();
    ~DmesgCatcher() override {};
    bool Initialize(const std::string& packageNam, int isWriteNewFile, int intParam2) override;
    int Catch(int fd, int jsonFd) override;
    bool Init(std::shared_ptr<SysEvent> event);

private:
    static const inline std::string FULL_DIR = "/data/log/eventlog/";
    bool needWriteSysrq_ = false;
    bool isWriteNewFile_ = false;
    std::shared_ptr<SysEvent> event_;

    std::string DmesgSaveTofile();
    bool DumpDmesgLog(int fd);
    bool WriteSysrq();
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_DMESG_CATCHER
