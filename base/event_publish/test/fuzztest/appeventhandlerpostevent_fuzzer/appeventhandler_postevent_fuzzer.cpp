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

#include <cstdint>
#include <cstddef>
#include <sstream>
#include <string>

#define private public
#define protected public
#include "app_event_handler.cpp"
#undef private
#undef protected
#include "appeventhandler_postevent_fuzzer.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FUZZTEST_MAX_STR_LEN = 32;
constexpr int FUZZ_SELECTOR_BYTES = 1;

std::string MakeStr(const uint8_t*& data, size_t& remain)
{
    std::string s;
    if (remain >= FUZZTEST_MAX_STR_LEN) {
        s = std::string(reinterpret_cast<const char*>(data), FUZZTEST_MAX_STR_LEN);
        data += FUZZTEST_MAX_STR_LEN;
        remain -= FUZZTEST_MAX_STR_LEN;
    }
    return s;
}

template <typename T>
bool ReadValue(const uint8_t*& data, size_t& remain, T& value)
{
    if (remain < sizeof(T)) {
        return false;
    }
    errno_t err = memcpy_s(&value, sizeof(T), data, sizeof(T));
    if (err != EOK) {
        return false;
    }
    data += sizeof(T);
    remain -= sizeof(T);
    return true;
}

void FuzzPostAppLaunch(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::AppLaunchInfo info;
    info.bundleName = MakeStr(data, remain);
    info.bundleVersion = MakeStr(data, remain);
    info.processName = MakeStr(data, remain);
    ReadValue(data, remain, info.startType);
    ReadValue(data, remain, info.iconInputTime);
    ReadValue(data, remain, info.animationFinishTime);
    ReadValue(data, remain, info.extendTime);
    ReadValue(data, remain, info.responseLatency);
    handler.PostEvent(info);
}

void FuzzPostScrollJank(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::ScrollJankInfo info;
    info.bundleName = MakeStr(data, remain);
    info.bundleVersion = MakeStr(data, remain);
    info.processName = MakeStr(data, remain);
    info.abilityName = MakeStr(data, remain);
    ReadValue(data, remain, info.beginTime);
    ReadValue(data, remain, info.duration);
    ReadValue(data, remain, info.totalAppFrames);
    ReadValue(data, remain, info.totalAppMissedFrames);
    info.externalLog.emplace_back(MakeStr(data, remain));
    handler.PostEvent(info);
}

void FuzzPostResourceOverLimit(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::ResourceOverLimitInfo info;
    info.bundleName = MakeStr(data, remain);
    info.bundleVersion = MakeStr(data, remain);
    info.resourceType = MakeStr(data, remain);
    ReadValue(data, remain, info.pid);
    ReadValue(data, remain, info.uid);
    ReadValue(data, remain, info.limitSize);
    ReadValue(data, remain, info.fdNum);
    info.topFdType = MakeStr(data, remain);
    info.level = MakeStr(data, remain);
    info.logPath.emplace_back(MakeStr(data, remain));
    handler.PostEvent(info);
}

void FuzzPostCpuUsageHigh(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::CpuUsageHighInfo info;
    info.bundleName = MakeStr(data, remain);
    info.bundleVersion = MakeStr(data, remain);
    ReadValue(data, remain, info.time);
    ReadValue(data, remain, info.beginTime);
    ReadValue(data, remain, info.endTime);
    uint8_t rawForeground = 0;
    ReadValue(data, remain, rawForeground);
    info.isForeground = (rawForeground != 0);
    ReadValue(data, remain, info.usage);
    ReadValue(data, remain, info.faultType);
    info.appRunningUniqueId = MakeStr(data, remain);
    AppEventHandler::ThreadInfo threadInfo;
    threadInfo.name = MakeStr(data, remain);
    ReadValue(data, remain, threadInfo.tid);
    ReadValue(data, remain, threadInfo.usage);
    info.threads.emplace_back(threadInfo);
    info.externalLog.emplace_back(MakeStr(data, remain));
    handler.PostEvent(info);
}

void FuzzPostBatteryUsage(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::BatteryUsageInfo info;
    info.bundleName = MakeStr(data, remain);
    info.bundleVersion = MakeStr(data, remain);
    ReadValue(data, remain, info.time);
    ReadValue(data, remain, info.beginTime);
    ReadValue(data, remain, info.endTime);
    handler.PostEvent(info);
}

void FuzzPostAppKilled(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::AppKilledInfo info;
    info.bundleName = MakeStr(data, remain);
    info.bundleVersion = MakeStr(data, remain);
    ReadValue(data, remain, info.time);
    ReadValue(data, remain, info.beginTime);
    ReadValue(data, remain, info.endTime);
    info.reason = MakeStr(data, remain);
    info.appRunningUniqueId = MakeStr(data, remain);
    ReadValue(data, remain, info.uid);
    ReadValue(data, remain, info.callingPid);
    uint8_t rawKilledFg = 0;
    ReadValue(data, remain, rawKilledFg);
    info.isForeground = (rawKilledFg != 0);
    info.callingProcessName = MakeStr(data, remain);
    info.lastExitDetailInfo.pid = MakeStr(data, remain);
    info.lastExitDetailInfo.uid = MakeStr(data, remain);
    info.lastExitDetailInfo.processName = MakeStr(data, remain);
    info.lastExitDetailInfo.exitMsg = MakeStr(data, remain);
    info.lastExitDetailInfo.killReason = MakeStr(data, remain);
    handler.PostEvent(info);
}

void FuzzPostAudioJankFrame(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::AudioJankFrameInfo info;
    info.bundleName = MakeStr(data, remain);
    info.bundleVersion = MakeStr(data, remain);
    ReadValue(data, remain, info.time);
    ReadValue(data, remain, info.maxFrameTime);
    ReadValue(data, remain, info.happenTime);
    info.faultType = MakeStr(data, remain);
    handler.PostEvent(info);
}

void FuzzAddMemoryDetail(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::DetailCommonMemoryInfo info;
    ReadValue(data, remain, info.nativeHeap);
    ReadValue(data, remain, info.arktsHeap);
    ReadValue(data, remain, info.anonPageOther);
    ReadValue(data, remain, info.filePageOther);
    std::stringstream jsonStr;
    AddMemoryDetailToJsonString(jsonStr, info);
}

void FuzzAddExtraFiledByResourceType(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::ResourceOverLimitInfo info;
    info.resourceType = MakeStr(data, remain);
    ReadValue(data, remain, info.extPssInfo.detailSmapsInfo.nativeHeap);
    ReadValue(data, remain, info.rssInfo.nativeHeap);
    std::stringstream jsonStr;
    AddExtraFiledByResourceType(jsonStr, info);
}

void FuzzHandleMemoryResourceType(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::ResourceOverLimitInfo info;
    info.resourceType = MakeStr(data, remain);
    ReadValue(data, remain, info.pss);
    ReadValue(data, remain, info.gpu);
    ReadValue(data, remain, info.ion);
    ReadValue(data, remain, info.rss);
    ReadValue(data, remain, info.avaliableMem);
    ReadValue(data, remain, info.freeMem);
    ReadValue(data, remain, info.totalMem);
    std::stringstream jsonStr;
    HandleMemoryResourceType(jsonStr, info);
}

void FuzzHandleJsHeapResourceType(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::ResourceOverLimitInfo info;
    ReadValue(data, remain, info.limitSize);
    ReadValue(data, remain, info.liveobjectSize);
    std::stringstream jsonStr;
    HandleJsHeapResourceType(jsonStr, info);
}

void FuzzHandleFdResourceType(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::ResourceOverLimitInfo info;
    ReadValue(data, remain, info.fdNum);
    info.topFdType = MakeStr(data, remain);
    ReadValue(data, remain, info.topFdNum);
    std::stringstream jsonStr;
    HandleFdResourceType(jsonStr, info);
}

void FuzzHandleThreadResourceType(const uint8_t*& data, size_t& remain, AppEventHandler& handler)
{
    AppEventHandler::ResourceOverLimitInfo info;
    ReadValue(data, remain, info.threadNum);
    std::stringstream jsonStr;
    HandleThreadResourceType(jsonStr, info);
}

using FuzzFunc = void(*)(const uint8_t*&, size_t&, AppEventHandler&);

const FuzzFunc FUZZ_FUNCS[] = {
    FuzzPostAppLaunch,
    FuzzPostScrollJank,
    FuzzPostResourceOverLimit,
    FuzzPostCpuUsageHigh,
    FuzzPostBatteryUsage,
    FuzzPostAppKilled,
    FuzzPostAudioJankFrame,
    FuzzAddMemoryDetail,
    FuzzAddExtraFiledByResourceType,
    FuzzHandleMemoryResourceType,
    FuzzHandleJsHeapResourceType,
    FuzzHandleFdResourceType,
    FuzzHandleThreadResourceType,
};

void FuzzAppEventHandlerPostEvent(const uint8_t* data, size_t size)
{
    if (size < FUZZ_SELECTOR_BYTES) {
        return;
    }
    constexpr size_t funcCount = sizeof(FUZZ_FUNCS) / sizeof(FUZZ_FUNCS[0]);
    uint8_t selector = data[0] % funcCount;
    data += FUZZ_SELECTOR_BYTES;
    size_t remain = size - FUZZ_SELECTOR_BYTES;
    AppEventHandler handler;
    FUZZ_FUNCS[selector](data, remain, handler);
}
} // namespace
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FuzzAppEventHandlerPostEvent(data, size);
    return 0;
}
