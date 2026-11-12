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
#include "native_leak_growth_analyser.h"

#include <cmath>
#include "hisysevent.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakGrowthAnalyser");

namespace {
const double NEGATIVE_NATURAL_LOG_2 = -log(2);
constexpr time_t HALF_LIFE_LEVEL = 120; // 120s
constexpr time_t HALF_LIFE_VAR = 1800;
constexpr time_t HALF_LIFE_SLOPE = 60;
constexpr double POW_2 = 2.0;
constexpr double SLOPE_ALERT = 40 * 1024 / 60; // KB/s
constexpr time_t SLOPE_PERSIST_TIME = 600;
constexpr double Z_SCORE_ALERT = 2.4;
constexpr time_t CALCULATE_TIME = 1800;
constexpr time_t REPORT_INTERVAL = 24 * 60 * 60; // one day
}

NativeLeakGrowthAnalyser::NativeLeakGrowthAnalyser() {}

NativeLeakGrowthAnalyser::~NativeLeakGrowthAnalyser() {}

void NativeLeakGrowthAnalyser::DoMemGrowthAnalyze(const map<pid_t, std::shared_ptr<FaultInfoBase>> allList)
{
    for (const auto &it : allList) {
        auto faultInfo = it.second;
        if (!faultInfo->IsApp()) {
            continue;
        }
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
        pid_ = nlInfo->GetPid();
        processName_ = nlInfo->GetProcessName();
        newAS_ = nlInfo->GetLastAnalyzeSample();
        if (AnalyzeMemGrowth(nlInfo)) {
            nlInfo->UpdateAnalyzeSample(newAS_);
        }
        leakReason_ = "";
        persisted_ = false;
        zScore_ = 0.0;
    }
}

bool NativeLeakGrowthAnalyser::AnalyzeMemGrowth(const shared_ptr<NativeLeakInfo> &nlInfo)
{
    auto lastSR = nlInfo->GetLastSampleRecord();
    time_t newTime = lastSR.time;
    if (newAS_.lastTime == newTime) {
        HIVIEW_LOGD("sample of process: %{public}s:%{public}d has not been updated", processName_.c_str(), pid_);
        return false;
    }
    if (newAS_.lastTime == 0) {
        newAS_.firstTime = newTime;
        HIVIEW_LOGI("get real pss of process %{public}s:%{public}d because of its first time analyze mem growth",
            processName_.c_str(), pid_);
        nlInfo->GetPss();
    }
    time_t timeInterval = newTime - newAS_.lastTime;
    double totalMem = static_cast<double>(lastSR.totalMem);
    if (!CalcEwma(timeInterval, totalMem, HALF_LIFE_LEVEL, HALF_LIFE_VAR, HALF_LIFE_SLOPE)) {
        return false;
    }
    double stdDeviation = sqrt(newAS_.lastVar);
    zScore_ = stdDeviation > 0.0 ? (totalMem - newAS_.lastLevel) / stdDeviation : 0.0;
    if (newAS_.lastSlope != 0 && newAS_.lastSlope >= SLOPE_ALERT) {
        newAS_.aboveSlopeTime = newAS_.aboveSlopeTime == 0 ? newAS_.aboveSlopeTime : newTime;
    } else {
        newAS_.aboveSlopeTime = 0;
    }
    persisted_ = newAS_.aboveSlopeTime == 0 ? false : (newTime - newAS_.aboveSlopeTime) >= SLOPE_PERSIST_TIME;
    newAS_.lastTotalMem = static_cast<int>(lastSR.totalMem);
    newAS_.lastTime = newTime;
    ReportMemGrowthLeak(nlInfo);
    return true;
}

bool NativeLeakGrowthAnalyser::CalcEwma(time_t timeInterval, double totalMem, time_t halfLifeLevel, time_t halfLifeVar,
    time_t halfLifeSlope)
{
    if (halfLifeLevel == 0 || halfLifeVar == 0 || halfLifeSlope == 0) {
        HIVIEW_LOGD("halfLife can not be zero");
        return false;
    }
    halfLifeLevel = static_cast<double>(halfLifeLevel);
    halfLifeVar = static_cast<double>(halfLifeVar);
    halfLifeSlope = static_cast<double>(halfLifeSlope);
    double levelParamAlpha = exp(NEGATIVE_NATURAL_LOG_2 * static_cast<double>(timeInterval) / halfLifeLevel);
    if (newAS_.lastLevel == 0) {
        newAS_.lastLevel = totalMem;
    } else {
        newAS_.lastLevel = newAS_.lastLevel * levelParamAlpha + totalMem * (1.0 - levelParamAlpha);
    }
    double varParamAlpha = exp(NEGATIVE_NATURAL_LOG_2 * static_cast<double>(timeInterval) / halfLifeVar);
    newAS_.lastVar = newAS_.lastVar * varParamAlpha + pow(totalMem - newAS_.lastLevel, POW_2) * (1.0 - varParamAlpha);
    double slope = (totalMem - static_cast<double>(newAS_.lastTotalMem)) / static_cast<double>(timeInterval);
    double slopeParamAlpha = exp(NEGATIVE_NATURAL_LOG_2 * static_cast<double>(timeInterval) / halfLifeSlope);
    newAS_.lastSlope = newAS_.lastSlope * slopeParamAlpha + slope * (1.0 - slopeParamAlpha);
    return true;
}

bool NativeLeakGrowthAnalyser::ReportMemGrowthLeak(const shared_ptr<NativeLeakInfo> &nlInfo)
{
    if (newAS_.lastTime - newAS_.firstTime < CALCULATE_TIME) {
        HIVIEW_LOGD("analyze time of process: %{public}s:%{public}d is too short", processName_.c_str(), pid_);
        return false;
    }
    DecideLeakReason();
    if (leakReason_.empty()) {
        HIVIEW_LOGD("process: %{public}s:%{public}d does not leak", processName_.c_str(), pid_);
        return false;
    }
    if (newAS_.lastMemGrowthReportTime == 0 || newAS_.lastTime - newAS_.lastMemGrowthReportTime > REPORT_INTERVAL) {
        newAS_.lastMemGrowthReportTime = newAS_.lastTime;
        DecideLeakName(nlInfo->GetLastSampleRecord());
        HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_GROWTH_LEAK", HiSysEvent::EventType::FAULT,
            "PID", pid_,
            "MODULE", processName_.empty() ? "/" : processName_,
            "VERSION", nlInfo->GetHapVersion().empty() ? "/" : nlInfo->GetHapVersion(),
            "HAPPEN_TIME", nlInfo->GetLastSampleRecord().realTime,
            "LEAK_TYPE", leakName_,
            "LEAK_REASON", leakReason_,
            "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(processName_ + leakName_ + leakReason_, 0),
            "MEM_LEVEL", newAS_.lastLevel,
            "MEM_VAR", newAS_.lastVar,
            "MEM_SLOPE", newAS_.lastSlope,
            "TOTAL_MEM", newAS_.lastTotalMem);
        HIVIEW_LOGI("Report leak of process %{public}s:%{public}d to hiview", processName_.c_str(), pid_);
        return true;
    }
    HIVIEW_LOGD("memory leak of process: %{public}s:%{public}d has been reported", processName_.c_str(), pid_);
    return false;
}

void NativeLeakGrowthAnalyser::DecideLeakReason()
{
    if (persisted_ && zScore_ > Z_SCORE_ALERT) {
        leakReason_ = "PERSIST and VARIANCE";
    } else if (persisted_) {
        leakReason_ = "PERSIST";
    } else if (zScore_ > Z_SCORE_ALERT) {
        leakReason_ = "VARIANCE";
    }
}

void NativeLeakGrowthAnalyser::DecideLeakName(const SampleRecord &newSR)
{
    if (newSR.gpu > max(newSR.ion, newSR.totalPss)) {
        leakName_ = "GPU_LEAK";
    }
    if (newSR.ion > max(newSR.gpu, newSR.totalPss)) {
        leakName_ = "ION_LEAK";
    }
    if (newSR.totalPss > max(newSR.ion, newSR.gpu)) {
        leakName_ = "PSS_MEMORY";
    }
}

} // namespace HiviewDFX
} // namespace OHOS