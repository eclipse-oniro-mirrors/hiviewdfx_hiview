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

#ifndef PERF_REPORTER_H
#define PERF_REPORTER_H

#include "perf_constants.h"
#include "perf_model.h"

#include "transaction/rs_render_service_client.h"

namespace OHOS {
namespace HiviewDFX {

class PerfReporter {
public:
    static PerfReporter& GetInstance();
    void ReportJankFrameApp(double jank, int32_t jankThreshold);
    void ReportPageShowMsg(const std::string& pageUrl, const std::string& bundleName, const std::string& pageName);
    void ReportAnimatorEvent(PerfEventType type, DataBase& data);
    void ReportSingleJankFrame(JankInfo& jankInfo);
    void ReportStatsJankFrame(int64_t jankFrameRecordBeginTime, int64_t duration,
        const std::vector<uint16_t>& jankFrameRecord, int32_t jankFrameTotalCount, const BaseInfo& baseInfo);
    void ReportWhiteBlockStat(uint64_t scrollStartTime, uint64_t scrollEndTime,
                              const std::map<int64_t, ImageLoadInfo*>& mRecords);
    void ReportSurface(uint64_t uniqueId, const std::string& surfaceName, const std::string& componentName,
        const std::string& bundleName, int32_t pid);

private:
    void ConvertToRsData(OHOS::Rosen::DataBaseRs &dataRs, DataBase& data);
    void FlushDataBase(AnimatorRecord* record, DataBase& data);
    void ReportPerfEventToRS(DataBase& data);
    void ReportPerfEventToUI(DataBase data);
};

class EventReporter {
public:
    static void ReportJankFrameApp(JankInfo& info);
    static void ReportPageShowMsg(const std::string& pageUrl, const std::string& bundleName,
        const std::string& pageName);
    static void ReportEventComplete(DataBase& data);
    static void ReportEventJankFrame(DataBase& data);
    static void ReportJankFrameFiltered(JankInfo& info);
    static void ReportJankFrameUnFiltered(JankInfo& info);
    static void ReportStatsJankFrame(int64_t startTime, int64_t duration, const std::vector<uint16_t>& jank,
        const BaseInfo& baseInfo, uint32_t jankStatusVersion = 1);
#ifdef RESOURCE_SCHEDULE_SERVICE_ENABLE
    static void ReportAppFrameDropToRss(const bool isInteractionJank, const std::string &bundleName,
        const int64_t maxFrameTime = 0);
#endif // RESOURCE_SCHEDULE_SERVICE_ENABLE

    static void ReportImageLoadStat(const ImageLoadStat& stat);
    static void ReportSurfaceInfo(const SurfaceInfo& surface);
};
}
}

#endif