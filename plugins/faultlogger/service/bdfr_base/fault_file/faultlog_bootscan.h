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
#ifndef FAULTLOG_BOOTSCAN_H
#define FAULTLOG_BOOTSCAN_H

#include "event.h"
#include "faultlog_info_inner.h"
#include "faultlog_manager.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogBootScan {
public:
    static void StartBootScan();
private:
    static bool IsCrashTempBigFile(const std::string& file);
    static bool IsCrashType(const std::string& file);
    static bool IsInValidTime(const std::string& file, const time_t& now);
    static bool IsEmptyStack(const std::string& file, const FaultLogInfo& info);
    static bool IsReported(const FaultLogInfo& info);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
