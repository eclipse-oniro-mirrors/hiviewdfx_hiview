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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EXPORT_FILE_WRITER_H
#define HIVIEW_BASE_EVENT_EXPORT_EXPORT_FILE_WRITER_H

#include <functional>
#include <memory>
#include <string>

#include "cached_event.h"
#include "event_write_strategy_factory.h"
#include "export_file_base_builder.h"

namespace OHOS {
namespace HiviewDFX {
using ExportFileWroteListener = std::function<void(const std::string&, const std::string&)>;
class ExportFileWriter {
public:
    void SetExportFileWroteListener(ExportFileWroteListener listener);
    bool Write(std::shared_ptr<ExportFileBaseBuilder> fileBuilder, CachedEventMap cachedEventMap,
        WriteStrategyParam& param);

private:
    ExportFileWroteListener exportFileWroteListener_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EXPORT_FILE_WRITER_H
