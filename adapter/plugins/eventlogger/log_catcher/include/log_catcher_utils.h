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

#ifdef HITRACE_CATCHER_ENABLE
#include <map>
#include <vector>
#endif

namespace OHOS {
namespace HiviewDFX {
namespace LogCatcherUtils {
static constexpr int WAIT_CHILD_PROCESS_COUNT = 200;

int DumpStacktrace(int fd, int pid, std::string& terminalBinderStack, int terminalBinderPid = 0,
    int terminalBinderTid = 0);
void GetThreadStack(const std::string& processStack, std::string& stack, int tid);
int DumpStackFfrt(int fd, const std::string& pid);
int WriteKernelStackToFd(int originFd, const std::string& msg, int pid);
void ReadShellToFile(int fd, const std::string& serviceName, const std::string& cmd, int& count);
#ifdef HITRACE_CATCHER_ENABLE
void HandleTelemetryMsg(std::map<std::string, std::string>& valuePairs);
void FreezeFilterTraceOn(const std::string& bundleName);
std::pair<std::string, std::vector<std::string>> FreezeDumpTrace(uint64_t hitraceTime, bool grayscale,
    const std::string& bundleName);
std::pair<std::string, std::string> GetTelemetryInfo();
#endif
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENTLOGGER_STACK_UTILS_H