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

#ifndef HIRETRIEVAL_MGR_INCLUDE_H
#define HIRETRIEVAL_MGR_INCLUDE_H

#include "hiretrieval_dynamic_loader.h"

#include <memory>
#include <mutex>
#include <string>

namespace OHOS::HiviewDFX {
class HiRetrievalMgr {
public:
    struct Config {
        std::string userType;
        std::string deviceType;
        std::string deviceModel;
    };

    static HiRetrievalMgr& GetInstance();

    int32_t Init();
    int32_t Participate(Config& cfg);
    int32_t Quit();
    bool IsParticipant();
    long long GetLastParticipationTs();
    int32_t Run();
    Config GetCurrentConfig();
    void SetWorkDir(const std::string& dir);

private:
    HiRetrievalMgr();
    ~HiRetrievalMgr() = default;

    std::string GetPreferenceFile();
    void PersistConfig(const Config& cfg);
    void ClearConfigCache();
    void ReadPersistedConfig(Config& cfg);

    std::unique_ptr<HiRetrievalDynamicLoader> dllLoader_;
    std::mutex mgrMutex_;
    std::string workDir_;
};
} // namespace OHOS::HiviewDFX

#endif // HIRETRIEVAL_MGR_INCLUDE_H