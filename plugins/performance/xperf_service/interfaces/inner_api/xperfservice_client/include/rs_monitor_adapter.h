/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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

#ifndef RS_MONITOR_ADAPTER_H
#define RS_MONITOR_ADAPTER_H

#include <string>
#include <vector>

namespace OHOS {
namespace HiviewDFX {
class RsMonitorAdapter {
public:

    static RsMonitorAdapter& GetInstance();

    void VideoStart(const std::vector<uint64_t>& uniqueIdList,
        const std::vector<std::string>& surfaceNameList, const uint32_t fps, const uint64_t reportTime);
    void VideoStop(const std::vector<uint64_t>& uniqueIdList,
        const std::vector<std::string>& surfaceNameList, const uint32_t fps);
    void VideoCollectFinish();
    void VideoCollect(const uint64_t uniqueId, const uint32_t sequence);
    bool VideoGet(uint64_t uniqueId);
    bool VideoGetRecent();

private:
    RsMonitorAdapter();
    ~RsMonitorAdapter();
};
} // namespace HiviewDFX
} // namespace OHOS

#endif