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
 
#include <parameters.h>
#include "hiview_logger.h"
#include "input_monitor.h"
#include "jank_frame_monitor.h"
#include "perf_reporter.h"
#include "perf_trace.h"
#include "perf_utils.h"
#include "scene_monitor.h"
#include "white_block_monitor.h"

 
namespace OHOS {
namespace HiviewDFX {
 
DEFINE_LOG_LABEL(0xD002D66, "Hiview-PerfMonitor");

static constexpr int DELAY_US = 200000;
 
WhiteBlockMonitor& WhiteBlockMonitor::GetInstance()
{
    static WhiteBlockMonitor instance;
    return instance;
}
 
void WhiteBlockMonitor::StartScroll(const BaseInfo& baseInfo)
{
    if (!IsBetaVersion()) {
        return;
    }
    std::lock_guard<ffrt::mutex> Lock(mMutex);
    scrollStartTime = static_cast<uint64_t>(GetCurrentSystimeMs());
    appWhiteInfo.bundleName = baseInfo.bundleName;
    appWhiteInfo.abilityName = baseInfo.abilityName;
    appWhiteInfo.pageUrl = baseInfo.pageUrl;
    appWhiteInfo.pageName = baseInfo.pageName;
    scrolling = true;
}
 
void WhiteBlockMonitor::EndScroll()
{
    if (!IsBetaVersion()) {
        return;
    }
    XPERF_TRACE_SCOPED("WhiteBlockMonitor::EndScroll");
    {
        std::lock_guard<ffrt::mutex> Lock(mMutex);
        scrollEndTime = static_cast<uint64_t>(GetCurrentSystimeMs());
        scrolling = false;
    }
    ffrt::submit([] { WhiteBlockMonitor::GetInstance().ReportWhiteBlockStat(); },
        ffrt::task_attr().qos(ffrt::qos_user_initiated).delay(DELAY_US));
}
 
void WhiteBlockMonitor::StartRecordImageLoadStat(int64_t id)
{
    if (!IsBetaVersion()) {
        return;
    }
    std::lock_guard<ffrt::mutex> Lock(mMutex);
    if (!scrolling) {
        HIVIEW_LOGD("not scrolling");
        return;
    }
    if (mRecords.find(id) !=  mRecords.end()) {
        HIVIEW_LOGD("record already exists");
        return;
    }
    ImageLoadInfo* record = new ImageLoadInfo();
    if (record == nullptr) {
        HIVIEW_LOGW("create ImageLoadInfo failed");
        return;
    }
    record->id = id;
    record->loadStartTime = static_cast<uint64_t>(GetCurrentSystimeMs());
    mRecords.emplace(id, std::move(record));
}
 
void WhiteBlockMonitor::EndRecordImageLoadStat(int64_t id, std::pair<int, int> size, const std::string& type, int state)
{
    if (!IsBetaVersion()) {
        return;
    }
    std::lock_guard<ffrt::mutex> Lock(mMutex);
    auto it = mRecords.find(id);
    if (it == mRecords.end() || it->second == nullptr) {
        HIVIEW_LOGD("record not exists");
        return;
    }
    it->second->loadEndTime = static_cast<uint64_t>(GetCurrentSystimeMs());
    it->second->imageType = type;
    it->second->width = size.first;
    it->second->height = size.second;
    it->second->loadState = state;
}
 
void WhiteBlockMonitor::ReportWhiteBlockStat()
{
    std::lock_guard<ffrt::mutex> Lock(mMutex);
    PerfReporter::GetInstance().ReportWhiteBlockStat(scrollStartTime, scrollEndTime, mRecords, appWhiteInfo);
    mRecords.clear();
}

bool WhiteBlockMonitor::IsBetaVersion()
{
    static std::string versionType = OHOS::system::GetParameter("const.logsystem.versiontype", "");
    return versionType == "beta";
}

}
}