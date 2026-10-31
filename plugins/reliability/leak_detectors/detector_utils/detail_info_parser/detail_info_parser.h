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

#ifndef DETAIL_INFO_PARSER_H
#define DETAIL_INFO_PARSER_H

#include <atomic>
#include "ffrt.h"
#include "dfr_util.h"
#include "memory_collector.h"

namespace OHOS {
namespace HiviewDFX {

enum class SmapsStatType {
    RSS_TYPE,
    PSS_TYPE
};

using DetailDmaBufInfo = AppEventHandler::DetailDmaBufInfo;
using DetailGpuInfo = AppEventHandler::DetailGpuInfo;
using DetailSmapsInfo = AppEventHandler::DetailSmapsInfo;
using DetailExtPssInfo = AppEventHandler::DetailExtPssInfo;

class DetailInfoParser : public Singleton<DetailInfoParser> {
    DECLARE_SINGLETON(DetailInfoParser);

public:
    DetailDmaBufInfo ParseIonProcessInfo(const std::string &data);
    DetailGpuInfo ParseGpuProcessInfo(const std::string &data);
    DetailSmapsInfo GetSmapsAndParse(pid_t pid, const std::string &detectorTypeName,
                                     SmapsStatType statType = SmapsStatType::PSS_TYPE);
    DetailExtPssInfo GetExtPssParseInfo(const pid_t pid, const std::string &detectorTypeName,
                                        SmapsStatType statType = SmapsStatType::PSS_TYPE);
    // 泛pss整体拆解

private:
    std::string AlignType(const std::string &oldDmaBufType);
    void AddDmabufSize(DetailDmaBufInfo &detailDmaBufInfo, const std::string &dmaBufType, const int &sizeBytes);
    void ComputeGpuHookSize(const std::string &data, DetailGpuInfo &detailGpuInfo);
    void JudgeCGpuType(const std::string &line, DetailGpuInfo &detailGpuInfo);
    void JudgeChannelGpuType(const std::string &line, DetailGpuInfo &detailGpuInfo);
    DetailGpuInfo GetGpuProcessInfoAndParse(pid_t pid);
    DetailDmaBufInfo GetIonProcessInfoAndParse(pid_t pid);
    struct SmapsCollectResult {
        std::vector<MemoryDetail> memoryDetails;
        uint64_t totalRss = 0;
        uint64_t totalSwap = 0;
        uint64_t totalPss = 0;
        uint64_t totalSwapPss = 0;
    };
    struct SmapsAggContext {
        SmapsStatType statType;
        DetailSmapsInfo &detailSmapsInfo;
        uint64_t &classTotalMem;
    };
    bool CollectSmapsData(pid_t pid, const std::string &detectorTypeName, SmapsCollectResult &result);
    void ProcessMemoryItem(const MemoryItem &memoryItem, const std::string &memoryClassStr,
        MemoryClass memoryClass, SmapsAggContext &context);
    void AggregateMemoryItems(const std::vector<MemoryDetail> &memoryDetails, SmapsAggContext &context);
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif  // DETAIL_INFO_PARSER_H