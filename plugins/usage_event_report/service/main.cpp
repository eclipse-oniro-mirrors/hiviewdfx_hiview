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
#include <unistd.h>

#include "defines.h"
#include "hiview_logger.h"
#include "usage_event_report_service.h"

namespace {
DEFINE_LOG_TAG("HiView-UsageReportMain");
}

int main(int argc __UNUSED, char* argv[] __UNUSED)
{
    OHOS::HiviewDFX::UsageEventReportService service;
    if (!service.ProcessArgsRequest(argc, argv)) {
        HIVIEW_LOGE("failed to process args request");
        _exit(-1); // -1 means default error code
    }
    _exit(0);
}
