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

#ifndef DFR_APP_KILLED_REPORTER_H
#define DFR_APP_KILLED_REPORTER_H

#include <string>
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
struct LastExitDetailInfo {
    std::string pid;
    std::string uid;
    std::string rss;
    std::string pss;
    std::string processState;
    std::string timestamp;
    std::string processName;
    std::string exitMsg;
    std::string killReason;
};
// AK: short for App Killed
struct AKReporter {
    std::string reason;
    std::string bundleName;
    std::string appRunningUniqueId;
    std::string version;
    std::string callingProcessName;
    LastExitDetailInfo lastExitDetailInfo;
    pid_t uid;
    pid_t callingPid;
    int processKillId;
    bool isForeground;
};

class AppKilledReporter : public Singleton<AppKilledReporter> {
public:
    void Report(const AKReporter &akReporter);
};
} // HiviewDFX
} // OHOS
#endif // DFR_APP_KILLED_REPORTER_H
