/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_DECORATOR_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_DECORATOR_H

#include <map>
#include <string>

#include "collect_result.h"
#include "ffrt.h"
#include "time_util.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
extern const char* const UC_STAT_LOG_PATH;
extern const char* const UC_SEPARATOR;
extern const char* const UC_STAT_DATE;
extern const char* const UC_API_STAT_TITLE;
extern const char* const UC_API_STAT_ITEM;

struct StatInfo {
    std::string name;
    uint32_t totalCall = 0;
    uint32_t failCall = 0;
    uint64_t avgLatency = 0;
    uint64_t maxLatency = 0;
    uint64_t totalTimeSpend = 0;

    std::string ToString() const
    {
        std::string str;
        str.append(name).append(" ")
        .append(std::to_string(totalCall)).append(" ")
        .append(std::to_string(failCall)).append(" ")
        .append(std::to_string(avgLatency)).append(" ")
        .append(std::to_string(maxLatency)).append(" ")
        .append(std::to_string(totalTimeSpend));
        return str;
    }
};

class StatInfoWrapper {
public:
    void UpdateStatInfo(uint64_t startTime, uint64_t endTime, const std::string& funcName, bool isCallSucc);
    std::map<std::string, StatInfo> GetStatInfo();
    void ResetStatInfo();

private:
    std::map<std::string, StatInfo> statInfos_;
    ffrt::mutex mutex_;
};

class UCDecorator {
public:
    UCDecorator() = default;
    virtual ~UCDecorator() = default;
    template <typename T> auto Invoke(T task, StatInfoWrapper& statInfoWrapper,
        const std::string& classFuncName)
    {
        uint64_t startTime = TimeUtil::GenerateTimestamp();
        auto result = task();
        if (!Parameter::IsBetaVersion() && !Parameter::IsUCollectionSwitchOn()) {
            return result;
        }
        uint64_t endTime = TimeUtil::GenerateTimestamp();
        bool isCallSucc;
        if constexpr (std::is_same_v<std::decay_t<decltype(result)>, int>) {
            isCallSucc = (result == UCollect::UcError::SUCCESS);
        } else {
            isCallSucc = (result.retCode == UCollect::UcError::SUCCESS);
        }
        statInfoWrapper.UpdateStatInfo(startTime, endTime, classFuncName, isCallSucc);
        return result;
    }
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_DECORATOR_H
