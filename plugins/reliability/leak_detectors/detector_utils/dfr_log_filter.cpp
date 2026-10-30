/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "dfr_log_filter.h"

#include <fstream>
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "DfrLogFilter");
using namespace std;

namespace {
// 匹配日志分段标记：命中则更新当前分段类型并写出该行
bool MatchAndWriteSection(const string &line, const string &sectionSign, FilterMethod newFilter,
    FilterMethod &filterType, std::ofstream &outFile)
{
    if (line.find(sectionSign) == string::npos) {
        return false;
    }
    filterType = newFilter;
    outFile << line << endl;
    return true;
}

// 依次尝试各日志分段标记，命中任一则该行已写出、分段类型已更新
bool MatchAnySectionAndWrite(const string &line, FilterMethod &filterType, std::ofstream &outFile)
{
    // Memcheck
    if (MatchAndWriteSection(line, LOGGER_MEMCHECK_MEMTRACK_HEAD, FILTER_MEMTRACK, filterType, outFile)) {
        return true;
    }
    // -----------
    if (MatchAndWriteSection(line, "-------------------", filterType, filterType, outFile)) {
        return true;
    }
    // ***********
    if (MatchAndWriteSection(line, "**************", filterType, filterType, outFile)) {
        return true;
    }
    // Ashmem
    if (MatchAndWriteSection(line, "ASHMEM_PROCESS_INFO", FILTER_ASHMEM, filterType, outFile)) {
        return true;
    }
    // Ion
    if (MatchAndWriteSection(line, "ION_PROCESS_INFO", FILTER_ION, filterType, outFile)) {
        return true;
    }
    // GPU
    if (MatchAndWriteSection(line, "GPU_PROCESS_INFO", FILTER_GPU, filterType, outFile)) {
        return true;
    }
    // end sign
    if (MatchAndWriteSection(line, "***** endl ******", NO_FILTER, filterType, outFile)) {
        return true;
    }
    // NMD
    if (MatchAndWriteSection(line, "LOGGER_MEMCHECK_SAMPLE_NMD_INFO", NO_FILTER, filterType, outFile)) {
        return true;
    }
    return false;
}
}

namespace {
constexpr int GPU_PID_POS = 2;
}

void DfrLogFilter::FilterMemoryLeakFile(const pair<string, int> &processInfo,
    const string &srcPath, const string &destPath)
{
    if (!DfrUtil::GetInstance().FileExist(srcPath)) {
        HIVIEW_LOGE("file not exists: %{public}s", srcPath.c_str());
        return;
    }
    const string &srcRealPath = DfrUtil::GetInstance().RealPath(srcPath);
    const string &dstRealPath = DfrUtil::GetInstance().RealPath(destPath);
    if (srcRealPath.empty() || dstRealPath.empty()) {
        HIVIEW_LOGE("invalid srcpath: %{public}s or destPath: %{public}s", srcPath.c_str(), destPath.c_str());
        return;
    }
    std::ifstream inFile(srcRealPath);
    if (!inFile.is_open()) {
        HIVIEW_LOGE("file open failed, input: %{public}s", srcRealPath.c_str());
        return;
    }

    std::ofstream outFile(dstRealPath);
    if (!outFile.is_open()) {
        HIVIEW_LOGE("file open failed, output: %{public}s", dstRealPath.c_str());
        inFile.close();
        return;
    }
    FilterMethod filterType = NO_FILTER;
    bool shouldwriteGpu = false;
    string line;
    while (std::getline(inFile, line)) {
        if (MatchAnySectionAndWrite(line, filterType, outFile)) {
            continue;
        }

        if (filterType == FILTER_MEMTRACK) {
            continue;
        }
        if (filterType == FILTER_GPU && ShouldFilterGpuInfo(processInfo.second, line, shouldwriteGpu)) {
            continue;
        }
        if (filterType != NO_FILTER && !ShouldFilterProcInfo(processInfo.first, filterType, line)) {
            continue;
        }
        outFile << line << endl;
    }
    inFile.close();
    outFile.close();
}

bool DfrLogFilter::ShouldFilterProcInfo(const string &processName, FilterMethod filterType, const string &line)
{
    map<FilterMethod, FilterSig> filterSigList {
        { FILTER_ASHMEM, { "Process ashmem detail info:", "Process_name\tProcess_ID", "**** endl ****", 0, '\t' } },
        { FILTER_ION, { "Process dma_heap info", "Process name", "**** endl ****", 0, ' ' } },
        { FILTER_DMABUF, { "Dma-buf objects usage of processes", "Process  ", "**** endl ****", 0, ' ' } },
    };
    if (filterSigList.count(filterType) == 0) {
        HIVIEW_LOGE("cannot match filterType: %{public}d", filterType);
        return true;
    }
    if (line.find(filterSigList.at(filterType).tableHeader) != string::npos) {
        return true;
    }
    return DfrUtil::GetInstance().IsProcessRelatedLine(line, 0, processName, filterSigList.at(filterType).interrupt);
}

bool DfrLogFilter::ShouldFilterGpuInfo(int pid, const string &line, bool &shouldwriteGpu)
{
    FilterSig gpuFilterSig { "", "used summary", "", GPU_PID_POS, ' ' };
    if (line.find(gpuFilterSig.tableHeader) != string::npos) {
        shouldwriteGpu = !DfrUtil::GetInstance().IsProcessRelatedLine(line, gpuFilterSig.pos, pid,
            gpuFilterSig.interrupt);
    }
    return shouldwriteGpu;
}

} // HiviewDFX
} // OHOS
