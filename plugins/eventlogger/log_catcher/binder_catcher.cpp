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
#include "binder_catcher.h"

#include <fstream>

#include "securec.h"

#include "common_utils.h"
#include "defines.h"
namespace OHOS {
namespace HiviewDFX {
#ifdef BINDER_CATCHER_ENABLE
using namespace OHOS::HiviewDFX::CommonUtils;
BinderCatcher::BinderCatcher() : EventLogCatcher()
{
    name_ = "BinderCatcher";
}

bool BinderCatcher::Initialize(const std::string& strParam1, int intParam1, int intParam2)
{
    // this catcher do not need parameters, just return true
    char buf[BUF_SIZE_512] = {0};
    int ret = snprintf_s(buf, BUF_SIZE_512, BUF_SIZE_512 - 1,
        "BinderCatcher --\n");
    if (ret > 0) {
        description_ = buf;
    }
    return true;
};

int BinderCatcher::Catch(int fd, int jsonFd)
{
    std::string line;
    int originSize = GetFdSize(fd);
    std::ifstream fin;
    fin.open("/proc/transaction_proc");
    if (!fin.is_open()) {
        std::string content = "open binder file failed :/proc/transaction_proc\r\n";
        FileUtil::SaveStringToFd(fd, content);
        goto end;
    }
    while (getline(fin, line)) {
        FileUtil::SaveStringToFd(fd, line + "\n");
    }
    fin.close();

    end:
    logSize_ = GetFdSize(fd) - originSize;
    if (logSize_ <= 0) {
        FileUtil::SaveStringToFd(fd, "binder content is empty!");
    }
    return logSize_;
};
#endif // BINDER_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
