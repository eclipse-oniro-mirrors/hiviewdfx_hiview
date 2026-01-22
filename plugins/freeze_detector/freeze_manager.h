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

#ifndef HIVIEWDFX_FREEZE_MANAGER_H
#define HIVIEWDFX_FREEZE_MANAGER_H

#include <map>
#include "log_store_ex.h"
#include "singleton.h"
#include <shared_mutex>

namespace OHOS {
namespace HiviewDFX {
enum FreezeLogType {
    EVENTLOG = 0,
    FREEZE_DETECTOR,
    FREEZE_EXT
};

class FreezeManager : public DelayedSingleton<FreezeManager>,
    public std::enable_shared_from_this<FreezeManager> {
    DISALLOW_COPY_AND_MOVE(FreezeManager);
public:
    static constexpr const char* const LOGGER_EVENT_LOG_PATH = "/data/log/eventlog";
    static constexpr const char* FREEZE_DETECTOR_PATH = "/data/log/faultlog/freeze/";
    static constexpr uint32_t BUF_SIZE_1024 = 1024;
    FreezeManager();
    ~FreezeManager();
    static FreezeManager &GetInStance();
    void InitLogStore();
    static std::string GetAppFreezeFile(const std::string& stackPath);

    void InsertTraceName(int64_t time, std::string traceName);
    std::string GetTraceName(int64_t time) const;
    std::string SaveFreezeExtInfoToFile(long uid, const std::string& bundleName,
        const std::string& stackFile, const std::string& cpuFile) const;
    int GetFreezeLogFd(int32_t freezeLogType, const std::string& fileName) const;
    void ParseLogEntry(const std::string& input, std::map<std::string, std::string> &sectionMaps);
    void FillProcMemory(const std::string& procStatm, long pid,
        std::map<std::string, std::string> &sectionMaps);
    void ExchangeFdWithFdsanTag(const int fd);
    int CloseFdWithFdsanTag(const int fd);
    int CloseFileByFp(FILE*& fp, std::string path);
    std::string GetlineByFile(std::string path);

private:
    void InitEventLogStore();
    void InitFreezeExtLogStore();
    void InitFreezeDetectorLogStore();
    LogStoreEx::LogFileFilter CreateLogFileFilter(int32_t id, const std::string& filePrefix) const;
    int32_t GetUidFromFileName(const std::string& fileName) const;
    std::vector<std::string> GetDightStrArr(const std::string& target) const;

    std::shared_ptr<LogStoreEx> eventLogStore_ = nullptr;
    std::shared_ptr<LogStoreEx> freezeDetectorLogStore_ = nullptr;
    std::shared_ptr<LogStoreEx> freezeExtLogStore_ = nullptr;

    std::map<int64_t, std::string> traceNameMap_;
    mutable std::shared_mutex traceNameMapMutex_;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // HIVIEWDFX_FREEZE_MANAGER_H