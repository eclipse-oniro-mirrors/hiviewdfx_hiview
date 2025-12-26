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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_FOLD_CONSTANT_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_FOLD_CONSTANT_H

namespace OHOS {
namespace HiviewDFX {
namespace MultiWindowMode {
constexpr int32_t WINDOW_MODE_FULL = 0;
constexpr int32_t WINDOW_MODE_FLOATING = 1;
constexpr int32_t WINDOW_MODE_SPLIT_PRIMARY = 2;
constexpr int32_t WINDOW_MODE_SPLIT_SECONDARY = 3;
constexpr int32_t WINDOW_MODE_MIDSCENE = 4;
}

namespace FoldCommonUtils {
constexpr int SYSTEM_WINDOW_BASE = 2000;
inline constexpr char SO_NAME[] = "libusage_event_report_util.z.so";
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_FOLD_CONSTANT_H
