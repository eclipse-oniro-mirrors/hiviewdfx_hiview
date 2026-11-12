/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cred_event_handler.h"

#include <fcntl.h>
#include <fstream>
#include <mutex>
#include <regex>
#include <string>
#include <unistd.h>
#include "fault_info_base.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "hitrace_meter.h"
#include "dfr_util.h"
#include "old_log_cleaner.h"

namespace OHOS {
namespace HiviewDFX {

using namespace std;
DEFINE_LOG_LABEL(0xD002DA0, "CredEventHandler");

namespace {
    constexpr int BASE_INDEX = 4;
    constexpr int BASE_PERCENTAGE = 100;
    constexpr int BASE_NUMBER = 2;
}

atomic<time_t> CredLeakInfo::lastLogStamp_ { 0 };

CredLeakInfo::CredLeakInfo(const string &eventName) : LeakEventInfoBase(eventName)
{
}

bool CredLeakInfo::IsEverReported()
{
    time_t lastLogStamp = GetLastReportRecord();
    time_t now = time(nullptr);
    if (now - lastLogStamp > SEC_PER_HOUR) {
        HIVIEW_LOGI("cred has reported cred at %{public}s",
            DfrUtil::GetInstance().TimeStampToStr(lastLogStamp).c_str());
    }
    return now <= lastLogStamp + static_cast<time_t>(SEC_PER_DAY);
}

void CredLeakInfo::RecordReport()
{
    lastLogStamp_.store(time(nullptr));
}

time_t CredLeakInfo::GetLastReportRecord()
{
    return lastLogStamp_.load();
}

bool CredLeakInfo::PrintMemList(std::ofstream &fout)
{
    auto memOrderList = ConvertStringToIntArray(memOrderList_);
    if (memOrderList.empty()) {
        HIVIEW_LOGE("memOrderList is empty!");
        return false;
    }
    fout << "memory allocation has reached the warning threshold:" << std::endl;
    fout << "===Radix1var Allocator Statistics ===" << std::endl;
    fout << std::setw(SHORT_WEIGHT) << std::left << "Total :" << memTotal_ << " Bytes" << std::endl;
    fout << std::setw(SHORT_WEIGHT) << std::left << "Used  :" << memUsed_ << " Bytes" << std::endl;
    if (memTotal_ != 0) {
        fout << std::setw(SHORT_WEIGHT) << std::left << "PCT   :";
        fout << std::floor(static_cast<double>(memUsed_) / memTotal_ * BASE_PERCENTAGE) << "%" << std::endl;
    }
    fout << std::setw(LONG_WEIGHT) << std::left << "Order";
    fout << std::setw(LONG_WEIGHT) << std::left << "Size(B)";
    fout << std::setw(LONG_WEIGHT) << std::left << "Count";
    fout << std::setw(LONG_WEIGHT) << std::left << "Total Size(B)" << std::endl;
    for (unsigned long index = 0; index < memOrderList.size(); index++) {
        fout << std::setw(LONG_WEIGHT) << std::left << index;
        fout << std::setw(LONG_WEIGHT) << std::left << (1 << (BASE_INDEX + index));
        fout << std::setw(LONG_WEIGHT) << std::left << memOrderList[index];
        uint64_t totalSize = static_cast<uint64_t>(1) << (BASE_INDEX + index);
        totalSize *= static_cast<uint64_t>(memOrderList[index]);
        fout << std::setw(LONG_WEIGHT) << std::left << std::to_string(totalSize);
        fout << std::endl;
    }
    fout << "current process: " << processCount_ << ", threads: " << threadsCount_;
    fout << std::endl;
    return true;
}

std::vector<int> CredLeakInfo::ConvertStringToIntArray(std::string memOrderListStr)
{
    std::vector<int> numbers;
    std::regex pattern(R"(^\[\d+(,\d+)*\]$)");
    if (!std::regex_match(memOrderListStr, pattern)) {
        return numbers;
    }
    std::string trimmedStr = memOrderListStr.substr(1, memOrderListStr.size() - 2); // 移除两端的方括号
    std::stringstream ss(trimmedStr);
    std::string numberStr;

    while (getline(ss, numberStr, ',')) {
        int num = 0;
        if (!DfrUtil::GetInstance().SafeStoi(numberStr, num)) {
            HIVIEW_LOGW("trans to int value failed: %{public}s", numberStr.c_str());
            continue;
        }
        numbers.push_back(num);
    }
    return numbers;
}

void CredLeakInfo::SetProcessCount(int processCount)
{
    processCount_ = processCount;
}

int CredLeakInfo::GetProcessCount()
{
    return processCount_;
}

void CredLeakInfo::SetThreadsCount(int threadsCount)
{
    threadsCount_ = threadsCount;
}

int CredLeakInfo::GetThreadsCount()
{
    return threadsCount_;
}

void CredLeakInfo::SetMemTotal(unsigned int memTotal)
{
    memTotal_ = memTotal;
}

unsigned int CredLeakInfo::GetMemTotal()
{
    return memTotal_;
}

void CredLeakInfo::SetMemUsed(unsigned int memUsed)
{
    memUsed_ = memUsed;
}

unsigned int CredLeakInfo::GetMemUsed()
{
    return memUsed_;
}

void CredLeakInfo::SetMemOrderList(const string &memOrderList)
{
    memOrderList_ = memOrderList;
}

std::string CredLeakInfo::GetMemOrderList()
{
    return memOrderList_;
}

/********************credEventHandler*********************** */
std::shared_ptr<LeakEventInfoBase> CredEventHandler::HandleMessage(SysEvent sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != KERNEL_UID) {
        HIVIEW_LOGE("Calling UID is not KERNEL_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return nullptr;
    }
    int32_t processCount = sysEvent.GetEventIntValue("PROCESS_COUNT");
    int32_t threadsCount = sysEvent.GetEventIntValue("THREADS_COUNT");
    int32_t memTotal = sysEvent.GetEventIntValue("MEM_TOTAL");
    int32_t memUsed = sysEvent.GetEventIntValue("MEM_USED");
    std::string memOrderList = sysEvent.GetEventValue("MEM_ORDER_LIST");
    shared_ptr<LeakEventInfoBase> eventInfo = make_shared<CredLeakInfo>("Cred");
    if (eventInfo == nullptr) {
        return nullptr;
    }
    auto credInfo = static_pointer_cast<CredLeakInfo>(eventInfo);
    if (credInfo->IsEverReported()) {
        HIVIEW_LOGE("Event: %{public}s has Reported at %{public}ld",
            credInfo->GetEventName().c_str(), credInfo->GetLastReportRecord());
        return nullptr;
    }
    credInfo->SetLeakedTime();
    credInfo->SetProcessCount(processCount);
    credInfo->SetThreadsCount(threadsCount);
    credInfo->SetMemTotal(memTotal);
    credInfo->SetMemUsed(memUsed);
    credInfo->SetMemOrderList(memOrderList);
    return eventInfo;
}

ErrCode CredEventHandler::DumpMessage(shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto credInfo = static_pointer_cast<CredLeakInfo>(eventInfo);
    std::string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    const std::string &filePath = credInfo->GetFilePath() + credInfo->GetFileName();
    const std::string &realPath = DfrUtil::GetInstance().RealPath(filePath);
    HIVIEW_LOGD("realpath, path: %{public}s", filePath.c_str());
    std::ofstream fout;
    fout.open(realPath, std::ios::out);
    if (!fout.is_open()) {
        HIVIEW_LOGE("FAILED TO OPEN FILE: %{public}s", realPath.c_str());
        return FAILURE;
    }
    credInfo->PrintMemList(fout);
    fout.close();
    return SUCCESSED;
}

ErrCode CredEventHandler::ReportMessage(shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto credInfo = static_pointer_cast<CredLeakInfo>(eventInfo);
    std::string summaryInfo = "CRED_LEAK PROCESS_COUNT: " + to_string(credInfo->GetProcessCount()) +
                              ", THREADS_COUNT: " + to_string(credInfo->GetThreadsCount()) +
                              ", MEM_TOTAL: " + to_string(credInfo->GetMemTotal()) +
                              ", MEM_USED: " + to_string(credInfo->GetMemUsed()) +
                              ", MEM_ORDER_LIST: " + credInfo->GetMemOrderList();

    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK_WP", HiSysEvent::EventType::FAULT,
        "MEMTOTAL", credInfo->GetMemTotal(),
        "MODULE", credInfo->GetEventName(),
        "HAPPEN_TIME", credInfo->GetLeakedTime(),
        "FIRST_FRAME", credInfo->GetEventName(),
        "SECOND_FRAME", "WARNING",
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(credInfo->GetEventName(), 0),
        "SUMMARY", summaryInfo);
    credInfo->RecordReport();
    return SUCCESSED;
}

} // HiviewDFX
} // OHOS