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
#ifdef DMESG_CATCHER_ENABLE
class DmesgCatcher : public EventLogCatcher {
public:
    explicit DmesgCatcher();
    ~DmesgCatcher() override {};
    bool Initialize(const std::string& packageNam, int isWriteNewFile, int needWriteSysrq) override;
    int Catch(int fd, int jsonFd) override;
    bool Init(std::shared_ptr<SysEvent> event);
    void WriteNewSysrq();

private:
    bool isWriteNewFile_ = false;
    bool needWriteSysrq_ = false;
    std::shared_ptr<SysEvent> event_;

    bool DumpDmesgLog(int fd);
    bool WriteSysrq();
    bool DumpSysrqToFile(int fd, char *buffer, int size);
};
#endif // DMESG_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_DMESG_CATCHER
