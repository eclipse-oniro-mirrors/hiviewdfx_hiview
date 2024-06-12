/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
namespace AppUsageEventSpace {
const std::string EVENT_NAME = "APP_USAGE";
const std::string KEY_OF_PACKAGE = "PACKAGE";
const std::string KEY_OF_VERSION = "VERSION";
const std::string KEY_OF_USAGE = "USAGE";
const std::string KEY_OF_DATE = "DATE";
const std::string KEY_OF_START_NUM = "TOTAL_START_NUM";
}

namespace FoldAppUsageEventSpace {
const std::string EVENT_NAME = "FOLD_APP_USAGE";
const std::string KEY_OF_PACKAGE = "PACKAGE";
const std::string KEY_OF_VERSION = "VERSION";
const std::string KEY_OF_USAGE = "USAGE";
const std::string KEY_OF_FOLD_VER_USAGE = "FOLD_V";
const std::string KEY_OF_FOLD_HOR_USAGE = "FOLD_H";
const std::string KEY_OF_EXPD_VER_USAGE = "EXPD_V";
const std::string KEY_OF_EXPD_HOR_USAGE = "EXPD_H";
const std::string KEY_OF_DATE = "DATE";
const std::string KEY_OF_START_NUM = "TOTAL_START_NUM";
const std::string SCENEBOARD_BUNDLE_NAME = "com.ohos.sceneboard"; // NOT include sceneboard
}

namespace SysUsageEventSpace {
const std::string EVENT_NAME = "SYS_USAGE";
const std::string KEY_OF_START = "START";
const std::string KEY_OF_END = "END";
const std::string KEY_OF_POWER = "POWER";
const std::string KEY_OF_RUNNING = "RUNNING";
}

namespace SysUsageDbSpace {
const std::string SYS_USAGE_TABLE = "sys_usage";
const std::string LAST_SYS_USAGE_TABLE = "last_sys_usage";
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
    const std::string FOREGROUND_EVENT_NAME = "APP_FOREGROUND";
    const std::string BACKGROUND_EVENT_NAME = "APP_BACKGROUND";
    const std::string KEY_OF_PROCESS_NAME = "PROCESS_NAME";
    const std::string KEY_OF_VERSION_NAME = "VERSION_NAME";
    const std::string KEY_OF_CALLER_BUNDLENAME = "CALLER_BUNDLENAME";
    const std::string KEY_OF_BUNDLE_TYPE = "BUNDLE_TYPE";
    const std::string KEY_OF_VERSION_CODE = "VERSION_CODE";
    const std::string KEY_OF_BUNDLE_NAME = "BUNDLE_NAME";
    const std::string KEY_OF_APP_PID = "APP_PID";
    const std::string KEY_OF_PROCESS_TYPE = "PROCESS_TYPE";
}

namespace FoldStateChangeEventSpace {
    const std::string EVENT_NAME = "NOTIFY_FOLD_STATE_CHANGE";
    const std::string KEY_OF_CURRENT_STATUS = "CURRENT_FOLD_STATUS";
    const std::string KEY_OF_NEXT_STATUS = "NEXT_FOLD_STATUS";
    const std::string KEY_OF_SENSOR_POSTURE = "SENSOR_POSTURE";
}

namespace VhModeChangeEventSpace {
    const std::string EVENT_NAME = "VH_MODE";
    const std::string KEY_OF_MODE = "MODE";
}

namespace ScreenFoldStatus {
    constexpr int EXPAND_LANDSCAPE_STATUS = 11;
    constexpr int EXPAND_PORTRAIT_STATUS = 12;
    constexpr int FOLD_LANDSCAPE_STATUS = 21;
    constexpr int FOLD_PORTRAIT_STATUS = 22;
}

namespace FoldEventTable {
    const std::string FIELD_ID = "id";
    const std::string FIELD_UID = "uid";
    const std::string FIELD_EVENT_ID = "rawid";
    const std::string FIELD_TS = "ts";
    const std::string FIELD_FOLD_STATUS = "fold_status";
    const std::string FIELD_PRE_FOLD_STATUS = "pre_fold_status";
    const std::string FIELD_VERSION_NAME = "version_name";
    const std::string FIELD_HAPPEN_TIME = "happen_time";
    const std::string FIELD_FOLD_PORTRAIT_DURATION = "fold_portrait_duration";
    const std::string FIELD_FOLD_LANDSCAPE_DURATION = "fold_landscape_duration";
    const std::string FIELD_EXPAND_PORTRAIT_DURATION = "expand_portrait_duration";
    const std::string FIELD_EXPAND_LANDSCAPE_DURATION = "expand_landscape_duration";
    const std::string FIELD_BUNDLE_NAME = "bundle_name";
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_USAGE_EVENT_COMMON_H
