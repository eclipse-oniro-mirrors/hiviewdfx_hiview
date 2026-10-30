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

#ifndef DFR_THIRD_PARTY_CONFIG_H
#define DFR_THIRD_PARTY_CONFIG_H

#include <map>
#include <string>
#include <singleton.h>
#include <cJSON.h>

namespace OHOS {
namespace HiviewDFX {
class ThirdPartyConfig : public Singleton<ThirdPartyConfig> {
    DECLARE_SINGLETON(ThirdPartyConfig);
public:
    void GetThreshold(std::map<std::string, int> &output);
private:
    void UpdatePolicy(const cJSON *node, std::map<std::string, int> &output);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_THIRD_PARTY_CONFIG_H
