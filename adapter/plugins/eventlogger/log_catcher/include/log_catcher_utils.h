/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef EVENTLOGGER_LOG_CATCHER_UTILS_H
#define EVENTLOGGER_LOG_CATCHER_UTILS_H
#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace LogCatcherUtils {
enum FFRT_TYPE {TOP, APP, SYS};
static constexpr int WAIT_CHILD_PROCESS_COUNT = 300;

int DumpStacktrace(int fd, int pid);
int WriteKernelStackToFd(int originFd, const std::string& msg, int pid);
FFRT_TYPE GetFfrtDumpType(int pid);
void ReadShellToFile(int fd, const std::string& serviceName, const std::string& cmd, int& count);
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENTLOGGER_STACK_UTILS_H