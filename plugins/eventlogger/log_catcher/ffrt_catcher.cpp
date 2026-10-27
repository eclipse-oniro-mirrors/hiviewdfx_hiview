/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "ffrt_catcher.h"

#include "file_util.h"
#include "log_catcher_utils.h"

namespace OHOS {
namespace HiviewDFX {
FfrtCatcher::FfrtCatcher() : EventLogCatcher()
{
    name_ = "FfrtCatcher";
}

bool FfrtCatcher::Initialize(const std::string& strParam1, int intParam1, int intParam2)
{
    // this catcher do not need parameters, just return true
    description_ = "FfrtCatcher --\n";
    pid_ = intParam1;
    return true;
};

int FfrtCatcher::Catch(int fd, int jsonFd)
{
    int originSize = GetFdSize(fd);
    if (originSize < 0) {
        return 0;
    }
    FileUtil::SaveStringToFd(fd, "ffrt fump pid=" + std::to_string(pid_) + ":\n");
    LogCatcherUtils::DumpStackFfrt(fd, std::to_string(pid_));

    logSize_ = GetFdSize(fd) - originSize;
    if (logSize_ <= 0) {
        FileUtil::SaveStringToFd(fd, "ffrt dump content is empty!");
    }

    return logSize_;
};
} // namespace HiviewDFX
} // namespace OHOS