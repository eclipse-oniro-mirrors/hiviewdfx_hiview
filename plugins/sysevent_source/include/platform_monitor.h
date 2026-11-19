/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_CORE_PLATFROM_MONITOR_H
#define HIVIEW_CORE_PLATFROM_MONITOR_H
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "event_loop.h"
#include "pipeline.h"
#include "sys_event.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
struct PerfMeasure {
    uint32_t maxTotalCount;
    uint32_t maxTotalSize;
    uint32_t totalCount;
    uint32_t totalSize;
    uint8_t breakCount;
    uint64_t breakDuration;
    uint32_t minSpeed;
    uint32_t maxSpeed;
    std::vector<uint32_t> realCounts;
    std::vector<uint32_t> processCounts;
    std::vector<uint32_t> waitCounts;
    uint32_t finishedCount;
    uint32_t overRealTotalCount;
    uint32_t overProcessTotalCount;
    uint32_t realPercent;
    uint32_t processpercent;
};

class PlatformMonitor {
public:
    PlatformMonitor(): maxTotalCount_(0), maxTotalSize_(0), looper_(nullptr) {}
    ~PlatformMonitor() {}
    void Breaking();
    void CollectCostTime(PipelineEvent *event);
    void StartMonitor(std::shared_ptr<EventLoop> looper);

private:
    void CollectPerfProfiler();
    void ReportBreakProfile();
    void ReportCycleProfile();
    void ReportRecoverProfile();
    void AccumulateTimeInterval(uint64_t costTime, std::map<int8_t, uint32_t> &stat);
    void CalcOverBenckMarkPct(PerfMeasure &perfMeasure);
    void ReportProfile(const PerfMeasure& perfMeasure);
    void GetCostTimeInterval(PerfMeasure &perfMeasure);
    void GetMaxSpeed(PerfMeasure &perfMeasure) const;
    void GetMaxTotalMeasure(PerfMeasure &perfMeasure);
    void GetBreakStat(PerfMeasure &perfMeasure);
    void InitData();

private:
    static constexpr uint8_t PCT = 100;
    uint32_t collectPeriod_ = 5 * TimeUtil::SECONDS_PER_MINUTE; // 5 minute
    uint32_t reportPeriod_ = TimeUtil::SECONDS_PER_HOUR; // one hour
    uint32_t totalSizeBenchMark_ =  200 * 1024 * 1024; // 200M
    uint32_t realTimeBenchMark_ = 100 * 1000; // 100 millisecond
    uint32_t processTimeBenchMark_ = 200 * 1000; // millisecond
    // 50, 100, 200, 500 millisecond, 1, 10, 100, 500, 1000 second,
    static constexpr uint64_t intervals_[] = {50 * 1000, 100 * 1000, 200 * 1000, 500 * 1000, 1000 * 1000,
                                              10 * 1000 * 1000, 100 * 1000 * 1000, 500 * 1000 * 1000,
                                              1000 * 1000 * 1000};

    // intervals
    std::mutex statMutex_;
    std::map<int8_t, uint32_t> realStat_;
    std::map<int8_t, uint32_t> processStat_;
    std::map<int8_t, uint32_t> waitTimeStat_;

    // max
    std::atomic<uint32_t> maxTotalCount_;
    std::atomic<uint32_t> maxTotalSize_;

    // break
    uint32_t totalCount_ = 0;
    uint32_t totalSize_ = 0;
    uint64_t breakTimestamp_ = 0;
    uint64_t recoverTimestamp_ = 0;
    uint8_t breakCount_ = 0;
    uint64_t breakDuration_ = 0;

    // over brenchmark
    uint32_t finishedCount_ = 0;
    uint32_t overRealTotalCount_ = 0;
    uint32_t overProcessTotalCount_ = 0;

    // speed
    uint32_t onceTotalRealTime_ = 0;
    uint32_t onceTotalProcTime_ = 0;
    uint32_t onceTotalWaitTime_ = 0;
    uint32_t onceTotalCnt_ = 0;
    uint32_t minSpeed_ = 0;
    uint32_t maxSpeed_ = 0;
    uint32_t curRealSpeed_ = 0;
    uint32_t curProcSpeed_ = 0;

    // avg
    double avgRealTime_ = 0;
    double avgProcessTime_ = 0;
    double avgWaitTime_ = 0;

    std::shared_ptr<EventLoop> looper_;
}; // PlatformMonitor
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_CORE_PLATFROM_MONITOR_H