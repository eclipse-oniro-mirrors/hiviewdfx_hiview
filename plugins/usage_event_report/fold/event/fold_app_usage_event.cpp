/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include "fold_app_usage_event.h"

#include "hisysevent_c.h"
#include "usage_event_common.h"

namespace {
#define PARAM_STRING(strVal, stringObj) \
    {.name = (strVal), .t = HISYSEVENT_STRING, \
        .v = {.s = const_cast<char*>((stringObj).c_str())}, .arraySize = 0}

#define PARAM_UINT32(strVal) \
    {.name = (strVal), .t = HISYSEVENT_UINT32, \
        .v = {.ui32 = this->paramMap_[(strVal)].GetUint32()}, .arraySize = 0}
}

namespace OHOS {
namespace HiviewDFX {
using namespace FoldAppUsageEventSpace;

FoldAppUsageEvent::FoldAppUsageEvent(const std::string &name, HiSysEvent::EventType type)
    : LoggerEvent(name, type)
{
    InitBaseParam();
#if FOLD_PC_COUNT_DURATION_ENABLE
    this->paramMap_.insert({std::string(KEY_OF_FOLD_KB_VER_USAGE), DEFAULT_UINT32});
    this->paramMap_.insert({std::string(KEY_OF_FOLD_DISPLAY_OUTER_USAGE), DEFAULT_UINT32});
    this->paramMap_.insert({std::string(KEY_OF_FOLD_DISPLAY_COORDINATION_USAGE), DEFAULT_UINT32});
#endif // FOLD_PC_COUNT_DURATION_ENABLE
}

void FoldAppUsageEvent::InitBaseParam()
{
    this->paramMap_ = {
        {std::string(KEY_OF_PACKAGE), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_VERSION), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_FOLD_VER_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_FOLD_HOR_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_FOLD_VER_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_FOLD_HOR_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_FOLD_VER_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_FOLD_HOR_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_FOLD_VER_MIDSCENE_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_FOLD_HOR_MIDSCENE_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_EXPD_VER_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_EXPD_HOR_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_EXPD_VER_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_EXPD_HOR_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_EXPD_VER_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_EXPD_HOR_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_EXPD_VER_MIDSCENE_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_EXPD_HOR_MIDSCENE_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_G_VER_FULL_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_G_HOR_FULL_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_G_VER_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_G_HOR_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_G_VER_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_G_HOR_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_G_VER_MIDSCENE_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_G_HOR_MIDSCENE_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_N_VER_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_N_VER_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_N_VER_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_N_HOR_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_N_HOR_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_N_HOR_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_LM_VER_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_LM_VER_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_LM_VER_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_LM_VER_MIDSCENE_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_LM_HOR_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_LM_HOR_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_LM_HOR_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_LM_HOR_MIDSCENE_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_T_FULL_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_T_SPLIT_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_T_FLOATING_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_USAGE), DEFAULT_UINT32},
        {std::string(KEY_OF_DATE), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_START_NUM), DEFAULT_UINT32}
    };
}

#ifndef FOLD_PC_COUNT_DURATION_ENABLE
void FoldAppUsageEvent::ReportFullEvent()
{
    std::string packageStr = this->paramMap_["PACKAGE"].GetString();
    std::string versionStr = this->paramMap_["VERSION"].GetString();
    std::string dateStr = this->paramMap_["DATE"].GetString();
    HiSysEventParam params[] = {
        PARAM_STRING("PACKAGE", packageStr), PARAM_STRING("VERSION", versionStr),
        PARAM_UINT32("FOLD_V"), PARAM_UINT32("FOLD_H"),
        PARAM_UINT32("EXPD_V"), PARAM_UINT32("EXPD_H"),
        PARAM_UINT32("USAGE"), PARAM_STRING("DATE", dateStr),
        PARAM_UINT32("TOTAL_START_NUM"),
        PARAM_UINT32("EXPD_V_SPLIT"), PARAM_UINT32("EXPD_V_FLOATING"),
        PARAM_UINT32("EXPD_V_MIDSCENE"), PARAM_UINT32("EXPD_H_SPLIT"),
        PARAM_UINT32("EXPD_H_FLOATING"), PARAM_UINT32("EXPD_H_MIDSCENE"),
        PARAM_UINT32("FOLD_V_SPLIT"), PARAM_UINT32("FOLD_V_FLOATING"),
        PARAM_UINT32("FOLD_V_MIDSCENE"), PARAM_UINT32("FOLD_H_SPLIT"),
        PARAM_UINT32("FOLD_H_FLOATING"), PARAM_UINT32("FOLD_H_MIDSCENE"),
        PARAM_UINT32("G_V_FULL"), PARAM_UINT32("G_V_SPLIT"),
        PARAM_UINT32("G_V_FLOATING"), PARAM_UINT32("G_V_MIDSCENE"),
        PARAM_UINT32("G_H_FULL"), PARAM_UINT32("G_H_SPLIT"),
        PARAM_UINT32("G_H_FLOATING"), PARAM_UINT32("G_H_MIDSCENE"),
        PARAM_UINT32("N_V"), PARAM_UINT32("N_V_SPLIT"),
        PARAM_UINT32("N_V_FLOATING"), PARAM_UINT32("N_H"),
        PARAM_UINT32("N_H_SPLIT"), PARAM_UINT32("N_H_FLOATING"),
        PARAM_UINT32("LM_V"), PARAM_UINT32("LM_V_SPLIT"),
        PARAM_UINT32("LM_V_FLOATING"), PARAM_UINT32("LM_V_MIDSCENE"),
        PARAM_UINT32("LM_H"), PARAM_UINT32("LM_H_SPLIT"),
        PARAM_UINT32("LM_H_FLOATING"), PARAM_UINT32("LM_H_MIDSCENE"),
        PARAM_UINT32("T_FULL"), PARAM_UINT32("T_SPLIT"),
        PARAM_UINT32("T_FLOATING"),
    };
    OH_HiSysEvent_Write("HIVIEWDFX", this->eventName_.c_str(), HISYSEVENT_STATISTIC,
        params, sizeof(params) / sizeof(HiSysEventParam));
}
#endif

void FoldAppUsageEvent::Report()
{
#if FOLD_PC_COUNT_DURATION_ENABLE
    std::string packageStr = this->paramMap_["PACKAGE"].GetString();
    std::string versionStr = this->paramMap_["VERSION"].GetString();
    std::string dateStr = this->paramMap_["DATE"].GetString();
    HiSysEventParam params[] = {
        PARAM_STRING("PACKAGE", packageStr),
        PARAM_STRING("VERSION", versionStr),
        PARAM_UINT32("FOLD_V"),
        PARAM_UINT32("EXPD_V"),
        PARAM_UINT32("EXPD_H"),
        PARAM_UINT32("USAGE"),
        PARAM_STRING("DATE", dateStr),
        PARAM_UINT32("TOTAL_START_NUM"),
        PARAM_UINT32("FOLD_KB_V"),
        {.name = "FOLD_DISPLAY_OUTER_V", .t = HISYSEVENT_UINT32,
            .v = {.ui32 = this->paramMap_[std::string("FOLD_H")].GetUint32()}, .arraySize = 0},
        PARAM_UINT32("FOLD_DISPLAY_COORDINATION"),
    };
    OH_HiSysEvent_Write("HIVIEWDFX", this->eventName_.c_str(), HISYSEVENT_STATISTIC,
        params, sizeof(params) / sizeof(HiSysEventParam));
#else
    ReportFullEvent();
#endif // FOLD_PC_COUNT_DURATION_ENABLE
}
} // namespace HiviewDFX
} // namespace OHOS
