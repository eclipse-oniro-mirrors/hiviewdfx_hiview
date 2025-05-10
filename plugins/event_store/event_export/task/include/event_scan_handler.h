/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EVENT_SCAN_HANDLER_H
#define HIVIEW_BASE_EVENT_EXPORT_EVENT_SCAN_HANDLER_H

#include <string>

#include "export_base_handler.h"
#include "export_config_parser.h"

namespace OHOS {
namespace HiviewDFX {
struct EventScanRequest : public BaseRequest {
    // name of the export module
    std::string moduleName;

    // the directory for exported event file to store
    std::string scanDir;

    // day count threshold for zipped exported event file to store
    uint8_t storedDayCnt;
};

class EventScanHandler : public ExportBaseHandler {
public:
    bool HandleRequest(RequestPtr req) override;

private:
    bool Scan(const std::string& moduleName, const std::string& scanDir, uint8_t storedDayCnt);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EVENT_SCAN_HANDLER_H