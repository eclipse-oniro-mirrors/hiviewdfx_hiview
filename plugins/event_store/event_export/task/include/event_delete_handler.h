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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EVENT_DELETE_HANDLER_H
#define HIVIEW_BASE_EVENT_EXPORT_EVENT_DELETE_HANDLER_H

#include <string>
#include <vector>

#include "export_base_handler.h"
#include "export_config_parser.h"
#include "export_db_manager.h"

namespace OHOS {
namespace HiviewDFX {
struct EventDelRequest : public BaseRequest {
    // the expiredin export event file
    std::vector<std::string> files;

    // name of the export module
    std::string moduleName;
};

class EventDeleteHandler : public ExportBaseHandler {
public:
    bool HandleRequest(RequestPtr req) override;

private:
    bool Delete(std::vector<std::string>& files, const std::string& moduleName);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EVENT_DELETE_HANDLER_H