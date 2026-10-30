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
#ifndef TELEMETRY_COMMON_H
#define TELEMETRY_COMMON_H

// This header provides logging macros for the standalone telemetry build.
// In standalone builds (TELEMETRY_STANDALONE), hiview_logger.h is not available,
// so we define the macros here. In non-standalone builds (e.g. xpower), the
// real hiview_logger.h is used via fault_common.h, so we must NOT redefine
// these macros to avoid -Wmacro-redefined errors.

#include <string>
#include <cinttypes>
#include <sys/types.h>

#ifdef TELEMETRY_STANDALONE
#include "hilog/log.h"
#include "telemetry_types.h"

// DFR log label macro for standalone builds (mirrors hiview_logger.h DEFINE_LOG_LABEL)
#ifndef DEFINE_LOG_LABEL
#define DEFINE_LOG_LABEL(region, name) \
    static unsigned int logLabelDomain = region; \
    static const char *logLabelTag = name
#endif

// HIVIEW_LOG macros (mirror hiview_logger.h)
#define HIVIEW_LOGD(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_DEBUG, logLabelDomain, logLabelTag, "%{public}s: " format, __func__, ##__VA_ARGS__)
#define HIVIEW_LOGI(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_INFO, logLabelDomain, logLabelTag, "%{public}s: " format, __func__, ##__VA_ARGS__)
#define HIVIEW_LOGW(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_WARN, logLabelDomain, logLabelTag, "%{public}s: " format, __func__, ##__VA_ARGS__)
#define HIVIEW_LOGE(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_ERROR, logLabelDomain, logLabelTag, "%{public}s: " format, __func__, ##__VA_ARGS__)
#define HIVIEW_LOGF(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_FATAL, logLabelDomain, logLabelTag, "%{public}s: " format, __func__, ##__VA_ARGS__)
#else
#include "hiview_logger.h"
#include "telemetry_types.h"
#endif // TELEMETRY_STANDALONE

#endif // TELEMETRY_COMMON_H
