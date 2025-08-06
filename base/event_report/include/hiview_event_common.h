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

#ifndef HIVIEW_BASE_EVENT_REPORT_HIVIEW_EVENT_COMMON_H
#define HIVIEW_BASE_EVENT_REPORT_HIVIEW_EVENT_COMMON_H

#include <string>

namespace OHOS {
namespace HiviewDFX {
constexpr unsigned int LABEL_DOMAIN = 0xD002D10;
inline constexpr char EVENT_DOMAIN[] = "HIVIEWDFX";

namespace BaseEventSpace {
inline constexpr char KEY_OF_DOMAIN[] = "domain_";
inline constexpr char KEY_OF_NAME[] = "name_";
inline constexpr char KEY_OF_TYPE[] = "type_";
}

namespace PluginEventSpace {
inline constexpr char LOAD_EVENT_NAME[] = "PLUGIN_LOAD";
inline constexpr char UNLOAD_EVENT_NAME[] = "PLUGIN_UNLOAD";
inline constexpr char KEY_OF_PLUGIN_NAME[] = "NAME";
inline constexpr char KEY_OF_RESULT[] = "RESULT";
inline constexpr char KEY_OF_DURATION[] = "DURATION";
constexpr uint32_t LOAD_SUCCESS = 0;
constexpr uint32_t LOAD_DUPLICATE_NAME = 1;
constexpr uint32_t LOAD_UNREGISTERED = 2;
constexpr uint32_t UNLOAD_SUCCESS = 0;
constexpr uint32_t UNLOAD_INVALID = 1;
constexpr uint32_t UNLOAD_NOT_FOUND = 2;
constexpr uint32_t UNLOAD_IN_USE = 3;
}

namespace PluginFaultEventSpace {
inline constexpr char EVENT_NAME[] = "PLUGIN_FAULT";
inline constexpr char KEY_OF_PLUGIN_NAME[] = "NAME";
inline constexpr char KEY_OF_REASON[] = "REASON";
}

namespace PluginStatsEventSpace {
inline constexpr char EVENT_NAME[] = "PLUGIN_STATS";
inline constexpr char KEY_OF_PLUGIN_NAME[] = "NAME";
inline constexpr char KEY_OF_AVG_TIME[] = "AVG_TIME";
inline constexpr char KEY_OF_TOP_K_TIME[] = "TOP_K_TIME";
inline constexpr char KEY_OF_TOP_K_EVENT[] = "TOP_K_EVENT";
inline constexpr char KEY_OF_TOTAL[] = "TOTAL";
inline constexpr char KEY_OF_PROC_NAME[] = "PROC_NAME";
inline constexpr char KEY_OF_PROC_TIME[] = "PROC_TIME";
inline constexpr char KEY_OF_TOTAL_TIME[] = "TOTAL_TIME";
}

namespace CpuSceneEvent {
inline constexpr char DOMAIN[] = "PERFORMANCE";
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_EVENT_REPORT_HIVIEW_EVENT_COMMON_H
