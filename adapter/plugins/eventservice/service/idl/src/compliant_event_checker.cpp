/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "compliant_event_checker.h"

#include <list>
#include <unordered_map>

#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int64_t SECURE_ENABALED_VAL = 1;

std::unordered_map<std::string, std::list<std::string>> COMPLIANT_EVENT_CONFIGS {
    {"AAFWK", {"ABILITY_ONACTIVE", "ABILITY_ONBACKGROUND", "ABILITY_ONFOREGROUND",
        "ABILITY_ONINACTIVE", "JS_ERROR", "START_ABILITY", "TERMINATE_ABILITY"}},
    {"ACE", {"INTERACTION_COMPLETED_LATENCY", "JS_ERROR"}},
    {"AV_CODEC", {"CODEC_START_INFO"}},
    {"GRAPHIC", {"INTERACTION_COMPLETED_LATENCY", "INTERACTION_RESPONSE_LATENCY"}},
    {"LOCATION", {"GNSS_STATE"}},
    {"PERFORMANCE", {}},
    {"RELIABILITY", {}},
    {"SANITIZER", {"ADDR_SANITIZER"}},
    {"WORKSCHEDULER", {"WORK_ADD", "WORK_REMOVE", "WORK_START", "WORK_STOP"}},
};
}

bool CompliantEventChecker::IsCompliantEvent(const std::string& domain, const std::string& eventName)
{
    static bool isSecureEnabeled = (Parameter::GetInteger("const.secure", SECURE_ENABALED_VAL) == SECURE_ENABALED_VAL);
    if (!isSecureEnabeled) {
        return true;
    }
    auto iter = COMPLIANT_EVENT_CONFIGS.find(domain);
    if (iter == COMPLIANT_EVENT_CONFIGS.end()) {
        return false;
    }
    if (iter->second.empty()) {
        return true;
    }
    return std::find(iter->second.begin(), iter->second.end(), eventName) != iter->second.end();
}
} // namespace HiviewDFX
} // namespace OHOS