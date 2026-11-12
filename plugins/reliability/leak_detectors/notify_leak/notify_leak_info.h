/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * Author: huangyu
 * Create: 2023-11-16
 */

#ifndef DFR_NOTIFY_LEAK_INFO_H
#define DFR_NOTIFY_LEAK_INFO_H

#include <string>
#include "fault_info_base.h"
#include "dfr_util.h"
#include "fault_dump_state_base.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

enum NotifyType {
    JS_LEAK = 0,
    LEAK_STACK,
    ARKWEB_LEAK,
    JSVM_LEAK,
    KMP_LEAK,
};

enum NotifyCompressStatus {
    NO_COMPRESS = 0,
    NEED_COMPRESS_AND_NOT_COMPRESSED,
    NEED_COMPRESS_AND_COMPRESSED,
};

class NotifyLeakInfo : public FaultInfoBase {
public:
    NotifyLeakInfo(pid_t pid, std::string name);
    ~NotifyLeakInfo();
    NotifyType GetNotifyType() const;
    void SetNotifyType(NotifyType notifyType);
    std::string GetNotifyDir() const;
    void SetNotifyDir(std::string notifyDir);
    std::string GetNotifyFileName() const;
    void SetNotifyFileName(std::string notifyDir);
    std::string GetBigMallocFileName() const;
    std::string GetLeakStack() const;
    void AddLeakStack(std::string stackLine);
    void SetIsReportEvent(bool flag);
    pid_t GetTid() const;
    void SetTid(pid_t tid);
    int GetLimitSize() const;
    void SetLimitSize(int limitSize);
    int GetLiveObjectSize() const;
    void SetLiveObjectSize(int liveObjectSize);
    std::string GetHeapdumpType() const;
    void SetHeapdumpType(std::string heapdumpType);
    std::string GetJsLeakFileName() const;
    std::string GetJsLeakFileZipName() const;
    std::string GetStaticJsLeakFileName() const;
    std::string GetStaticJsLeakFilePath() const;
    std::string GetStaticJsLeakNotifyFileName() const;
    void SetStaticJsLeakNotifyFileName(const std::string &fileName);
    bool HasStaticJsHeap() const;
    std::string GetArktsType() const;
    void SetArktsType(const std::string &arktsType);
    bool IsDynamicArkts() const;
    std::string GetArkWebLeakFileName() const;
    std::string GetArkWebLeakFilePath() const;
    std::string GetArkWebLeakFileZipName() const;
    std::string GetArkWebLeakFileZipPath() const;
    std::string GetJSVMLeakFileName() const;
    std::string GetJSVMLeakFilePath() const;
    std::string GetJSVMLeakFileZipName() const;
    std::string GetJSVMLeakFileZipPath() const;
    std::string GetKMPLeakFileName() const;
    std::string GetKMPLeakFilePath() const;
    std::string GetKMPLeakFileZipName() const;
    std::string GetKMPLeakFileZipPath() const;
    bool IsEverReported();
    void RecordReport();
    static void ResetHeapDumpCnt(int cnt);
    bool CouldTelemetry();
    bool CommonCompress();
    void ConsumeQuota();
    struct EventQuota GetEventQuota();
    std::string GetJsLeakFilePath() const;
    std::string GetJsLeakFileZipPath() const;
    std::string GetJsLeakNoTidFileName() const;
    std::string GetJsLeakNoTidFilePath() const;
    void SetCommpressStatus(NotifyCompressStatus compressStatus);
    NotifyCompressStatus GetCommpressStatus();
    bool ShouldPostEvent();
    bool ShouldSendHisysevent();
    void SetEventConfig(const std::string &eventConfig);
    std::string GetEventConfig();
    bool IsAppWantEvent();
    void SetOnlyAppreportFlag(bool flag);
    bool GetOnlyAppreportFlag();
    std::string GetProfilerPath() const override;
    std::string GetProfilerName() const override;
    uint64_t GetTelemetryType();
    uint64_t GetFaultType() const;
    std::string GetLogPathForSysEvent();

private:
    NotifyType notifyType_ { JS_LEAK };
    std::string notifyDir_ { "" };
    std::string notifyFileName_ { "" };
    std::string staticNotifyFileName_ { "" };
    std::string arktsType_ { "" };
    std::string leakStack_ { "" };
    pid_t tid_ { 0 };
    int limitSize_ { 0 };
    int liveObjectSize_ { 0 };
    std::string heapdumpType_ { "" };
    static ffrt::mutex rawheapListMtx_;
    static std::map<std::string, int64_t> RawheapDumpList_;
    static std::atomic<int> dumpCnt_;
    bool shouldCompress_ { false };
    bool hasCompress_ { false };
    NotifyCompressStatus compressStatus_ { NO_COMPRESS };
    std::string eventConfig_ { "" };
    bool onlyAppreportFlag_ { false };
    uint64_t telemetryType_ { 0 };
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NOTIFY_LEAK_INFO_H
