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
#ifndef OHOS_HIVIEWDFX_UCOLLECT_TEST_COMMON_H
#define OHOS_HIVIEWDFX_UCOLLECT_TEST_COMMON_H
#include <gtest/gtest.h>

#include "collect_result.h"
#include "common_util.h"
#include "file_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollect {
template <typename T> void FileCleanTest(T task, const std::string& path, const std::string& prefix, int32_t maxNum)
{
    std::string oldFile;
    for (int32_t count = 0; count <= maxNum; ++count) {
        auto result = task();
        ASSERT_TRUE(result.retCode == UcError::SUCCESS);
        if (count == 0) {
            oldFile = result.data;
        }
    }
    ASSERT_FALSE(FileUtil::FileExists(oldFile));
    std::vector<std::string> files;
    UCollectUtil::CommonUtil::GetDirRegexFiles(path, prefix, files);
    ASSERT_EQ(files.size(), maxNum);
}
} // UCollect
} // HiviewDFX
} // OHOS
#endif // OHOS_HIVIEWDFX_UCOLLECT_TEST_COMMON_H
