/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_HIVIEWDFX_XPERF_SERVICE_LOG_H
#define OHOS_HIVIEWDFX_XPERF_SERVICE_LOG_H

#include "hilog/log.h"

namespace OHOS {
namespace HiviewDFX {
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D66

#undef LOG_TAG
#define LOG_TAG "XPERF_SERVICE"

#define LOGD(fmt, ...) HILOG_DEBUG(LOG_CORE, "[%{public}s]: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOGI(fmt, ...) HILOG_INFO(LOG_CORE, "[%{public}s]: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOGW(fmt, ...) HILOG_WARN(LOG_CORE, "[%{public}s]: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOGE(fmt, ...) HILOG_ERROR(LOG_CORE, "[%{public}s]: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOGF(fmt, ...) HILOG_FATAL(LOG_CORE, "[%{public}s]: " fmt, __FUNCTION__, ##__VA_ARGS__)
} // namespace HiviewDFX
} // namespace OHOS

#endif
