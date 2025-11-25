/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "light_hilog_catcher.h"

#include "file_util.h"
#include "freeze_json_util.h"
#include "hiview_logger.h"
#include "hilog_collector.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr int REPORT_HILOG_LINE = 100;
    constexpr uint32_t HILOG_LINE_NUM = 1000;
}
#ifdef HILOG_CATCHER_ENABLE
using namespace UCollect;
using namespace UCollectUtil;

DEFINE_LOG_LABEL(0xD002D11, "EventLogger-LightHilogCatcher");
LightHilogCatcher::LightHilogCatcher() : EventLogCatcher()
{
    name_ = "LightHilogCatcher";
}

bool LightHilogCatcher::Initialize(const std::string& strParam1, int intParam1, int intParam2)
{
    // this catcher do not need parameters, just return true
    description_ = "catcher cmd: hilog -z 1000 -P ";
    writeToJsFd = intParam1;
    pid_ = intParam2;
    return true;
}

int LightHilogCatcher::Catch(int fd, int jsonFd)
{
    if (pid_ <= 0) {
        return 0;
    }

    std::shared_ptr<HilogCollector> collector = HilogCollector::Create();
    CollectResult<std::string> result = collector->CollectLastLog(pid_, HILOG_LINE_NUM);
    if (result.retCode != UcError::SUCCESS) {
        HIVIEW_LOGE("Collect last log failed, retCode:%{public}d", result.retCode);
        return 0;
    }

    std::string lightHiLogStr = result.data;
    int originSize = GetFdSize(fd);
    if (lightHiLogStr.empty()) {
        HIVIEW_LOGE("light hilog is empty!");
        return 0;
    } else if (writeToJsFd && jsonFd >= 0) {
        std::list<std::string> lightHiLogList;
        std::stringstream lightHiLogStream(lightHiLogStr);
        std::string line;
        int count = 0;
        while (count++ < REPORT_HILOG_LINE && std::getline(lightHiLogStream, line)) {
            lightHiLogList.push_back(StringUtil::EscapeJsonStringValue(line));
        }
        FreezeJsonUtil::WriteKeyValue(jsonFd, "hilog", FreezeJsonUtil::GetStrByList(lightHiLogList));
    }

    FileUtil::SaveStringToFd(fd, lightHiLogStr);
    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}
#endif
} // namespace HiviewDFX
} // namespace OHOS