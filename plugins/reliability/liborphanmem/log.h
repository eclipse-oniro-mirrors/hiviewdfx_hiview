/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef LIBORPHANMEM_LOG_H
#define LIBORPHANMEM_LOG_H

#include <log_base.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0xD002DA0
#define LOG_TAG "liborphanmem"

#define HILOGD(fmt, ...) HILOG_BASE_DEBUG(LOG_CORE, fmt, ##__VA_ARGS__)
#define HILOGI(fmt, ...) HILOG_BASE_INFO(LOG_CORE, fmt, ##__VA_ARGS__)
#define HILOGW(fmt, ...) HILOG_BASE_WARN(LOG_CORE, fmt, ##__VA_ARGS__)
#define HILOGE(fmt, ...) HILOG_BASE_ERROR(LOG_CORE, fmt, ##__VA_ARGS__)
#define HILOGF(fmt, ...) \
    HILOG_BASE_FATAL(LOG_CORE, "[%{public}s:%{public}d] " fmt, (__FILE_NAME__), (__LINE__), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif
