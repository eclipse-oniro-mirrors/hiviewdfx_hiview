/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <unistd.h>
#include "faultlogger_ffi.h"
#include "faultlogger_client.h"
#include "faultlogger_info.h"
#include "hiview_logger.h"
#include <mutex>
#include "faultlog_query_result.h"
#include "faultlog_info.h"
#include "common_utils.h"

using namespace OHOS::HiviewDFX;

DEFINE_LOG_LABEL(0xD002D11, "Faultlogger-cj");
std::mutex g_mutex;

char* MallocCString(const std::string& origin)
{
    if (origin.empty()) {
        return nullptr;
    }
    auto length = origin.length() + 1;
    char* res =  static_cast<char*>(malloc(sizeof(char) * length));
    if (res == nullptr) {
        return nullptr;
    }
    return std::char_traits<char>::copy(res, origin.c_str(), length);
}

CJFaultLogInfo ConvertInform(std::unique_ptr<FaultLogInfo> faultLogInfo)
{
    CJFaultLogInfo ret = {
        .pid = faultLogInfo->GetProcessId(),
        .uid = faultLogInfo->GetId(),
        .type = faultLogInfo->GetFaultType(),
        .ts = faultLogInfo->GetTimeStamp(),
        .reason = faultLogInfo->GetFaultReason(),
        .module = faultLogInfo->GetModuleName(),
        .summary = faultLogInfo->GetFaultSummary(),
    };

    int fd = faultLogInfo->GetRawFileDescriptor();
    if (fd < 0) {
        HIVIEW_LOGE("pid %{public}d Fail to get fd:%{public}d\n", faultLogInfo->GetProcessId(), fd);
        ret.fullLog = "Fail to get log, fd is " + std::to_string(fd);
        return ret;
    }
    while (fd >= 0) {
        char buf[BUF_SIZE_512] = {0};
        int nread = TEMP_FAILURE_RETRY(read((fd), buf, BUF_SIZE_512 - 1));
        if (nread == -1) {
            if (errno == EAGAIN) {
                continue;
            } else {
                break;
            }
        } else if (nread == 0) {
            break;
        }
        ret.fullLog += buf;
    }
    return ret;
}

void FaultLogExecute(std::unique_ptr<CFaultLogInfoContext>& faultLogInfoContext)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    const int maxQueryCount = 10;
    int currentCount = 0;
    auto faultLogResult = QuerySelfFaultLog((FaultLogType)faultLogInfoContext->faultType,
        maxQueryCount);
    if (faultLogResult == nullptr) {
        faultLogInfoContext->resolved = true;
        return;
    }

    while (faultLogResult->HasNext()) {
        if (currentCount >= maxQueryCount) {
            break;
        }
        auto faultLogInfo = faultLogResult->Next();
        if (faultLogInfo == nullptr) {
            break;
        }
        currentCount++;
        faultLogInfoContext->infoVector.push_back(ConvertInform(std::move(faultLogInfo)));
    }
    faultLogInfoContext->resolved = true;
}

extern "C" {
    CArrFaultLogInfo FfiFaultLoggerQuery(int32_t faultType, int32_t &code)
    {
        CArrFaultLogInfo faultInfos = {.head = nullptr, .size = 0};
        if (!CheckFaultloggerStatus()) {
            code = ERR_SERVICE_STATUS;
            return faultInfos;
        }

        auto faultLogInfoContext = std::make_unique<CFaultLogInfoContext>();

        faultLogInfoContext->faultType = faultType;
        FaultLogExecute(faultLogInfoContext);
        faultInfos.size = static_cast<int64_t>(faultLogInfoContext->infoVector.size());
        if (faultInfos.size == 0) {
            return faultInfos;
        }
        if (faultLogInfoContext->resolved) {
            CFaultLogInfo *retValue = static_cast<CFaultLogInfo *>(malloc(sizeof(CFaultLogInfo) * faultInfos.size));
            if (retValue == nullptr) {
                return faultInfos;
            }
            int i = 0;
            for (auto& infoItem : faultLogInfoContext->infoVector) {
                retValue[i].pid = infoItem.pid;
                retValue[i].uid = infoItem.uid;
                retValue[i].faultLogType = infoItem.type;
                retValue[i].timestamp = infoItem.ts;
                retValue[i].reason = MallocCString(infoItem.reason);
                retValue[i].module = MallocCString(infoItem.module);
                retValue[i].summary = MallocCString(infoItem.summary);
                retValue[i].fullLog = MallocCString(infoItem.fullLog);
                ++i;
                HIVIEW_LOGI("add element when resovled pid = %{public}d, uid = %{public}d, ts = %{public}" PRId64,
                infoItem.pid, infoItem.uid, infoItem.ts);
            }
            faultInfos.head = retValue;
        } else {
            HIVIEW_LOGE("get signal info list failed");
        }
        return faultInfos;
    }
}