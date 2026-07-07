/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_USAGE_EVENT_COMMON_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_USAGE_EVENT_COMMON_H

#include <string>

#include "hiview_event_common.h"

namespace OHOS {
namespace HiviewDFX {
inline constexpr size_t MAX_APP_USAGE_SIZE = 100;

#define KEY_OF_PACKAGE_LITERAL "PACKAGE"
#define KEY_OF_VERSION_LITERAL "VERSION"
#define KEY_OF_USAGE_LITERAL "USAGE"
#define KEY_OF_DATE_LITERAL "DATE"
#define KEY_OF_START_NUM_LITERAL "TOTAL_START_NUM"
#define KEY_OF_START_LITERAL "START"
#define KEY_OF_END_LITERAL "END"
#define KEY_OF_POWER_LITERAL "POWER"
#define KEY_OF_RUNNING_LITERAL "RUNNING"

namespace AppUsageEventSpace {
inline constexpr char EVENT_NAME[] = "APP_USAGE";
inline constexpr char KEY_OF_PACKAGE[] = KEY_OF_PACKAGE_LITERAL;
inline constexpr char KEY_OF_VERSION[] = KEY_OF_VERSION_LITERAL;
inline constexpr char KEY_OF_USAGE[] = KEY_OF_USAGE_LITERAL;
inline constexpr char KEY_OF_DATE[] = KEY_OF_DATE_LITERAL;
inline constexpr char KEY_OF_START_NUM[] = KEY_OF_START_NUM_LITERAL;
}

namespace FoldAppUsageEventSpace {
inline constexpr char EVENT_NAME[] = "FOLD_APP_USAGE";
inline constexpr char KEY_OF_PACKAGE[] = KEY_OF_PACKAGE_LITERAL;
inline constexpr char KEY_OF_VERSION[] = KEY_OF_VERSION_LITERAL;
inline constexpr char KEY_OF_USAGE[] = KEY_OF_USAGE_LITERAL;
#if FOLD_PC_COUNT_DURATION_ENABLE
inline constexpr char KEY_OF_FOLD_KB_VER_USAGE[] = "FOLD_KB_V";
inline constexpr char KEY_OF_FOLD_DISPLAY_OUTER_USAGE[] = "FOLD_DISPLAY_OUTER_V";
inline constexpr char KEY_OF_FOLD_DISPLAY_COORDINATION_USAGE[] = "FOLD_DISPLAY_COORDINATION";
#endif // FOLD_PC_COUNT_DURATION_ENABLE
inline constexpr char KEY_OF_FOLD_VER_USAGE[] = "FOLD_V";
inline constexpr char KEY_OF_FOLD_VER_SPLIT_USAGE[] = "FOLD_V_SPLIT";
inline constexpr char KEY_OF_FOLD_VER_FLOATING_USAGE[] = "FOLD_V_FLOATING";
inline constexpr char KEY_OF_FOLD_VER_MIDSCENE_USAGE[] = "FOLD_V_MIDSCENE";
inline constexpr char KEY_OF_FOLD_HOR_USAGE[] = "FOLD_H";
inline constexpr char KEY_OF_FOLD_HOR_SPLIT_USAGE[] = "FOLD_H_SPLIT";
inline constexpr char KEY_OF_FOLD_HOR_FLOATING_USAGE[] = "FOLD_H_FLOATING";
inline constexpr char KEY_OF_FOLD_HOR_MIDSCENE_USAGE[] = "FOLD_H_MIDSCENE";
inline constexpr char KEY_OF_EXPD_VER_USAGE[] = "EXPD_V";
inline constexpr char KEY_OF_EXPD_VER_SPLIT_USAGE[] = "EXPD_V_SPLIT";
inline constexpr char KEY_OF_EXPD_VER_FLOATING_USAGE[] = "EXPD_V_FLOATING";
inline constexpr char KEY_OF_EXPD_VER_MIDSCENE_USAGE[] = "EXPD_V_MIDSCENE";
inline constexpr char KEY_OF_EXPD_HOR_USAGE[] = "EXPD_H";
inline constexpr char KEY_OF_EXPD_HOR_SPLIT_USAGE[] = "EXPD_H_SPLIT";
inline constexpr char KEY_OF_EXPD_HOR_FLOATING_USAGE[] = "EXPD_H_FLOATING";
inline constexpr char KEY_OF_EXPD_HOR_MIDSCENE_USAGE[] = "EXPD_H_MIDSCENE";
inline constexpr char KEY_OF_G_VER_FULL_USAGE[] = "G_V_FULL";
inline constexpr char KEY_OF_G_VER_SPLIT_USAGE[] = "G_V_SPLIT";
inline constexpr char KEY_OF_G_VER_FLOATING_USAGE[] = "G_V_FLOATING";
inline constexpr char KEY_OF_G_VER_MIDSCENE_USAGE[] = "G_V_MIDSCENE";
inline constexpr char KEY_OF_G_HOR_FULL_USAGE[] = "G_H_FULL";
inline constexpr char KEY_OF_G_HOR_SPLIT_USAGE[] = "G_H_SPLIT";
inline constexpr char KEY_OF_G_HOR_FLOATING_USAGE[] = "G_H_FLOATING";
inline constexpr char KEY_OF_G_HOR_MIDSCENE_USAGE[] = "G_H_MIDSCENE";
inline constexpr char KEY_OF_N_VER_USAGE[] = "N_V";
inline constexpr char KEY_OF_N_VER_SPLIT_USAGE[] = "N_V_SPLIT";
inline constexpr char KEY_OF_N_VER_FLOATING_USAGE[] = "N_V_FLOATING";
inline constexpr char KEY_OF_N_HOR_USAGE[] = "N_H";
inline constexpr char KEY_OF_N_HOR_SPLIT_USAGE[] = "N_H_SPLIT";
inline constexpr char KEY_OF_N_HOR_FLOATING_USAGE[] = "N_H_FLOATING";
inline constexpr char KEY_OF_LM_VER_USAGE[] = "LM_V";
inline constexpr char KEY_OF_LM_VER_SPLIT_USAGE[] = "LM_V_SPLIT";
inline constexpr char KEY_OF_LM_VER_FLOATING_USAGE[] = "LM_V_FLOATING";
inline constexpr char KEY_OF_LM_VER_MIDSCENE_USAGE[] = "LM_V_MIDSCENE";
inline constexpr char KEY_OF_LM_HOR_USAGE[] = "LM_H";
inline constexpr char KEY_OF_LM_HOR_SPLIT_USAGE[] = "LM_H_SPLIT";
inline constexpr char KEY_OF_LM_HOR_FLOATING_USAGE[] = "LM_H_FLOATING";
inline constexpr char KEY_OF_LM_HOR_MIDSCENE_USAGE[] = "LM_H_MIDSCENE";
inline constexpr char KEY_OF_T_FULL_USAGE[] = "T_FULL";
inline constexpr char KEY_OF_T_SPLIT_USAGE[] = "T_SPLIT";
inline constexpr char KEY_OF_T_FLOATING_USAGE[] = "T_FLOATING";
inline constexpr char KEY_OF_DATE[] = KEY_OF_DATE_LITERAL;
inline constexpr char KEY_OF_START_NUM[] = KEY_OF_START_NUM_LITERAL;
inline constexpr char SCENEBOARD_BUNDLE_NAME[] = "com.ohos.sceneboard"; // NOT include sceneboard
}

namespace SysUsageEventSpace {
inline constexpr char EVENT_NAME[] = "SYS_USAGE";
inline constexpr char KEY_OF_START[] = KEY_OF_START_LITERAL;
inline constexpr char KEY_OF_END[] = KEY_OF_END_LITERAL;
inline constexpr char KEY_OF_POWER[] = KEY_OF_POWER_LITERAL;
inline constexpr char KEY_OF_RUNNING[] = KEY_OF_RUNNING_LITERAL;
}

namespace SysUsageDbSpace {
inline constexpr char SYS_USAGE_TABLE[] = "sys_usage";
inline constexpr char LAST_SYS_USAGE_TABLE[] = "last_sys_usage";
}
namespace DomainSpace {
inline constexpr char HIVIEWDFX_UE_DOMAIN[] = "HIVIEWDFX_UE";
}

namespace FoldEventId {
inline constexpr int EVENT_APP_START = 1101;
inline constexpr int EVENT_APP_EXIT = 1102;
inline constexpr int EVENT_SCREEN_STATUS_CHANGED = 1103;
inline constexpr int EVENT_COUNT_DURATION = 1104;
#if FOLD_PC_COUNT_DURATION_ENABLE
inline constexpr int EVENT_ENTER_COORDINATION_MODE = 1105;
inline constexpr int EVENT_EXIT_COORDINATION_MODE = 1106;
inline constexpr int EVENT_COUNT_COORDINATION_DURATION = 1107;
#endif // FOLD_PC_COUNT_DURATION_ENABLE
}

namespace AppEventSpace {
inline constexpr char FOCUS_WINDOW[] = "FOCUS_WINDOW";
inline constexpr char KEY_OF_BUNDLE_NAME[] = "BUNDLE_NAME";
inline constexpr char KEY_OF_WINDOW_TYPE[] = "WINDOW_TYPE";
}

namespace FoldStateChangeEventSpace {
#if FOLD_PC_COUNT_DURATION_ENABLE
inline constexpr char EVENT_NAME[] = "NOTIFY_FOLD_STATE_CHANGE";
inline constexpr char KEY_OF_NEXT_STATUS[] = "NEXT_FOLD_STATUS";
#else
inline constexpr char EVENT_NAME[] = "DISPLAY_MODE";
inline constexpr char KEY_OF_NEXT_STATUS[] = "FOLD_DISPLAY_MODE";
inline constexpr int32_t DISPLAY_MODE_EXPAND = 1;
inline constexpr int32_t DISPLAY_MODE_FOLD = 2;
inline constexpr int32_t DISPLAY_MODE_G = 5;
inline constexpr int32_t DISPLAY_MODE_N = 7;
inline constexpr int32_t DISPLAY_MODE_LM = 8;
#endif
}

#if FOLD_PC_COUNT_DURATION_ENABLE
namespace FoldDisplayModeChangeEventSpace {
inline constexpr char EVENT_NAME[] = "DISPLAY_MODE";
inline constexpr char KEY_OF_NEXT_STATUS[] = "FOLD_DISPLAY_MODE";
}

namespace SuperFoldState {
inline constexpr int32_t UNKNOWN = 0;
inline constexpr int32_t FOLD_STATE_FOLDED = 1;
inline constexpr int32_t FOLD_STATE_HALF_FOLDED = 2;
inline constexpr int32_t FOLD_STATE_EXPANDED = 3;
inline constexpr int32_t FOLD_STATE_KEYBOARD = 4;
}
#endif // FOLD_PC_COUNT_DURATION_ENABLE

namespace VhModeChangeEventSpace {
inline constexpr char EVENT_NAME[] = "VH_MODE";
inline constexpr char KEY_OF_MODE[] = "MODE";
inline constexpr int32_t VH_MODE_PORTRAIT = 0;
inline constexpr int32_t VH_MODE_LANDSCAPE = 1;
}

namespace FoldTentModeEventSpace {
inline constexpr char EVENT_NAME[] = "FOLD_TENT_MODE";
inline constexpr char KEY_OF_TENT_STATUS[] = "FOLD_TENT_STATUS";
inline constexpr int32_t TENT_MODE = 1;
}

namespace MultiWindowChangeEventSpace {
inline constexpr char EVENT_NAME[] = "MULTI_WINDOW_NUMBER";
inline constexpr char KEY_OF_MULTI_NUM[] = "MULTI_NUM";
inline constexpr char KEY_OF_MULTI_WINDOW[] = "MULTI_WINDOW";
}

namespace MultiWindowMode {
inline constexpr int32_t WINDOW_MODE_FULL = 0;
inline constexpr int32_t WINDOW_MODE_FLOATING = 1;
inline constexpr int32_t WINDOW_MODE_SPLIT_PRIMARY = 2;
inline constexpr int32_t WINDOW_MODE_SPLIT_SECONDARY = 3;
inline constexpr int32_t WINDOW_MODE_MIDSCENE = 4;
}

namespace FoldStatusBase {
inline constexpr int8_t EXPAND = 1;
inline constexpr int8_t FOLD = 2;
inline constexpr int8_t G = 3;
inline constexpr int8_t N = 5;
inline constexpr int8_t LM = 6;
inline constexpr int8_t TENT = 7;
inline constexpr int8_t LANDSCAPE = 1;
inline constexpr int8_t PORTRAIT = 2;
inline constexpr int8_t FULL = 0;
inline constexpr int8_t SPLIT = 1;
inline constexpr int8_t FLOATING = 2;
inline constexpr int8_t MIDSCENE = 3;
}

namespace ScreenFoldStatus {
using namespace FoldStatusBase;
inline constexpr int EXPAND_LANDSCAPE_FULL_STATUS = EXPAND * 100 + LANDSCAPE * 10 + FULL;
inline constexpr int EXPAND_LANDSCAPE_SPLIT_STATUS = EXPAND * 100 + LANDSCAPE * 10 + SPLIT;
inline constexpr int EXPAND_LANDSCAPE_FLOATING_STATUS = EXPAND * 100 + LANDSCAPE * 10 + FLOATING;
inline constexpr int EXPAND_LANDSCAPE_MIDSCENE_STATUS = EXPAND * 100 + LANDSCAPE * 10 + MIDSCENE;
inline constexpr int EXPAND_PORTRAIT_FULL_STATUS = EXPAND * 100 + PORTRAIT * 10 + FULL;
inline constexpr int EXPAND_PORTRAIT_SPLIT_STATUS = EXPAND * 100 + PORTRAIT * 10 + SPLIT;
inline constexpr int EXPAND_PORTRAIT_FLOATING_STATUS = EXPAND * 100 + PORTRAIT * 10 + FLOATING;
inline constexpr int EXPAND_PORTRAIT_MIDSCENE_STATUS = EXPAND * 100 + PORTRAIT * 10 + MIDSCENE;
inline constexpr int FOLD_LANDSCAPE_FULL_STATUS = FOLD * 100 + LANDSCAPE * 10 + FULL;
inline constexpr int FOLD_LANDSCAPE_SPLIT_STATUS = FOLD * 100 + LANDSCAPE * 10 + SPLIT;
inline constexpr int FOLD_LANDSCAPE_FLOATING_STATUS = FOLD * 100 + LANDSCAPE * 10 + FLOATING;
inline constexpr int FOLD_LANDSCAPE_MIDSCENE_STATUS = FOLD * 100 + LANDSCAPE * 10 + MIDSCENE;
inline constexpr int FOLD_PORTRAIT_FULL_STATUS = FOLD * 100 + PORTRAIT * 10 + FULL;
inline constexpr int FOLD_PORTRAIT_SPLIT_STATUS = FOLD * 100 + PORTRAIT * 10 + SPLIT;
inline constexpr int FOLD_PORTRAIT_FLOATING_STATUS = FOLD * 100 + PORTRAIT * 10 + FLOATING;
inline constexpr int FOLD_PORTRAIT_MIDSCENE_STATUS = FOLD * 100 + PORTRAIT * 10 + MIDSCENE;
inline constexpr int G_LANDSCAPE_FULL_STATUS = G * 100 + LANDSCAPE * 10 + FULL;
inline constexpr int G_LANDSCAPE_SPLIT_STATUS = G * 100 + LANDSCAPE * 10 + SPLIT;
inline constexpr int G_LANDSCAPE_FLOATING_STATUS = G * 100 + LANDSCAPE * 10 + FLOATING;
inline constexpr int G_LANDSCAPE_MIDSCENE_STATUS = G * 100 + LANDSCAPE * 10 + MIDSCENE;
inline constexpr int G_PORTRAIT_FULL_STATUS = G * 100 + PORTRAIT * 10 + FULL;
inline constexpr int G_PORTRAIT_SPLIT_STATUS = G * 100 + PORTRAIT * 10 + SPLIT;
inline constexpr int G_PORTRAIT_FLOATING_STATUS = G * 100 + PORTRAIT * 10 + FLOATING;
inline constexpr int G_PORTRAIT_MIDSCENE_STATUS = G * 100 + PORTRAIT * 10 + MIDSCENE;
inline constexpr int N_LANDSCAPE_FULL_STATUS = N * 100 + LANDSCAPE * 10 + FULL;
inline constexpr int N_LANDSCAPE_SPLIT_STATUS = N * 100 + LANDSCAPE * 10 + SPLIT;
inline constexpr int N_LANDSCAPE_FLOATING_STATUS = N * 100 + LANDSCAPE * 10 + FLOATING;
inline constexpr int N_PORTRAIT_FULL_STATUS = N * 100 + PORTRAIT * 10 + FULL;
inline constexpr int N_PORTRAIT_SPLIT_STATUS = N * 100 + PORTRAIT * 10 + SPLIT;
inline constexpr int N_PORTRAIT_FLOATING_STATUS = N * 100 + PORTRAIT * 10 + FLOATING;
inline constexpr int LM_LANDSCAPE_FULL_STATUS = LM * 100 + LANDSCAPE * 10 + FULL;
inline constexpr int LM_LANDSCAPE_SPLIT_STATUS = LM * 100 + LANDSCAPE * 10 + SPLIT;
inline constexpr int LM_LANDSCAPE_FLOATING_STATUS = LM * 100 + LANDSCAPE * 10 + FLOATING;
inline constexpr int LM_LANDSCAPE_MIDSCENE_STATUS = LM * 100 + LANDSCAPE * 10 + MIDSCENE;
inline constexpr int LM_PORTRAIT_FULL_STATUS = LM * 100 + PORTRAIT * 10 + FULL;
inline constexpr int LM_PORTRAIT_SPLIT_STATUS = LM * 100 + PORTRAIT * 10 + SPLIT;
inline constexpr int LM_PORTRAIT_FLOATING_STATUS = LM * 100 + PORTRAIT * 10 + FLOATING;
inline constexpr int LM_PORTRAIT_MIDSCENE_STATUS = LM * 100 + PORTRAIT * 10 + MIDSCENE;
inline constexpr int T_LANDSCAPE_FULL_STATUS = TENT * 100 + LANDSCAPE * 10 + FULL;
inline constexpr int T_LANDSCAPE_SPLIT_STATUS = TENT * 100 + LANDSCAPE * 10 + SPLIT;
inline constexpr int T_LANDSCAPE_FLOATING_STATUS = TENT * 100 + LANDSCAPE * 10 + FLOATING;
#if FOLD_PC_COUNT_DURATION_ENABLE
inline constexpr int FOLD_KB_PORTRAIT_STATUS = 420;
inline constexpr int FOLD_DISPLAY_MODE_COORDINATION_STATUS = 4;
#endif // FOLD_PC_COUNT_DURATION_ENABLE
}

namespace FoldEventTable {
inline constexpr char FIELD_ID[] = "id";
inline constexpr char FIELD_UID[] = "uid";
inline constexpr char FIELD_EVENT_ID[] = "rawid";
inline constexpr char FIELD_TS[] = "ts";
inline constexpr char FIELD_FOLD_STATUS[] = "fold_status";
inline constexpr char FIELD_PRE_FOLD_STATUS[] = "pre_fold_status";
#if FOLD_PC_COUNT_DURATION_ENABLE
inline constexpr char FIELD_DISPLAY_MODE[] = "fold_display_mode";
inline constexpr char FIELD_PRE_DISPLAY_MODE[] = "pre_fold_display_mode";
#endif // FOLD_PC_COUNT_DURATION_ENABLE
inline constexpr char FIELD_VERSION_NAME[] = "version_name";
inline constexpr char FIELD_HAPPEN_TIME[] = "happen_time";
inline constexpr char FIELD_FOLD_PORTRAIT_DURATION[] = "fold_portrait_duration";
#if FOLD_PC_COUNT_DURATION_ENABLE
inline constexpr char FIELD_FOLD_KB_PORTRAIT_DURATION[] = "fold_kb_portrait_duration";
inline constexpr char FIELD_FOLD_DISPLAY_COORDINATION_DURATION[] = "fold_display_coordination_duration";
#endif // FOLD_PC_COUNT_DURATION_ENABLE
inline constexpr char FIELD_FOLD_PORTRAIT_SPLIT_DURATION[] = "fold_portrait_split_duration";
inline constexpr char FIELD_FOLD_PORTRAIT_FLOATING_DURATION[] = "fold_portrait_floating_duration";
inline constexpr char FIELD_FOLD_PORTRAIT_MIDSCENE_DURATION[] = "fold_portrait_midscene_duration";
inline constexpr char FIELD_FOLD_LANDSCAPE_DURATION[] = "fold_landscape_duration";
inline constexpr char FIELD_FOLD_LANDSCAPE_SPLIT_DURATION[] = "fold_landscape_split_duration";
inline constexpr char FIELD_FOLD_LANDSCAPE_FLOATING_DURATION[] = "fold_landscape_floating_duration";
inline constexpr char FIELD_FOLD_LANDSCAPE_MIDSCENE_DURATION[] = "fold_landscape_midscene_duration";
inline constexpr char FIELD_EXPAND_PORTRAIT_DURATION[] = "expand_portrait_duration";
inline constexpr char FIELD_EXPAND_PORTRAIT_SPLIT_DURATION[] = "expand_portrait_split_duration";
inline constexpr char FIELD_EXPAND_PORTRAIT_FLOATING_DURATION[] = "expand_portrait_floating_duration";
inline constexpr char FIELD_EXPAND_PORTRAIT_MIDSCENE_DURATION[] = "expand_portrait_midscene_duration";
inline constexpr char FIELD_EXPAND_LANDSCAPE_DURATION[] = "expand_landscape_duration";
inline constexpr char FIELD_EXPAND_LANDSCAPE_SPLIT_DURATION[] = "expand_landscape_split_duration";
inline constexpr char FIELD_EXPAND_LANDSCAPE_FLOATING_DURATION[] = "expand_landscape_floating_duration";
inline constexpr char FIELD_EXPAND_LANDSCAPE_MIDSCENE_DURATION[] = "expand_landscape_midscene_duration";
inline constexpr char FIELD_G_PORTRAIT_FULL_DURATION[] = "g_portrait_full_duration";
inline constexpr char FIELD_G_PORTRAIT_SPLIT_DURATION[] = "g_portrait_split_duration";
inline constexpr char FIELD_G_PORTRAIT_FLOATING_DURATION[] = "g_portrait_floating_duration";
inline constexpr char FIELD_G_PORTRAIT_MIDSCENE_DURATION[] = "g_portrait_midscene_duration";
inline constexpr char FIELD_G_LANDSCAPE_FULL_DURATION[] = "g_landscape_full_duration";
inline constexpr char FIELD_G_LANDSCAPE_SPLIT_DURATION[] = "g_landscape_split_duration";
inline constexpr char FIELD_G_LANDSCAPE_FLOATING_DURATION[] = "g_landscape_floating_duration";
inline constexpr char FIELD_G_LANDSCAPE_MIDSCENE_DURATION[] = "g_landscape_midscene_duration";
inline constexpr char FIELD_N_PORTRAIT_DURATION[] = "n_portrait_duration";
inline constexpr char FIELD_N_PORTRAIT_SPLIT_DURATION[] = "n_portrait_split_duration";
inline constexpr char FIELD_N_PORTRAIT_FLOATING_DURATION[] = "n_portrait_floating_duration";
inline constexpr char FIELD_N_LANDSCAPE_DURATION[] = "n_landscape_duration";
inline constexpr char FIELD_N_LANDSCAPE_SPLIT_DURATION[] = "n_landscape_split_duration";
inline constexpr char FIELD_N_LANDSCAPE_FLOATING_DURATION[] = "n_landscape_floating_duration";
inline constexpr char FIELD_LM_PORTRAIT_DURATION[] = "lm_portrait_duration";
inline constexpr char FIELD_LM_PORTRAIT_SPLIT_DURATION[] = "lm_portrait_split_duration";
inline constexpr char FIELD_LM_PORTRAIT_FLOATING_DURATION[] = "lm_portrait_floating_duration";
inline constexpr char FIELD_LM_PORTRAIT_MIDSCENE_DURATION[] = "lm_portrait_midscene_duration";
inline constexpr char FIELD_LM_LANDSCAPE_DURATION[] = "lm_landscape_duration";
inline constexpr char FIELD_LM_LANDSCAPE_SPLIT_DURATION[] = "lm_landscape_split_duration";
inline constexpr char FIELD_LM_LANDSCAPE_FLOATING_DURATION[] = "lm_landscape_floating_duration";
inline constexpr char FIELD_LM_LANDSCAPE_MIDSCENE_DURATION[] = "lm_landscape_midscene_duration";
inline constexpr char FIELD_T_LANDSCAPE_DURATION[] = "t_landscape_duration";
inline constexpr char FIELD_T_LANDSCAPE_SPLIT_DURATION[] = "t_landscape_split_duration";
inline constexpr char FIELD_T_LANDSCAPE_FLOATING_DURATION[] = "t_landscape_floating_duration";
inline constexpr char FIELD_BUNDLE_NAME[] = "bundle_name";
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_USAGE_EVENT_COMMON_H
