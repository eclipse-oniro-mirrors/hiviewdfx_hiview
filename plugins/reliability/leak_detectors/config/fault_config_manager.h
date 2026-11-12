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
#ifndef DFR_FAULT_CONFIG_MANAGER_H
#define DFR_FAULT_CONFIG_MANAGER_H

#include <string>
#include <cJSON.h>
#include <singleton.h>
#include "leak_json_parser.h"

namespace OHOS {
namespace HiviewDFX {

class FaultConfigManager : public Singleton<FaultConfigManager> {
    DECLARE_SINGLETON(FaultConfigManager);
public:
    void ParseConfigFile();

private:
    void ParseConfigByVersion(const cJSON *root, const std::string &version);
    void ParseQuota(const cJSON *node);
    const cJSON *GetQuotaNode(const cJSON *node);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FAULT_CONFIG_MANAGER_H
