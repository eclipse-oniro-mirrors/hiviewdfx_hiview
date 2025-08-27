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
    bool Initialize(const std::string& packageNam, int writeNewFile, int writeType) override;
    int Catch(int fd, int jsonFd) override;
    bool Init(std::shared_ptr<SysEvent> event);
    void WriteNewFile(int pid);

    enum WRITE_TYPE {
        DMESG,
        SYS_RQ,
        HUNG_TASK,
        SYSRQ_HUNGTASK
    };
private:
    int writeNewFile_ = 0;
    int writeType_ = 0;
    std::shared_ptr<SysEvent> event_;

    bool DumpDmesgLog(int fdOne, int fdTwo);
    bool WriteSysrqTrigger();
    bool DumpToFile(int fdOne, int fdTwo, const std::string& dataStr);
    void GetSysrq(const std::string& dataStr, std::string& sysrqStr);
    void GetHungTask(const std::string& dataStr, std::string& hungTaskStr);
    FILE* GeFileInfoByName(const std::string& fileName, int& fd);
    void CloseFp(FILE*& fp);
#ifdef KERNELSTACK_CATCHER_ENABLE
    void GetTidsByPid(int pid, std::vector<pid_t>& tids);
    int DumpKernelStacktrace(int fd, int pid);
#endif
};
#endif // DMESG_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_DMESG_CATCHER
