/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_BASE_LOGGER_H
#define HIVIEW_BASE_LOGGER_H

#include <cinttypes>
#include "hilog/log.h"

#define PUBLIC "{public}"
// All Log domain in hiview should has the prefix of 0xD002D
// And every src file use this header should define LOG_DOMAIN and LOG_TAG
#define DEFINE_LOG_TAG(name) \
    static unsigned int logLabelDomain = 0xD002D10; \
    static const char *logLabelTag = name

#define DEFINE_LOG_LABEL(region, name) \
    static unsigned int logLabelDomain = region; \
    static const char *logLabelTag = name

#define HIVIEW_LOGD(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_DEBUG, logLabelDomain, logLabelTag, "%" PUBLIC "s: " format, __func__, ##__VA_ARGS__)
#define HIVIEW_LOGI(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_INFO, logLabelDomain, logLabelTag, "%" PUBLIC "s: " format, __func__, ##__VA_ARGS__)
#define HIVIEW_LOGW(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_WARN, logLabelDomain, logLabelTag, "%" PUBLIC "s: " format, __func__, ##__VA_ARGS__)
#define HIVIEW_LOGE(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_ERROR, logLabelDomain, logLabelTag, "%" PUBLIC "s: " format, __func__, ##__VA_ARGS__)
#define HIVIEW_LOGF(format, ...) \
    HILOG_IMPL(LOG_CORE, LOG_FATAL, logLabelDomain, logLabelTag, "%" PUBLIC "s: " format, __func__, ##__VA_ARGS__)

#endif // HIVIEW_BASE_LOGGER_H
