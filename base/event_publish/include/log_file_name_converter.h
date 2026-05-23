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

#ifndef OHOS_HIVIEWDFX_LOG_FILE_NAME_CONVERTER_H
#define OHOS_HIVIEWDFX_LOG_FILE_NAME_CONVERTER_H

#include <string>
#include <map>
#include <json/json.h>

namespace OHOS {
namespace HiviewDFX {

struct LogFileTypeInfo {
    std::string regexPattern;
    std::string resourceType;
    std::string suffix;
};

enum class LogFileType {
    JS_HEAP,
    RSS_KERNEL_SMAPS,
    RSS_JSHEAP,
    RSS_NATIVE_SMAPS,
    RSS_NATIVEHEAP,
    RSS_ASHMEM,
    RSS_KOTLIN,
    RSS_JSVM,
    RSS_ARKWEBV8,

    EXTPSS_SMAPS,
    EXTPSS_PROCINFO,
    EXTPSS_NATIVE,
    EXTPSS_GPU,
    EXTPSS_DMA,
    EXTPSS_JSHEAP,
    EXTPSS_ASHMEM,
    EXTPSS_KOTLIN,
    EXTPSS_JSVM,
    EXTPSS_ARKWEBV8,

    DMA_LOG,
    DMA_HTRACE,
    GPU_LOG,
    GPU_HTRACE,

    FD_LOG,
    FD_HTRACE,

    THREAD_LOG,
    THREAD_HTRACE,

    UNKNOWN
};

inline constexpr const char* const LOG_TYPE_NAME[] = {
    "jsHeap",
    "rssKernelSmaps",
    "rssJsHeap",
    "rssNativeSmaps",
    "rssNativeheap",
    "rssAshmem",
    "rssKotlin",
    "rssJsvm",
    "rssArkwebv8",

    "extpssSmaps",
    "extpssProcInfo",
    "extpssNative",
    "extpssGpu",
    "extpssDma",
    "extpssJsheap",
    "extpssAshmem",
    "extpssKotlin",
    "extpssJsvm",
    "extpssArkwebv8",

    "dmaLog",
    "dmaHtrace",
    "gpuLog",
    "gpuHtrace",

    "fdLog",
    "fdHtrace",

    "threadLog",
    "threadHtrace",

    "unknown"
};


constexpr int TIMESTAMP_LENGTH = 14;
constexpr int MILLISECONDS_MULTIPLIER = 1000;

void ConvertLogFileName(const std::string& oldFileName, std::string& newFileName, int pid,
                        const std::string& resouceType);
bool ShouldRefinedLogFileName(int32_t uid, const std::string& pathHolder);
void RefineLogFilePaths(Json::Value& eventJson, const std::string& oldLogPath, std::string& newFileName,
                        bool needRefined);

} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_LOG_FILE_NAME_CONVERTER_H