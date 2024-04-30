/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "SceneDataMapDbAdapter.h"

void SceneDataMapDbAdapter::ClearMapByBundleName(const std::string& bundleName)
{
    for (std::map<int32_t, std::string>::iterator it = pidBundleMap.begin(); it != pidBundleMap.end();) {
        if (it->second == bundleName) {
            it = pidBundleMap.erase(it);
        } else {
            it++;
        }
    }
}

void SceneDataMapDbAdapter::SaveAppIdIntoMap(const int32_t& appId, const std::string& bundleName)
{
    if (appId > 0) {
        if (!pidBundleMap.count(appId)) {
            pidBundleMap.insert(std::make_pair(appId, bundleName));
        } else {
            pidBundleMap[appId] = bundleName;
        }
    }
}
