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

#include "log_file_name_converter.h"
#include <ctime>
#include <regex>
#include "time_util.h"
#include "bundle_util.h"
#include "file_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("LogFileNameConverter");

namespace {
constexpr const char* const PARAM_PROPERTY = "params";
constexpr const char* const PID = "pid";
constexpr const char* const RESOURCE_TYPE = "resource_type";
}

const std::map<LogFileType, LogFileTypeInfo> LOG_FILE_TYPE_MAP = {
    {LogFileType::JS_HEAP, {"memleak-js-[^-]+-\\d+-\\d+-\\d{14}\\.rawheap", "js_heap", "_js_heap.rawheap"}},

    {LogFileType::RSS_KERNEL_SMAPS, {"memleak-kernel-[^-]+-0-\\d{14}\\.txt", "rss_memory", "_rss_smaps.log"}},
    {LogFileType::RSS_JSHEAP, {"memleak-js-[^-]+-\\d+-\\d{14}\\.rawheap", "rss_memory", "_rss_jsheap.rawheap"}},
    {LogFileType::RSS_NATIVE_SMAPS,
        {"memleak-native-hiapp-[^-]+-\\d+-smaps.txt", "rss_memory", "_rss_smaps.log"}},
    {LogFileType::RSS_ASHMEM, {"memleak-kernel-hiapp-[^-]+-0-\\d{14}\\.txt", "rss_memory", "_rss_ashmem.htrace"}},
    {LogFileType::RSS_KOTLIN, {"memleak-kotlin-[^-]+-\\d+-\\d{14}\\.kdump", "rss_memory", "_rss_kotlin.kdump"}},
    {LogFileType::RSS_JSVM, {"memleak-jsvm-[^-]+-\\d+-\\d{14}\\.rawheap", "rss_memory", "_rss_jsvm.rawheap"}},
    {LogFileType::RSS_ARKWEBV8,
        {"memleak-arkweb_v8-[^-]+-\\d+-\\d{14}\\.rawheap", "rss_memory", "_rss_arkwebv8.rawheap"}},

    {LogFileType::EXTPSS_SMAPS, {"memleak-native-[^-]+-\\d+-smaps\\.txt", "pss_memory", "_extpss_smaps.log"}},
    {LogFileType::EXTPSS_PROCINFO, {"memleak-kernel-hiapp-[^-]+-0-\\d{14}\\.txt", "pss_memory",
        "_extpss_procinfo.log"}},
    {LogFileType::EXTPSS_NATIVE, {"memleak-native-[^-]+-\\d+-\\d{14}\\.txt", "pss_memory", "_extpss_native.htrace"}},
    {LogFileType::EXTPSS_GPU, {"memleak-kernel-gpu-0-\\d{14}\\.txt", "pss_memory", "_extpss_gpu.htrace"}},
    {LogFileType::EXTPSS_DMA, {"memleak-kernel-ion-0-\\d{14}\\.txt", "pss_memory", "_extpss_dma.htrace"}},
    {LogFileType::EXTPSS_JSHEAP, {"memleak-js-[^-]+-\\d+-\\d{14}\\.rawheap", "pss_memory", "_extpss_jsheap.rawheap"}},
    {LogFileType::EXTPSS_ASHMEM, {"memleak-kernel-ashmem-0-\\d{14}\\.txt", "pss_memory", "_extpss_ashmem.htrace"}},
    {LogFileType::EXTPSS_KOTLIN, {"memleak-kotlin-[^-]+-\\d+-\\d{14}\\.kdump", "pss_memory", "_extpss_kotlin.kdump"}},
    {LogFileType::EXTPSS_JSVM, {"memleak-jsvm-[^-]+-\\d+-\\d{14}\\.rawheap", "pss_memory",
                                "_extpss_jsvm.rawheap"}},
    {LogFileType::EXTPSS_ARKWEBV8, {"memleak-arkweb_v8-[^-]+-\\d+-\\d{14}\\.rawheap", "pss_memory",
                                    "_extpss_arkwebv8.rawheap"}},

    {LogFileType::DMA_LOG, {"memleak-kernel-[^-]+-0-\\d{14}\\.txt", "ion_memory", "_dma.log"}},
    {LogFileType::DMA_HTRACE, {"memleak-kernel-[^-]+-\\d+-\\d{14}\\.txt", "ion_memory", "_dma.htrace"}},

    {LogFileType::GPU_LOG, {"memleak-kernel-[^-]+-0-\\d{14}\\.txt", "gpu_memory", "_gpu.log"}},
    {LogFileType::GPU_HTRACE, {"memleak-kernel-[^-]+-\\d+-\\d{14}\\.txt", "gpu_memory", "_gpu.htrace"}},

    {LogFileType::FD_LOG, {"fdleak-[^-]+-\\d+-log-\\d{14}\\.txt", "fd", "_fd.log"}},
    {LogFileType::FD_HTRACE, {"fdleak-hiapp-[^-]+-\\d+-profiler-\\d{14}\\.txt", "fd", "_fd.htrace"}},

    {LogFileType::THREAD_LOG, {"threadleak-[^-]+-\\d+-log-\\d{14}\\.txt", "thread", "_thread.log"}},
    {LogFileType::THREAD_HTRACE, {"threadleak-hiapp-[^-]+-\\d+-profiler-\\d{14}\\.txt", "thread", "_thread.htrace"}},
};

LogFileType MatchLogFileType(const std::string& fileName, const std::string& resouceType)
{
    if (fileName.empty()) {
        return LogFileType::UNKNOWN;
    }

    for (const auto& [type, info] : LOG_FILE_TYPE_MAP) {
        if (std::regex_match(fileName, std::regex(info.regexPattern))) {
            if (resouceType == info.resourceType) {
                HIVIEW_LOGI("name: %{public}s, mapLogType: %{public}s", fileName.c_str(),
                            LOG_TYPE_NAME[static_cast<int>(type)]);
                return type;
            }
        }
    }
    HIVIEW_LOGE("name: %{public}s, resouceType: %{public}s", fileName.c_str(), resouceType.c_str());
    return LogFileType::UNKNOWN;
}

std::string ExtractTimestampFromFileName(const std::string& fileName)
{
    std::regex timestampPattern("\\d{14}");
    std::smatch match;
    if (std::regex_search(fileName, match, timestampPattern) && match.size() > 0) {
        std::string timestampStr = match.str();
        if (!timestampStr.empty()) {
            return timestampStr;
        }
    }
    return "";
}

std::string ConvertTimestampToMilliseconds(const std::string& timestampStr)
{
    if (timestampStr.empty()) {
        return std::to_string(TimeUtil::GetMilliseconds());
    }
    if (timestampStr.length() == TIMESTAMP_LENGTH) {
        time_t timeStamp = TimeUtil::StrToTimeStamp(timestampStr, "%Y%m%d%H%M%S");
        return std::to_string(timeStamp * MILLISECONDS_MULTIPLIER);
    }
    return timestampStr;
}

std::string GenerateNewFileName(LogFileType fileType, const std::string& timestampStr, int pid)
{
    auto it = LOG_FILE_TYPE_MAP.find(fileType);
    if (it == LOG_FILE_TYPE_MAP.end()) {
        return "";
    }

    std::string timeStr = ConvertTimestampToMilliseconds(timestampStr);
    std::string pidStr = std::to_string(pid);
    std::string prefix = "RESOURCE_OVERLIMIT_" + timeStr + "_" + pidStr;

    return prefix + it->second.suffix;
}

void ConvertLogFileName(const std::string& oldFileName, std::string& newFileName, int pid,
                        const std::string& resourceType)
{
    if (oldFileName.empty()) {
        return;
    }

    LogFileType fileType = MatchLogFileType(oldFileName, resourceType);
    if (fileType == LogFileType::UNKNOWN) {
        newFileName = "";
        HIVIEW_LOGE("UNKNOWN type, name: %{public}s, resourceType: %{public}s, pid: %{public}d, newFileName empty",
                    oldFileName.c_str(), resourceType.c_str(), pid);
        return;
    }

    std::string timestampStr = ExtractTimestampFromFileName(oldFileName);
    newFileName = GenerateNewFileName(fileType, timestampStr, pid);
}

bool ShouldRefinedLogFileName(int32_t uid, const std::string& pathHolder)
{
    std::string eventConfigDir = BundleUtil::GetSandBoxPath(uid, "base", pathHolder, "cache/eventConfig");
    if (eventConfigDir.empty()) {
        HIVIEW_LOGE("Current sandbox eventConfig path is not exist.");
        return false;
    }
    std::string property = std::string("user.event_config.useRefinedLogFileName");
    std::string value;
    if (!FileUtil::GetDirXattr(eventConfigDir, property, value)) {
        HIVIEW_LOGW("failed to get dir cfg xattr about refined, value: %{public}s", value.c_str());
        return false;
    }
    return value == "true";
}

void RefineLogFilePaths(Json::Value& eventJson, const std::string& oldLogPath, std::string& newFileName,
                        bool needRefined)
{
    if (!needRefined) {
        return;
    }
    if (oldLogPath.empty()) {
        return;
    }
    int pid = 0;
    std::string resourceType;
    if (eventJson[PARAM_PROPERTY].isMember(PID) && eventJson[PARAM_PROPERTY][PID].isInt()) {
        pid = eventJson[PARAM_PROPERTY][PID].asInt();
    }
    if (eventJson[PARAM_PROPERTY].isMember(RESOURCE_TYPE) && eventJson[PARAM_PROPERTY][RESOURCE_TYPE].isString()) {
        resourceType = eventJson[PARAM_PROPERTY][RESOURCE_TYPE].asString();
    }
    std::string oldFileName = FileUtil::ExtractFileName(oldLogPath);
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
    HIVIEW_LOGI("use refined file name: %{public}s, curLogName: %{public}s", oldLogPath.c_str(),
                newFileName.c_str());
}

} // namespace HiviewDFX
} // namespace OHOS