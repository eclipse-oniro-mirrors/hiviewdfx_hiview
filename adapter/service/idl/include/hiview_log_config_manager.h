/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_LOG_CONFIG_MANAGER_H
#define HIVIEW_LOG_CONFIG_MANAGER_H

#include <mutex>
#include <unordered_map>

#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
struct ConfigInfo {
    ConfigInfo(const std::string& path) : path(path) {};
    std::string path;
    bool isReadOnly { false };
};

class HiviewLogConfigManager : public DelayedRefSingleton<HiviewLogConfigManager> {
public:
    HiviewLogConfigManager() = default;
    ~HiviewLogConfigManager() = default;
    std::shared_ptr<ConfigInfo> GetConfigInfoByType(const std::string& type);

private:
    std::shared_ptr<ConfigInfo> GetLogConfigFromFile(const std::string& type);

private:
    std::unordered_map<std::string, std::shared_ptr<ConfigInfo>> configInfos;
    std::mutex logMutex;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif