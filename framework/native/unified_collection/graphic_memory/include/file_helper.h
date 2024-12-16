/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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


#ifndef HIVIEWDFX_HIVIEW_GRAPHIC_MEMORY_FILE_HELPER_H
#define HIVIEWDFX_HIVIEW_GRAPHIC_MEMORY_FILE_HELPER_H

#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace Graphic {

class FileHelper {
public:
    using DataHandler = std::function<bool(std::string &line)>;
    static bool ReadFileByLine(const std::string &filePath, const DataHandler &func);
};

} // namespace Graphic
} // namespace HiviewDFX
} // namespace OHOS

#endif