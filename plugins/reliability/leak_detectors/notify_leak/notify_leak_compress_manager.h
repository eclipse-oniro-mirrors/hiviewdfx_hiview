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
#ifndef DFR_NOTIFY_LEAK_COMPRESS_MANAGER_H
#define DFR_NOTIFY_LEAK_COMPRESS_MANAGER_H

#include "singleton.h"

#include <deque>
#include <string>
#include <ffrt.h>

namespace OHOS {
namespace HiviewDFX {

class NotifyLeakCompressManager : public Singleton<NotifyLeakCompressManager> {
    DECLARE_SINGLETON(NotifyLeakCompressManager);
public:
    bool CommonCompress(uint64_t faultType, const std::string &bundleName);
    bool CouldCompress();
    void UpdateCompressHistory(const std::string &bundleName);
    int GetCommonCompressRemainingQuota(const std::string &bundleName);

private:
    void InitFile();
    std::deque<std::pair<time_t, std::string>> ReadCompressHistory();
    bool HaveEnoughQuota(const std::string &bundleName);
    void WriteDefaultContend(time_t defaultTime, const std::string &bundleName);
    void OverWriteData(const std::deque<std::pair<time_t, std::string>> &history);

private:
   ffrt::mutex compressHistroyMtx_;
};

}
}
#endif
