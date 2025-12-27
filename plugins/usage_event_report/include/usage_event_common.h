/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
constexpr size_t MAX_APP_USAGE_SIZE = 100;

namespace AppUsageEventSpace {
constexpr char EVENT_NAME[] = "APP_USAGE";
constexpr char KEY_OF_PACKAGE[] = "PACKAGE";
constexpr char KEY_OF_VERSION[] = "VERSION";
constexpr char KEY_OF_USAGE[] = "USAGE";
constexpr char KEY_OF_DATE[] = "DATE";
constexpr char KEY_OF_START_NUM[] = "TOTAL_START_NUM";
}

namespace FoldAppUsageEventSpace {
constexpr char EVENT_NAME[] = "FOLD_APP_USAGE";
constexpr char KEY_OF_PACKAGE[] = "PACKAGE";
constexpr char KEY_OF_VERSION[] = "VERSION";
constexpr char KEY_OF_USAGE[] = "USAGE";
constexpr char KEY_OF_FOLD_VER_USAGE[] = "FOLD_V";
constexpr char KEY_OF_FOLD_VER_SPLIT_USAGE[] = "FOLD_V_SPLIT";
constexpr char KEY_OF_FOLD_VER_FLOATING_USAGE[] = "FOLD_V_FLOATING";
constexpr char KEY_OF_FOLD_VER_MIDSCENE_USAGE[] = "FOLD_V_MIDSCENE";
constexpr char KEY_OF_FOLD_HOR_USAGE[] = "FOLD_H";
constexpr char KEY_OF_FOLD_HOR_SPLIT_USAGE[] = "FOLD_H_SPLIT";
constexpr char KEY_OF_FOLD_HOR_FLOATING_USAGE[] = "FOLD_H_FLOATING";
constexpr char KEY_OF_FOLD_HOR_MIDSCENE_USAGE[] = "FOLD_H_MIDSCENE";
constexpr char KEY_OF_EXPD_VER_USAGE[] = "EXPD_V";
constexpr char KEY_OF_EXPD_VER_SPLIT_USAGE[] = "EXPD_V_SPLIT";
constexpr char KEY_OF_EXPD_VER_FLOATING_USAGE[] = "EXPD_V_FLOATING";
constexpr char KEY_OF_EXPD_VER_MIDSCENE_USAGE[] = "EXPD_V_MIDSCENE";
constexpr char KEY_OF_EXPD_HOR_USAGE[] = "EXPD_H";
constexpr char KEY_OF_EXPD_HOR_SPLIT_USAGE[] = "EXPD_H_SPLIT";
constexpr char KEY_OF_EXPD_HOR_FLOATING_USAGE[] = "EXPD_H_FLOATING";
constexpr char KEY_OF_EXPD_HOR_MIDSCENE_USAGE[] = "EXPD_H_MIDSCENE";
constexpr char KEY_OF_G_VER_FULL_USAGE[] = "G_V_FULL";
constexpr char KEY_OF_G_VER_SPLIT_USAGE[] = "G_V_SPLIT";
constexpr char KEY_OF_G_VER_FLOATING_USAGE[] = "G_V_FLOATING";
constexpr char KEY_OF_G_VER_MIDSCENE_USAGE[] = "G_V_MIDSCENE";
constexpr char KEY_OF_G_HOR_FULL_USAGE[] = "G_H_FULL";
constexpr char KEY_OF_G_HOR_SPLIT_USAGE[] = "G_H_SPLIT";
constexpr char KEY_OF_G_HOR_FLOATING_USAGE[] = "G_H_FLOATING";
constexpr char KEY_OF_G_HOR_MIDSCENE_USAGE[] = "G_H_MIDSCENE";
constexpr char KEY_OF_DATE[] = "DATE";
constexpr char KEY_OF_START_NUM[] = "TOTAL_START_NUM";
constexpr char SCENEBOARD_BUNDLE_NAME[] = "com.ohos.sceneboard"; // NOT include sceneboard
}

namespace SysUsageEventSpace {
constexpr char EVENT_NAME[] = "SYS_USAGE";
constexpr char KEY_OF_START[] = "START";
constexpr char KEY_OF_END[] = "END";
constexpr char KEY_OF_POWER[] = "POWER";
constexpr char KEY_OF_RUNNING[] = "RUNNING";
}

namespace SysUsageDbSpace {
constexpr char SYS_USAGE_TABLE[] = "sys_usage";
constexpr char LAST_SYS_USAGE_TABLE[] = "last_sys_usage";
}
namespace DomainSpace {
constexpr char HIVIEWDFX_UE_DOMAIN[] = "HIVIEWDFX_UE";
}

namespace FoldEventId {
constexpr int EVENT_APP_START = 1101;
constexpr int EVENT_APP_EXIT = 1102;
constexpr int EVENT_SCREEN_STATUS_CHANGED = 1103;
constexpr int EVENT_COUNT_DURATION = 1104;
}

namespace AppEventSpace {
constexpr char FOCUS_WINDOW[] = "FOCUS_WINDOW";
constexpr char KEY_OF_BUNDLE_NAME[] = "BUNDLE_NAME";
constexpr char KEY_OF_WINDOW_TYPE[] = "WINDOW_TYPE";
}

namespace FoldStateChangeEventSpace {
constexpr char EVENT_NAME[] = "NOTIFY_FOLD_STATE_CHANGE";
constexpr char KEY_OF_NEXT_STATUS[] = "NEXT_FOLD_STATUS";
}

namespace FoldState {
constexpr int32_t FOLD_STATE_EXPAND = 1;
constexpr int32_t FOLD_STATE_FOLDED = 2;
constexpr int32_t FOLD_STATE_HALF_FOLDED = 3;
constexpr int32_t FOLD_STATE_EXPAND_WITH_SECOND_EXPAND = 11;
constexpr int32_t FOLD_STATE_EXPAND_WITH_SECOND_HALF_FOLDED = 21;
constexpr int32_t FOLD_STATE_FOLDED_WITH_SECOND_EXPAND = 12;
constexpr int32_t FOLD_STATE_FOLDED_WITH_SECOND_HALF_FOLDED = 22;
constexpr int32_t FOLD_STATE_HALF_FOLDED_WITH_SECOND_EXPAND = 13;
constexpr int32_t FOLD_STATE_HALF_FOLDED_WITH_SECOND_HALF_FOLDED = 23;
}

namespace VhModeChangeEventSpace {
constexpr char EVENT_NAME[] = "VH_MODE";
constexpr char KEY_OF_MODE[] = "MODE";
}

namespace MultiWindowChangeEventSpace {
constexpr char EVENT_NAME[] = "MULTI_WINDOW_NUMBER";
constexpr char KEY_OF_MULTI_NUM[] = "MULTI_NUM";
constexpr char KEY_OF_MULTI_WINDOW[] = "MULTI_WINDOW";
}

namespace ScreenFoldStatus {
constexpr int EXPAND_LANDSCAPE_FULL_STATUS = 110;
constexpr int EXPAND_LANDSCAPE_SPLIT_STATUS = 111;
constexpr int EXPAND_LANDSCAPE_FLOATING_STATUS = 112;
constexpr int EXPAND_LANDSCAPE_MIDSCENE_STATUS = 113;
constexpr int EXPAND_PORTRAIT_FULL_STATUS = 120;
constexpr int EXPAND_PORTRAIT_SPLIT_STATUS = 121;
constexpr int EXPAND_PORTRAIT_FLOATING_STATUS = 122;
constexpr int EXPAND_PORTRAIT_MIDSCENE_STATUS = 123;
constexpr int FOLD_LANDSCAPE_FULL_STATUS = 210;
constexpr int FOLD_LANDSCAPE_SPLIT_STATUS = 211;
constexpr int FOLD_LANDSCAPE_FLOATING_STATUS = 212;
constexpr int FOLD_LANDSCAPE_MIDSCENE_STATUS = 213;
constexpr int FOLD_PORTRAIT_FULL_STATUS = 220;
constexpr int FOLD_PORTRAIT_SPLIT_STATUS = 221;
constexpr int FOLD_PORTRAIT_FLOATING_STATUS = 222;
constexpr int FOLD_PORTRAIT_MIDSCENE_STATUS = 223;
constexpr int G_LANDSCAPE_FULL_STATUS = 310;
constexpr int G_LANDSCAPE_SPLIT_STATUS = 311;
constexpr int G_LANDSCAPE_FLOATING_STATUS = 312;
constexpr int G_LANDSCAPE_MIDSCENE_STATUS = 313;
constexpr int G_PORTRAIT_FULL_STATUS = 320;
constexpr int G_PORTRAIT_SPLIT_STATUS = 321;
constexpr int G_PORTRAIT_FLOATING_STATUS = 322;
constexpr int G_PORTRAIT_MIDSCENE_STATUS = 323;
}

namespace FoldEventTable {
constexpr char FIELD_ID[] = "id";
constexpr char FIELD_UID[] = "uid";
constexpr char FIELD_EVENT_ID[] = "rawid";
constexpr char FIELD_TS[] = "ts";
constexpr char FIELD_FOLD_STATUS[] = "fold_status";
constexpr char FIELD_PRE_FOLD_STATUS[] = "pre_fold_status";
constexpr char FIELD_VERSION_NAME[] = "version_name";
constexpr char FIELD_HAPPEN_TIME[] = "happen_time";
constexpr char FIELD_FOLD_PORTRAIT_DURATION[] = "fold_portrait_duration";
constexpr char FIELD_FOLD_PORTRAIT_SPLIT_DURATION[] = "fold_portrait_split_duration";
constexpr char FIELD_FOLD_PORTRAIT_FLOATING_DURATION[] = "fold_portrait_floating_duration";
constexpr char FIELD_FOLD_PORTRAIT_MIDSCENE_DURATION[] = "fold_portrait_midscene_duration";
constexpr char FIELD_FOLD_LANDSCAPE_DURATION[] = "fold_landscape_duration";
constexpr char FIELD_FOLD_LANDSCAPE_SPLIT_DURATION[] = "fold_landscape_split_duration";
constexpr char FIELD_FOLD_LANDSCAPE_FLOATING_DURATION[] = "fold_landscape_floating_duration";
constexpr char FIELD_FOLD_LANDSCAPE_MIDSCENE_DURATION[] = "fold_landscape_midscene_duration";
constexpr char FIELD_EXPAND_PORTRAIT_DURATION[] = "expand_portrait_duration";
constexpr char FIELD_EXPAND_PORTRAIT_SPLIT_DURATION[] = "expand_portrait_split_duration";
constexpr char FIELD_EXPAND_PORTRAIT_FLOATING_DURATION[] = "expand_portrait_floating_duration";
constexpr char FIELD_EXPAND_PORTRAIT_MIDSCENE_DURATION[] = "expand_portrait_midscene_duration";
constexpr char FIELD_EXPAND_LANDSCAPE_DURATION[] = "expand_landscape_duration";
constexpr char FIELD_EXPAND_LANDSCAPE_SPLIT_DURATION[] = "expand_landscape_split_duration";
constexpr char FIELD_EXPAND_LANDSCAPE_FLOATING_DURATION[] = "expand_landscape_floating_duration";
constexpr char FIELD_EXPAND_LANDSCAPE_MIDSCENE_DURATION[] = "expand_landscape_midscene_duration";
constexpr char FIELD_G_PORTRAIT_FULL_DURATION[] = "g_portrait_full_duration";
constexpr char FIELD_G_PORTRAIT_SPLIT_DURATION[] = "g_portrait_split_duration";
constexpr char FIELD_G_PORTRAIT_FLOATING_DURATION[] = "g_portrait_floating_duration";
constexpr char FIELD_G_PORTRAIT_MIDSCENE_DURATION[] = "g_portrait_midscene_duration";
constexpr char FIELD_G_LANDSCAPE_FULL_DURATION[] = "g_landscape_full_duration";
constexpr char FIELD_G_LANDSCAPE_SPLIT_DURATION[] = "g_landscape_split_duration";
constexpr char FIELD_G_LANDSCAPE_FLOATING_DURATION[] = "g_landscape_floating_duration";
constexpr char FIELD_G_LANDSCAPE_MIDSCENE_DURATION[] = "g_landscape_midscene_duration";
constexpr char FIELD_BUNDLE_NAME[] = "bundle_name";
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_USAGE_EVENT_COMMON_H
