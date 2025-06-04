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
 
#ifndef WHITE_BLOCK_MONITOR_H
#define WHITE_BLOCK_MONITOR_H
 
#include <map>
#include <mutex>
#include "perf_constants.h"
#include "perf_model.h"
 
namespace OHOS {
namespace HiviewDFX {
 
class WhiteBlockMonitor {
public:
    static WhiteBlockMonitor& GetInstance();
 
    void StartScroll();
    void EndScroll();
    void StartRecordImageLoadStat(int64_t id);
    void EndRecordImageLoadStat(int64_t id, std::pair<int, int> size, const std::string& type, int state);
 
private:
    bool RecordExist(int64_t id);
    ImageLoadInfo* GetRecord(int64_t id);
    void ReportWhiteBlockStat();
    void CleanUpRecords();
 
private:
    mutable std::mutex mMutex;
    uint64_t scrollStartTime{0};
    uint64_t scrollEndTime{0};
    bool scrolling{false};
    std::map<int64_t, ImageLoadInfo*> mRecords;
};
 
}
}
 
#endif // WHITE_BLOCK_MONITOR_H