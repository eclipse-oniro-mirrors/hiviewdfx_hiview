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
#ifndef EVENTLOGGER_UTIL_TEST_H
#define EVENTLOGGER_UTIL_TEST_H
#include <string>
#include <sys/types.h>

#include <gtest/gtest.h>
namespace OHOS {
namespace HiviewDFX {
constexpr mode_t DEFAULT_MODE = 0644;
void InitSeLinuxEnabled();
void CancelSeLinuxEnabled();
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENTLOGGER_UTIL_TEST_H
