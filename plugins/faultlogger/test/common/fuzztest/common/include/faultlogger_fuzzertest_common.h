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

#ifndef FAULTLOGGER_FUZZERTEST_COMMON_H
#define FAULTLOGGER_FUZZERTEST_COMMON_H

#include <iostream>
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {

const int FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH = 50;

#define STREAM_TO_VALUEINFO(data, value) \
    do { \
        errno_t err = memcpy_s(&(value), sizeof(value), (data), sizeof(value)); \
        if (err != 0) { \
            std::cout << "memcpy_s return value is abnormal!" << std::endl; \
        } \
        (data) += sizeof(value); \
    } while (0)
}
}
#endif
