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
 * Author: zhengkaijie
 * Create: 2024-7-18
 */

#include "fault_dump_state_base.h"
#include "kernel_leak_config.h"
#include "fault_common.h"
#include "hitrace_meter.h"
#include "old_log_cleaner.h"
#include "sys_event.h"
#include "hiview_global.h"
#include "string_ex.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FaultLeakDumpStateBase");

using namespace std;

namespace {
constexpr int WEIGHT_8 = 8;
constexpr int WEIGHT_16 = 16;
constexpr int WEIGHT_32 = 32;
constexpr int CAN_RECLAIM_POS = 10;
constexpr int IS_RECLAIM_POS = 11;
constexpr int NULL_POS = 13;

const string SYSTEM_HEAP_FILE_PATH = "/sys/kernel/debug/dmaheap/heaps/system_heap";
const string NEW_ION_PROC_FILE_PATH = "/proc/process_dmabuf_info";

const vector<CommonDumpType> FORK_DUMP_LIST = {
    SMAPS_INFO,
    RENDER_SERVICE_MEM,
    MEDIA_MEM,
    AVC_MEM,
};
};

void FaultDumpStateBase::DoDumpLog(DumperInfo dumperInfo)
{
    for (auto item : dumperInfo.pathMap) {
        // ensure log dump by order
        logDumpMtx_.lock();
        dumperInfo.dumpType = item.first;
        DoPerDumpLog(dumperInfo);
        logDumpMtx_.unlock();
    }
}

void FaultDumpStateBase::DoPerDumpLog(DumperInfo &dumperInfo)
{
    CommonDumpType dumpType = dumperInfo.dumpType;
    HIVIEW_LOGD("%{public}s DoPerDumpLog dump %{public}s", DETECTOR_TYPE_NAME[dumperInfo.detectorType],
        CommonDumpTypeStr.at(dumpType).c_str());
    if (count(FORK_DUMP_LIST.begin(), FORK_DUMP_LIST.end(), dumpType)) {
        ForkProcessToDumpInfo(dumperInfo);
        return;
    }
    if (DfrUtil::GetInstance().IsApp(dumperInfo.pid)) {
        DoAppPerDumpLog(dumperInfo);
    } else {
        DoServicePerDumpLog(dumperInfo);
    }
}

static string GetPathFromPathMap(DumperInfo &dumperInfo)
{
    for (auto item : dumperInfo.pathMap) {
        if (item.first == dumperInfo.dumpType) {
            return item.second;
        }
    }
    return "";
}

void FaultDumpStateBase::DoAppPerDumpLog(DumperInfo &dumperInfo)
{
    CommonDumpType appDumpType = dumperInfo.dumpType;
    if ((appDumpType & ION_PROCESS_INFO)) {
        DumpMmDmabuf(dumperInfo);
        return;
    }
    if ((appDumpType & GPU_PROCESS_INFO)) {
        ofstream fout;
        if (!(GetWriteFout(dumperInfo, fout))) {
            return;
        }
        DfrUtil::GetInstance().DumpSingleDetailFromIoctl(dumperInfo.pid, CMD_GET_MEM_DETAIL_INFO,
                                                         MTYPE_USER_GPU, fout);
    } else {
        const CommonDumpTypeMap procNodeMap = KernelLeakConfig::GetInstance().GetProcNodeMap();
        // dump proc info based on type
        if (procNodeMap.count(appDumpType)) {
            DumpProcInfo(dumperInfo);
        }
    }
}

void FaultDumpStateBase::DoServicePerDumpLog(DumperInfo &dumperInfo)
{
    CommonDumpType serviceDumpType = dumperInfo.dumpType;
    const CommonDumpTypeMap procNodeMap = KernelLeakConfig::GetInstance().GetProcNodeMap();
    // dump proc info based on type
    if (procNodeMap.count(serviceDumpType)) {
        DumpProcInfo(dumperInfo);
    }
    if ((serviceDumpType & SYSTEM_HEAP)) {
        DumpSystemHeapInfo(dumperInfo);
    }
    if ((serviceDumpType & ION_NEW_PROC)) {
        DumpIonNewProcInfo(dumperInfo);
    }
    if ((serviceDumpType & SYSVIEW)) {
        DumpSysviewProc(dumperInfo);
    }
}

void FaultDumpStateBase::DumpMmDmabuf(DumperInfo &dumperInfo) const
{
    string procFilePath = "/proc/" + to_string(dumperInfo.pid) + "/mm_dmabuf_info";

    string traceStr(__func__);
    traceStr.append(":").append(procFilePath);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);

    ofstream fout;
    if (!(GetWriteFout(dumperInfo, fout))) {
        return;
    }

    string finBuffer = DfrUtil::GetInstance().GetCacheProcInfo(procFilePath);
    if (finBuffer.empty()) {
        HIVIEW_LOGW("the finBuffer of %{public}s info is empty! ", procFilePath.c_str());
        finBuffer = "finBuffer is empty!";
    }
    fout << endl;
    fout << "*****************************" << endl;
    fout << "LOGGER_MEMCHECK_PROC_INFO" << endl;
    fout << "MM_DMABUF_INFO" << endl;
    fout << "realtime:\t" << DfrUtil::GetInstance().GetRealTime() << endl;
    string line;
    stringstream finBufferStream(finBuffer);
    fout << left;
    while (getline(finBufferStream, line)) {
        istringstream lineStream(line);
        vector<string> elemts;
        int count = 0;
        string elemt;
        while (lineStream >> elemt) {
            count++;
            if (count == CAN_RECLAIM_POS || count == IS_RECLAIM_POS || count == NULL_POS) {
                continue;
            }
            elemts.push_back(elemt);
        }
        for (size_t i = 0; i < elemts.size(); i++) {
            fout << setw(WEIGHT_8) << elemts[i] << "\t";
        }
        fout << endl;
    }
    fout << "************ endl ************" << endl << endl;
    fout.close();
}

string FaultDumpStateBase::GetProcFilePath(CommonDumpType dumpType)
{
    const CommonDumpTypeMap procNodeMap = KernelLeakConfig::GetInstance().GetProcNodeMap();
    KernelTypeParam kernelTypeParam = procNodeMap.at(dumpType);
    string procName = kernelTypeParam.procName;
    string meminfoName = kernelTypeParam.meminfoName;
    string procFilePath = DfrUtil::GetInstance().GetProcPath(meminfoName, procName);
    return procFilePath;
}

void FaultDumpStateBase::DumpProcInfo(DumperInfo &dumperInfo)
{
    string procFilePath = GetProcFilePath(dumperInfo.dumpType);
    string traceStr(__func__);
    traceStr.append(":").append(procFilePath);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    ofstream fout;
    if (!(GetWriteFout(dumperInfo, fout))) {
        return;
    }

    string finBuffer = DfrUtil::GetInstance().GetCacheProcInfo(procFilePath);
    if (finBuffer.empty()) {
        HIVIEW_LOGW("the finBuffer of %{public}s info is empty! ", procFilePath.c_str());
        finBuffer = "finBuffer is empty!";
    }
    fout << endl;
    fout << "*****************************" << endl;
    fout << "LOGGER_MEMCHECK_PROC_INFO" << endl;
    fout << CommonDumpTypeStr.at(dumperInfo.dumpType) << endl;
    fout << "realtime:\t" << DfrUtil::GetInstance().GetRealTime() << endl;
    fout << finBuffer.c_str() << endl;
    fout << "************ endl ************" << endl;
    fout << endl;
    fout.close();
}

void FaultDumpStateBase::DumpSystemHeapInfo(DumperInfo &dumperInfo) const
{
    string traceStr(__func__);
    traceStr.append(":").append(SYSTEM_HEAP_FILE_PATH);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);

    ofstream fout;
    if (!(GetWriteFout(dumperInfo, fout))) {
        return;
    }
    ifstream fin(SYSTEM_HEAP_FILE_PATH);
    if (!fin.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s", SYSTEM_HEAP_FILE_PATH.c_str());
        return;
    }
    fout << endl;
    fout << "*****************************" << endl;
    fout << "LOGGER_MEMCHECK_SYSTEM_HEAP_INFO" << endl;
    fout << fin.rdbuf();
    fout << "************ endl ************" << endl;
    fout << endl;
    fin.close();
    fout.close();
}

void FaultDumpStateBase::DumpSysviewProc(DumperInfo &dumperInfo) const
{
    string traceStr(__func__);
    traceStr.append(":").append(PROC_SYSVIEW);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);

    ofstream fout;
    if (!(GetWriteFout(dumperInfo, fout))) {
        return;
    }
    string finBufferIonNew = DfrUtil::GetInstance().GetCacheProcInfo(PROC_SYSVIEW);
    if (finBufferIonNew.empty()) {
        HIVIEW_LOGW("the finBuffer of %{public}s info is empty! ", PROC_SYSVIEW);
        finBufferIonNew = "finBuffer is empty!";
    }
    fout << "*****************************" << endl;
    fout << "LOGGER_PROCESS_SYSVIEW_INFO" << endl;
    fout << finBufferIonNew.c_str() << endl;
    fout << "************ endl ************" << endl;
    fout << endl;
    fout.close();
}

void FaultDumpStateBase::DumpIonNewProcInfo(DumperInfo &dumperInfo) const
{
    string traceStr(__func__);
    traceStr.append(":").append(NEW_ION_PROC_FILE_PATH);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);

    ofstream fout;
    if (!(GetWriteFout(dumperInfo, fout))) {
        return;
    }
    string finBufferIonNew = DfrUtil::GetInstance().GetCacheProcInfo(NEW_ION_PROC_FILE_PATH);
    if (finBufferIonNew.empty()) {
        HIVIEW_LOGW("the finBuffer of %{public}s info is empty! ", NEW_ION_PROC_FILE_PATH.c_str());
        finBufferIonNew = "finBuffer is empty!";
    }
    fout << "*****************************" << endl;
    fout << "LOGGER_PROCESS_DMABUF_INFO" << endl;
    fout << finBufferIonNew.c_str() << endl;
    fout << "************ endl ************" << endl;
    fout << endl;
    fout.close();
}

inline string GetDumpTypeString(CommonDumpType dumpType)
{
    auto it = CommonDumpTypeStr.find(dumpType);
    if (it != CommonDumpTypeStr.end()) {
        return it->second;
    }
    return "";
}

static inline bool Dup2Fd(int srcFd, int targetFd, const string& detectorTypeName)
{
    int ret = dup2(srcFd, targetFd);
    if (ret < 0) {
        HIVIEW_LOGE("%{public}s dup2 writeFd fail, err: %{public}s", detectorTypeName.c_str(), strerror(errno));
        return false;
    }
    return true;
}

bool FaultDumpStateBase::ForkProcessToDumpInfo(DumperInfo &dumperInfo)
{
    string detectorTypeName = DETECTOR_TYPE_NAME[dumperInfo.detectorType];
    int writeFd = 0;
    if (!(GetWriteFd(dumperInfo, writeFd))) {
        return false;
    }
    fdsan_exchange_owner_tag(writeFd, 0, FDSAN_TAG);

    string dumpTypeString = GetDumpTypeString(dumperInfo.dumpType);
    if (!dumpTypeString.empty()) {
        string generalMessage = string("*****************************\n") +
            string("LOGGER_MEMCHECK_") + dumpTypeString.c_str() + "\n" +
            string("get info realtime:\t") + DfrUtil::GetInstance().GetRealTime() + "\n";
        size_t generalMessageSize = generalMessage.size();
        ssize_t writeRet = write(writeFd, generalMessage.c_str(), generalMessageSize);
        if (writeRet != static_cast<ssize_t>(generalMessageSize)) {
            HIVIEW_LOGE("%{public}s write incomplete, expected %{public}zu wrote %{public}zd, err: %{public}s",
                detectorTypeName.c_str(), generalMessageSize, writeRet, strerror(errno));
        }
        if (writeRet < 0) {
            HIVIEW_LOGE("%{public}s write get extra info realtime failed, err: %{public}s",
                detectorTypeName.c_str(), strerror(errno));
        }
    }

    if (!Dup2Fd(writeFd, STDIN_FILENO, detectorTypeName) || !Dup2Fd(writeFd, STDOUT_FILENO, detectorTypeName)
        || !Dup2Fd(writeFd, STDERR_FILENO, detectorTypeName)) {
        fdsan_close_with_tag(writeFd, FDSAN_TAG);
        return false;
    }
    fdsan_close_with_tag(writeFd, FDSAN_TAG);
    return DfrUtil::GetInstance().ForkProcessAndDump(dumperInfo.pid, dumperInfo.dumpType, detectorTypeName);
}

bool FaultDumpStateBase::GetWriteFout(DumperInfo &dumperInfo, ofstream &fout) const
{
    string fileLogPath = GetPathFromPathMap(dumperInfo);
    string detectorTypeName = DETECTOR_TYPE_NAME[dumperInfo.detectorType];
    FileParams  fileParams = {
        .procName = dumperInfo.processName,
        .logPath = fileLogPath,
        .foutMode = ios::out | ios::app,
    };
    size_t lastSlashPos = fileLogPath.find_last_of('/');
    fileParams.fileName = (lastSlashPos == std::string::npos) ?
                         fileLogPath : fileLogPath.substr(lastSlashPos + 1);
    OldLogCleaner::GetInstance().CreateFile(fileParams, fout);
    if (!fout.is_open()) {
        HIVIEW_LOGE("%{public}s FAILED TO OPEN FILE:%{public}s", detectorTypeName.c_str(), fileLogPath.c_str());
        return false;
    }

    return true;
}

bool FaultDumpStateBase::GetWriteFd(DumperInfo &dumperInfo, int &writeFd) const
{
    string detectorTypeName = DETECTOR_TYPE_NAME[dumperInfo.detectorType];
    const string &filePath = GetPathFromPathMap(dumperInfo);
    const string &realPath = DfrUtil::GetInstance().RealPath(filePath);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath path: %{public}s", filePath.c_str());
        return false;
    }
    uint32_t fileAction = O_CREAT | O_RDWR | O_APPEND;
    writeFd = open(realPath.c_str(), fileAction, DEFAULT_LOG_FILE_MODE);
    if (writeFd < 0) {
        HIVIEW_LOGE("%{public}s failed to open %{public}s, err: %{public}s",
            detectorTypeName.c_str(), realPath.c_str(), strerror(errno));
    }
    return writeFd > 0;
}
} // namespace HiviewDFX
} // namespace OHOS

