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
#include "freeze_stack_summary.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <limits>
#include <sys/stat.h>

#include "constants.h"
#include "file_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char SUMMARY_BEGIN[] = "---AppFreezeHeaviestStack Begin---";
constexpr char SUMMARY_END[] = "---AppFreezeHeaviestStack End---";
constexpr size_t MAX_SUMMARY_FILE_SIZE = 32 * 1024 * 1024;

bool ParseInt(const std::string& value, int32_t& result)
{
    if (value.empty()) {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    long parsed = std::strtol(value.c_str(), &end, FaultLogger::DECIMAL_BASE);
    if (errno != 0 || end == value.c_str() || *end != '\0' || parsed < INT32_MIN || parsed > INT32_MAX) {
        return false;
    }
    result = static_cast<int32_t>(parsed);
    return true;
}

bool ParseUint64(const std::string& value, uint64_t& result)
{
    if (value.empty() || value[0] == '-') {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    unsigned long long parsed = std::strtoull(value.c_str(), &end, FaultLogger::DECIMAL_BASE);
    if (errno != 0 || end == value.c_str() || *end != '\0' ||
        parsed > std::numeric_limits<uint64_t>::max()) {
        return false;
    }
    result = static_cast<uint64_t>(parsed);
    return true;
}

std::string Unescape(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); i++) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            result.push_back(value[i]);
            continue;
        }
        switch (value[++i]) {
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case '\\': result.push_back('\\'); break;
            case ':': result.push_back(':'); break;
            default: result.push_back(value[i]); break;
        }
    }
    return result;
}

std::vector<std::string> SplitPaths(const std::string& pathList)
{
    std::vector<std::string> paths;
    size_t begin = 0;
    while (begin <= pathList.size()) {
        size_t end = pathList.find(',', begin);
        std::string path = pathList.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
        if (!path.empty()) {
            paths.emplace_back(path);
        }
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1;
    }
    return paths;
}

bool LoadBoundedFile(const std::string& path, std::string& content)
{
    struct stat fileStat {};
    if (stat(path.c_str(), &fileStat) != 0 || fileStat.st_size < 0 ||
        static_cast<uint64_t>(fileStat.st_size) > MAX_SUMMARY_FILE_SIZE) {
        return false;
    }
    return FileUtil::LoadStringFromFile(path, content);
}

void SetDefaultQuality(std::map<std::string, std::string>& eventInfos)
{
    eventInfos[FaultKey::BUSIEST_STACK_COUNT] = "0";
    eventInfos[FaultKey::BUSIEST_STACK_RATIO_PERMILLE] = "0";
    eventInfos[FaultKey::STACK_SOURCE] = "NONE";
    eventInfos[FaultKey::HAS_MAIN_THREAD_STACK] = "false";
    eventInfos[FaultKey::HEAVIEST_STACK_STATUS] = "no_sample";
    eventInfos[FaultKey::LOG_VALIDITY] = "INVALID";
    eventInfos[FaultKey::LOG_INVALID_REASON] = "MAIN_THREAD_SAMPLE_MISSING";
}

void SetFallbackQuality(std::map<std::string, std::string>& eventInfos, const std::string& reason,
                        const std::string& status = "parse_failed")
{
    const bool hasInstantStack =
        (!eventInfos[FaultKey::FIRST_FRAME].empty() || !eventInfos[FaultKey::SECOND_FRAME].empty() ||
         !eventInfos[FaultKey::LAST_FRAME].empty());
    eventInfos[FaultKey::STACK_SOURCE] = hasInstantStack ? "INSTANT_STACK" : "NONE";
    eventInfos[FaultKey::HEAVIEST_STACK_STATUS] = status;
    eventInfos[FaultKey::LOG_VALIDITY] = hasInstantStack ? "DEGRADED" : "INVALID";
    eventInfos[FaultKey::LOG_INVALID_REASON] = reason;
    eventInfos[FaultKey::HAS_MAIN_THREAD_STACK] = hasInstantStack ? "true" : "false";
}

}  // namespace

bool ParseFreezeStackSummary(const std::string& content, FreezeStackSummary& summary)
{
    size_t begin = content.find(SUMMARY_BEGIN);
    if (begin == std::string::npos) {
        return false;
    }
    begin = content.find('\n', begin);
    size_t end = content.find(SUMMARY_END, begin == std::string::npos ? 0 : begin + 1);
    if (begin == std::string::npos || end == std::string::npos || end <= begin ||
        content.find(SUMMARY_BEGIN, begin + 1) != std::string::npos) {
        return false;
    }

    std::map<std::string, std::string> values;
    size_t cursor = begin + 1;
    while (cursor < end) {
        size_t lineEnd = content.find('\n', cursor);
        lineEnd = lineEnd == std::string::npos || lineEnd > end ? end : lineEnd;
        std::string line = content.substr(cursor, lineEnd - cursor);
        size_t separator = line.find(':');
        if (separator == std::string::npos || separator == 0 ||
            !values.emplace(line.substr(0, separator), line.substr(separator + 1)).second) {
            return false;
        }
        cursor = lineEnd + 1;
    }

    auto statusIt = values.find("status");
    auto versionIt = values.find("version");
    if (versionIt == values.end() || versionIt->second != "1" || statusIt == values.end() ||
        (statusIt->second != "success" && statusIt->second != "no_sample" &&
        statusIt->second != "failed")) {
        return false;
    }
    summary = {};
    summary.status = statusIt->second;
    uint64_t firstSnapshotTime = 0;
    uint64_t stackId = 0;
    auto firstSnapshotIt = values.find("first_snapshot_time");
    auto stackIdIt = values.find("stack_id");
    if (firstSnapshotIt == values.end() || stackIdIt == values.end() ||
        !ParseUint64(firstSnapshotIt->second, firstSnapshotTime) || !ParseUint64(stackIdIt->second, stackId)) {
        return false;
    }
    summary.firstSnapshotTime = firstSnapshotTime;
    summary.stackId = stackId;
    auto totalIt = values.find("total_samples");
    auto countIt = values.find("busiest_count");
    auto ratioIt = values.find("busiest_ratio_permille");
    if (totalIt == values.end() || countIt == values.end() || ratioIt == values.end() ||
        !ParseInt(totalIt->second, summary.totalSamples) || !ParseInt(countIt->second, summary.busiestCount) ||
        !ParseInt(ratioIt->second, summary.busiestRatioPermille) || summary.totalSamples < 0 ||
        summary.busiestCount < 0 || summary.busiestCount > summary.totalSamples ||
        summary.busiestRatioPermille < 0 || summary.busiestRatioPermille > 1000) {
        return false;
    }

    int32_t frameCount = 0;
    auto frameCountIt = values.find("frame_count");
    if (frameCountIt == values.end() || !ParseInt(frameCountIt->second, frameCount) || frameCount < 0 ||
        frameCount > 256) {
        return false;
    }
    for (int32_t i = 0; i < frameCount; i++) {
        std::string key = "frame_" + std::to_string(i);
        auto frameIt = values.find(key);
        if (frameIt == values.end()) {
            return false;
        }
        summary.frames.emplace_back(Unescape(frameIt->second));
    }
    return true;
}

bool ApplyFreezeStackSummary(const std::string& pathList, std::map<std::string, std::string>& eventInfos)
{
    SetDefaultQuality(eventInfos);
    FreezeStackSummary summary;
    bool parsed = false;
    for (const auto& path : SplitPaths(pathList)) {
        std::string content;
        if (LoadBoundedFile(path, content) && ParseFreezeStackSummary(content, summary)) {
            parsed = true;
            break;
        }
    }
    if (!parsed || summary.status != "success") {
        const std::string status = parsed && summary.status == "no_sample" ? "no_sample" : "parse_failed";
        SetFallbackQuality(eventInfos, parsed ? "MAIN_THREAD_SAMPLE_MISSING" : "HEAVIEST_STACK_PARSE_FAILED",
                           status);
        return false;
    }

    eventInfos[FaultKey::BUSIEST_STACK_COUNT] = std::to_string(summary.busiestCount);
    eventInfos[FaultKey::BUSIEST_STACK_RATIO_PERMILLE] = std::to_string(summary.busiestRatioPermille);
    if (summary.frames.empty()) {
        SetFallbackQuality(eventInfos, "EMPTY_HEAVIEST_STACK");
        return false;
    }
    eventInfos[FaultKey::STACK_SOURCE] = "HEAVIEST_SAMPLE";
    eventInfos[FaultKey::HAS_MAIN_THREAD_STACK] = "true";
    eventInfos[FaultKey::HEAVIEST_STACK_STATUS] = "success";
    eventInfos[FaultKey::LOG_VALIDITY] = "FULL";
    eventInfos[FaultKey::LOG_INVALID_REASON].clear();
    const auto& frames = summary.frames;
    if (frames.size() == 1) {
        eventInfos[FaultKey::FIRST_FRAME] = frames[0];
        eventInfos[FaultKey::SECOND_FRAME].clear();
        eventInfos[FaultKey::LAST_FRAME] = frames[0];
    } else if (frames.size() == 2) {
        eventInfos[FaultKey::FIRST_FRAME] = frames[0];
        eventInfos[FaultKey::SECOND_FRAME] = frames[1];
        eventInfos[FaultKey::LAST_FRAME] = frames[1];
    } else {
        eventInfos[FaultKey::FIRST_FRAME] = frames.front();
        eventInfos[FaultKey::SECOND_FRAME] = frames[1];
        eventInfos[FaultKey::LAST_FRAME] = frames.back();
    }
    return true;
}
}  // namespace HiviewDFX
}  // namespace OHOS
