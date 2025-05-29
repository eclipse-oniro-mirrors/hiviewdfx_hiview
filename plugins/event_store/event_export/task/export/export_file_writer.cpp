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

#include "export_file_writer.h"

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");
bool ExportFileWriter::Write(std::shared_ptr<ExportFileBaseBuilder> fileBuilder, CachedEventMap events,
    WriteStrategyParam& param)
{
    if (fileBuilder == nullptr) {
        HIVIEW_LOGE("invalid export file builder");
        return false;
    }
    std::string buildStr;
    if (!fileBuilder->Build(events, buildStr)) {
        return false;
    }
    auto strategy = EventWriteStrategyFactory::GetWriteStrategy(StrategyType::ZIP_JSON_FILE);
    if (strategy == nullptr) {
        HIVIEW_LOGE("write strategy is null");
        return false;
    }
    strategy->SetWriteStrategyParam(param);
    return strategy->Write(buildStr,
        [this] (const std::string& srcPath, const std::string& destPath) {
            if (exportFileWroteListener_ == nullptr) {
                return;
            }
            exportFileWroteListener_(srcPath, destPath);
        });
}

void ExportFileWriter::SetExportFileWroteListener(ExportFileWroteListener listener)
{
    exportFileWroteListener_ = listener;
}
} // HiviewDFX
} // OHOS
