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

static constexpr int DELAY_MS = 200;
 
WhiteBlockMonitor& WhiteBlockMonitor::GetInstance()
{
    static WhiteBlockMonitor instance;
    return instance;
}
 
void WhiteBlockMonitor::StartScroll()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    scrollStartTime = static_cast<uint64_t>(GetCurrentSystimeMs());
    scrolling = true;
}
 
void WhiteBlockMonitor::EndScroll()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    scrollEndTime = static_cast<uint64_t>(GetCurrentSystimeMs());
    scrolling = false;
    std::thread delayThread([this] { this->ReportWhiteBlockStat(); });
    delayThread.detach();
}
 
void WhiteBlockMonitor::StartRecordImageLoadStat(int64_t id)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    if (!scrolling) {
        HIVIEW_LOGD("not scrolling");
        return;
    }
    if (RecordExist(id)) {
        HIVIEW_LOGD("record already exists");
        return;
    }
    ImageLoadInfo* record = new ImageLoadInfo();
    record->id = id;
    record->loadStartTime = static_cast<uint64_t>(GetCurrentSystimeMs());
    mRecords.emplace(id, record);
}
 
void WhiteBlockMonitor::EndRecordImageLoadStat(int64_t id, std::pair<int, int> size, const std::string& type, int state)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    ImageLoadInfo* record = GetRecord(id);
    if (record == nullptr) {
        HIVIEW_LOGD("record not exists");
        return;
    }
    record->loadEndTime = static_cast<uint64_t>(GetCurrentSystimeMs());
    record->imageType = type;
    record->width = size.first;
    record->height = size.second;
    record->loadState = state;
}
 
bool WhiteBlockMonitor::RecordExist(int64_t id)
{
    return mRecords.count(id);
}
 
ImageLoadInfo* WhiteBlockMonitor::GetRecord(int64_t id)
{
    ImageLoadInfo* record = nullptr;
    auto it = mRecords.find(id);
    if (it != mRecords.end()) {
        record = it->second;
    }
    return record;
}
 
void WhiteBlockMonitor::ReportWhiteBlockStat()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    std::lock_guard<std::mutex> Lock(mMutex);
    PerfReporter::GetInstance().ReportWhiteBlockStat(scrollStartTime, scrollEndTime, mRecords);
    CleanUpRecords();
}
 
void WhiteBlockMonitor::CleanUpRecords()
{
    for (auto iter = mRecords.begin(); iter != mRecords.end();) {
        if (iter->second != nullptr) {
            delete iter->second;
            iter->second = nullptr;
        }
        iter = mRecords.erase(iter);
    }
}
}
}