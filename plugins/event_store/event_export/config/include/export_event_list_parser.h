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

#ifndef HIVIEW_BASE_EXPORT_EVENT_LIST_PARSER_H
#define HIVIEW_BASE_EXPORT_EVENT_LIST_PARSER_H

#include <map>

#include "cJSON.h"
#include "event_json_parser.h"

namespace OHOS {
namespace HiviewDFX {
class ExportEventListParser {
public:
    ExportEventListParser(const std::string& configFile);
    virtual ~ExportEventListParser();

public:
    void GetExportEventList(ExportEventList& eventList) const;
    virtual int64_t GetConfigurationVersion() const;

private:
    void ParseConfiguration();

private:
    cJSON* jsonRoot_ = nullptr;
    int64_t configurationVersion_ = 0;
};
} // HiviewDFX
} // OHOS

#endif // HIVIEW_BASE_EXPORT_EVENT_LIST_PARSER_H
