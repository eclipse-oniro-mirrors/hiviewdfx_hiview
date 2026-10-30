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

#ifndef OLD_LOG_PARAM_CONFIG_H
#define OLD_LOG_PARAM_CONFIG_H

#include <unordered_map>
#include <string>
#include <singleton.h>
#include <cJSON.h>

namespace OHOS {
namespace HiviewDFX {

struct LogFileNums {
    int fileKeepNums {3};
    int fileExceedNums {5};
};

struct FolderOldLog {
    int resourceLeakDirMaxSizeMB {300}; // 默认300MB
    bool enable {false};
};

struct OldLogCategory {
    std::string directCleanType;
    std::unordered_map<std::string, LogFileNums> oldLogStandard;
    FolderOldLog folderOldLog;
    int rawheapBetaAgingHours {168};
    int rawheapNonbetaAgingHours {15};
};

class OldLogParamConfig : public Singleton<OldLogParamConfig> {
    DECLARE_SINGLETON(OldLogParamConfig);
public:
    void ParseConfig(const cJSON *node);
    void Print();
    inline const OldLogCategory &GetOldLogCategory()
    {
        return oldLogCategory_;
    }
private:
    OldLogCategory oldLogCategory_;
private:
    void UpdateOldLogStandard(const cJSON *node);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // OLD_LOG_PARAM_CONFIG_H