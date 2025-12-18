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

#ifndef HISYSEVENT_UTIL_H_
#define HISYSEVENT_UTIL_H_
#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace HisysEventUtil {

/**
 * The function to confirm whether the event is stored in the database.
 *
 * @param name the event name.
 * @param key the param name in the event.
 * @param value the value of the param in the event.
 * @return whether the event is stored in the database.
 */
bool IsEventProcessed(const std::string& name, const std::string& key, const std::string& value);

constexpr char KERNEL_VENDOR[] = "KERNEL_VENDOR";
}
}
}
#endif // HISYSEVENT_UTIL_H_
