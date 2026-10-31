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

#ifndef DFR_THREAD_LEAK_CONFIG_H
#define DFR_THREAD_LEAK_CONFIG_H
#include <cJSON.h>
#include <singleton.h>

namespace OHOS {
namespace HiviewDFX {
class ThreadLeakConfig : public Singleton<ThreadLeakConfig> {
    DECLARE_SINGLETON(ThreadLeakConfig);
public:
    void UpdateValue(const cJSON *node);
    inline bool GetEnable()
    {
        return enable_;
    }
    inline bool GetKillEnable()
    {
        return killEnable_;
    }
    inline int GetThreshold()
    {
        return threshold_;
    }
    inline int GetSmallThreshold()
    {
        return smallThreshold_;
    }
    inline int GetUploadInterval() // no usage
    {
        return uploadInterval_;
    }
    inline int GetHungtaskThreadCheck()
    {
        return hungtaskThreadCheck_;
    }
    inline int GetProfilerTime()
    {
        return profilerTime_;
    }
    inline bool GetAsyncStackEnable()
    {
        return asyncStackEnable_;
    }
    void Print();
private:
    bool enable_ { false };
    bool killEnable_ { false };
    int threshold_ { 0 };
    int smallThreshold_ { 0 };
    int uploadInterval_ { 0 }; // no usage
    int hungtaskThreadCheck_ { 0 };
    int profilerTime_ { 0 };
    bool asyncStackEnable_ { false };
private:
    void UpdateThreshold(const cJSON *node);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // DFR_THREAD_LEAK_CONFIG_H
