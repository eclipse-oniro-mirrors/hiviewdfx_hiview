/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "fd_orphan_vnode_scan.h"

#include <ctime>
#include <fstream>
#include <sstream>

#include "dfr_util.h"
#include "fd_leak_dump_client.h"
#include "fd_leak_config.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "old_log_cleaner.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdOrphanVnodeScan");

namespace {
constexpr int32_t ORPHAN_VNODE_REPEAT_INTERVAL = 24 * 3600; // 24 hours in seconds
const  std::string DIVIDING_LINE = "======================================================";
}

FdOrphanVnodeScan::FdOrphanVnodeScan() {}

FdOrphanVnodeScan::~FdOrphanVnodeScan() {}

void FdOrphanVnodeScan::ReportOrphanVnodeLeakEvent(const FdOrphanVnodeInfo &info)
{
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "ORPHAN_VNODE_LEAK", HiSysEvent::EventType::FAULT,
        "PID", info.pid,
        "FD_NUM", info.fdNum,
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(
            info.processName + std::to_string(info.fdNum) + "ORPHAN_VNODE_LEAK", 0),
        "HAPPEN_TIME", info.timeStr,
        "FIRST_FRAME", "orphan_vnode",
        "SECOND_FRAME", "OVER_LIMIT",
        "MODULE", info.processName.empty() ? "/" : info.processName,
        "LOG_PATH", std::string(FILE_DESCRIPTOR_LEAK_PATH) + "orphanvnode-" + info.processName + "-" +
            std::to_string(info.pid) + "-log-" + info.timeStr + ".txt",
        "SUMMARY", "pid: " + std::to_string(info.pid) + ", fdNum: " + std::to_string(info.fdNum) +
            ", processName: " + info.processName + " find orphanvnode");
}

void FdOrphanVnodeScan::ExtractTopProcessInfo(const std::vector<std::string>& topOrphanVnodeList,
    int32_t& pid, std::string& processName, int32_t& fdNum)
{
    if (topOrphanVnodeList.empty()) {
        return;
    }

    std::istringstream iss(topOrphanVnodeList[0]);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("pid:") == 0) {
            std::istringstream lineStream(line);
            std::string key;
            std::string value;
            lineStream >> key >> value;
            DfrUtil::GetInstance().ConvertToInt(value, pid);
        } else if (line.find("processName:") == 0) {
            std::istringstream lineStream(line);
            std::string key;
            lineStream >> key >> processName;
        } else if (line.find("fdNum:") == 0) {
            std::istringstream lineStream(line);
            std::string key;
            std::string value;
            lineStream >> key >> value;
            DfrUtil::GetInstance().ConvertToInt(value, fdNum);
        }
    }
}

void FdOrphanVnodeScan::RecordProcInfo(std::ofstream &fout, const std::string &procFilePath,
                                       const std::string &title)
{
    std::string content = DfrUtil::GetInstance().GetCacheProcInfo(procFilePath);
    if (content.empty()) {
        return;
    }
    fout << "\n=== " << title << " begin ===\n" << std::endl;
    fout << content << std::endl;
    fout << "=== " << title << " end ===" << std::endl;
}

void FdOrphanVnodeScan::DumpOrphanVnodeInfo(const std::vector<std::string> &topOrphanVnodeList,
    const FdOrphanVnodeInfo &info)
{
    if (topOrphanVnodeList.empty()) {
        HIVIEW_LOGE("topOrphanVnodeList is empty");
        return;
    }

    std::ofstream fout;
    FileParams fileParams;
    fileParams.fileName = "orphanvnode-" + info.processName + "-" + std::to_string(info.pid) + "-log-" +
        info.timeStr + ".txt";
    fileParams.procName = info.processName;
    fileParams.logPath = FILE_DESCRIPTOR_LEAK_PATH + fileParams.fileName;
    fileParams.foutMode = std::ios::trunc;

    OldLogCleaner::GetInstance().CreateFile(fileParams, fout, false);
    if (!fout.is_open()) {
        HIVIEW_LOGE("Failed to open fd leak log file");
        return;
    }
    if (DfrUtil::GetInstance().IsBetaOrDevelopMode() && !topOrphanVnodeList.empty()) {
        fout << "=== Top3 Orphan Vnode List begin ===\n" << std::endl;
        for (const auto &info : topOrphanVnodeList) {
            fout << info << std::endl;
            fout << DIVIDING_LINE << std::endl;
        }
        fout << "=== Top3 Orphan Vnode List end ===" << std::endl;
    }
    RecordProcInfo(fout, PROC_SLABINFO_PATH, "/proc/slabinfo");
    RecordProcInfo(fout, HMFS_ORPHAN_NODES_DETAIL_INFO_PATH, "orphan_nodes_detail_info");

    fout.close();
    HIVIEW_LOGI("DumpOrphanVnodeInfo success");
}

void FdOrphanVnodeScan::ScanOrphanVnodeOnChargeSleep()
{
    time_t lastScanTime = lastScanOrphanVnodeTime_.load();
    if (!DfrUtil::GetInstance().ShouldDetectAfterInterval(lastScanTime, ORPHAN_VNODE_REPEAT_INTERVAL)) {
        HIVIEW_LOGW("has scan orphan vnode repeated! lastScanTime: %{public}s",
                    DfrUtil::GetInstance().TimeStampToStr(lastScanTime).c_str());
        return;
    }
    ffrt::submit([=] {
        std::vector<std::string> topOrphanVnodeList;
        ErrCode ret = FdLeakDumpClient::GetInstance().SendScanOrphanVnodeOverLimitRequest(
            FdLeakConfig::GetInstance().GetThreshold(),
            FdLeakConfig::GetInstance().GetOrphanVnodeThreshold(),
            topOrphanVnodeList);
        if (ret != SUCCESSED) {
            HIVIEW_LOGE("SendScanOrphanVnodeOverLimitRequest failed, ret is %{public}d", ret);
            return;
        }
        if (topOrphanVnodeList.empty()) {
            HIVIEW_LOGI("No orphan vnode over limit found, skip processing");
            return;
        }

        FdOrphanVnodeInfo info;
        info.timeStr = DfrUtil::GetInstance().GetRealTimeStampStr();
        ExtractTopProcessInfo(topOrphanVnodeList, info.pid, info.processName, info.fdNum);

        DumpOrphanVnodeInfo(topOrphanVnodeList, info);
        ReportOrphanVnodeLeakEvent(info);
        lastScanOrphanVnodeTime_.store(time(nullptr));
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_FdOrphanVnodeScan_ScanOrphanVnodeOnChargeSleep")});
}

} // namespace HiviewDFX
} // namespace OHOS