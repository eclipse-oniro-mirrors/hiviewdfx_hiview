/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "hiviewservice_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "fuzz_data_source.h"
#include "hiview_service.h"
#include "hiview_platform.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
HiviewService g_hiviewService;
int g_fd = 0;
constexpr size_t BINARY_MOD = 2;
constexpr size_t MAX_STR_LEN = 100;
constexpr int32_t HIVIEW_UID = 1201;

struct Initializer {
    Initializer()
    {
        g_fd = open("/dev/null", O_RDWR | O_CREAT | O_TRUNC, 0600); //0600 for file mode
        if (g_fd <= 0) {
            printf("failed to open file, exit\n");
            isInit = false;
            return;
        }
        isInit = true;
    }

    ~Initializer()
    {
        if (isInit) {
            (void)close(g_fd);
        }
    }

    bool isInit = false;
};
Initializer g_initializer;
}

static void HiviewServiceDumpFuzzTest(const uint8_t* data, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    std::string strData = std::string(reinterpret_cast<const char*>(data), size);
    g_hiviewService.DumpRequestDispatcher(g_fd, {});
    g_hiviewService.DumpRequestDispatcher(g_fd, {strData});
}

static void HiViewServiceSnapshotTraceFuzzTest(const uint8_t* data, size_t size)
{
    std::string tag = std::string(reinterpret_cast<const char*>(data), size);
    std::vector<std::string> tags;
    tags.push_back(tag);
    UCollectClient::TraceParam param;
    std::vector<int32_t> filterPids;
    g_hiviewService.OpenTrace(tags, param, filterPids);
}

static void HiViewServiceDumpSnapshotTraceFuzzTest(const uint8_t* data, size_t size)
{
    std::string tag = std::string(reinterpret_cast<const char*>(data), size);
    g_hiviewService.DumpSnapshotTrace("fuzzTest" + tag, false);
}

static void HiViewServiceFileOperateFuzzTest(const uint8_t* data, size_t size)
{
    std::string srcFile = std::string(reinterpret_cast<const char*>(data), size);
    std::string destFile = srcFile + "_dest";
    (void)g_hiviewService.Copy(srcFile, destFile);
    (void)g_hiviewService.Move(srcFile, destFile);
    (void)g_hiviewService.Remove(destFile);
    (void)g_hiviewService.Remove(srcFile);
}

static void HiViewServiceRecordingTraceFuzzTest(const uint8_t* data, size_t size)
{
    std::string outputPath = std::string(reinterpret_cast<const char*>(data), size);
    (void)g_hiviewService.RecordingTraceOn(outputPath);
    (void)g_hiviewService.RecordingTraceOff();
    (void)g_hiviewService.CloseTrace();
}

static void HiViewServiceCaptureDurationTraceFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    UCollectClient::AppCaller appCaller;
    (void)source.GetValue(appCaller.actionId);
    (void)source.GetValue(appCaller.foreground);
    (void)source.GetValue(appCaller.uid);
    (void)source.GetValue(appCaller.pid);
    (void)source.GetValue(appCaller.happenTime);
    (void)source.GetValue(appCaller.beginTime);
    (void)source.GetValue(appCaller.endTime);
    uint8_t isBusinessJank = 0;
    (void)source.GetValue(isBusinessJank);
    appCaller.isBusinessJank = (isBusinessJank % BINARY_MOD == 0);
    std::string strData;
    (void)source.GetString(strData, MAX_STR_LEN);
    appCaller.bundleName = strData;
    appCaller.bundleVersion = strData;
    appCaller.threadName = strData;
    (void)g_hiviewService.CaptureDurationTrace(appCaller);
}

static void HiViewServiceSysCpuUsageFuzzTest(const uint8_t* data, size_t size)
{
    (void)data;
    (void)size;
    (void)g_hiviewService.GetSysCpuUsage();
}

static void HiViewServiceMemoryFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    UCollectClient::MemoryCaller memoryCaller;
    (void)source.GetValue(memoryCaller.pid);
    (void)source.GetValue(memoryCaller.limitValue);
    uint8_t enabledDebugLog = 0;
    (void)source.GetValue(enabledDebugLog);
    memoryCaller.enabledDebugLog = (enabledDebugLog % BINARY_MOD == 0);
    std::string strData;
    (void)source.GetString(strData, MAX_STR_LEN);
    memoryCaller.resourceType = strData;
    (void)g_hiviewService.SetAppResourceLimit(memoryCaller);
    std::vector<UCollectClient::MemoryCaller> memList = {memoryCaller};
    (void)g_hiviewService.SetSplitMemoryValue(memList);
    int32_t graphicPid = 0;
    (void)source.GetValue(graphicPid);
    (void)g_hiviewService.GetGraphicUsage(graphicPid);
}

static void HiViewServiceForkDumpFuzzTest(const uint8_t* data, size_t size)
{
    std::string packageName = std::string(reinterpret_cast<const char*>(data), size);
    int32_t mainProcPid = static_cast<int32_t>(getpid());
    int32_t subProcPid = mainProcPid + 1;
    (void)g_hiviewService.SetForkDumpService(packageName, mainProcPid, subProcPid);
}
} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (!OHOS::HiviewDFX::g_initializer.isInit) {
        printf("failed to init environment, exit\n");
        return 0;
    }
    setuid(OHOS::HiviewDFX::HIVIEW_UID); // hiview uid
    OHOS::HiviewDFX::HiviewServiceDumpFuzzTest(data, size);
    OHOS::HiviewDFX::HiViewServiceSnapshotTraceFuzzTest(data, size);
    OHOS::HiviewDFX::HiViewServiceDumpSnapshotTraceFuzzTest(data, size);
    OHOS::HiviewDFX::HiViewServiceFileOperateFuzzTest(data, size);
    OHOS::HiviewDFX::HiViewServiceRecordingTraceFuzzTest(data, size);
    OHOS::HiviewDFX::HiViewServiceCaptureDurationTraceFuzzTest(data, size);
    OHOS::HiviewDFX::HiViewServiceSysCpuUsageFuzzTest(data, size);
    OHOS::HiviewDFX::HiViewServiceMemoryFuzzTest(data, size);
    OHOS::HiviewDFX::HiViewServiceForkDumpFuzzTest(data, size);
    return 0;
}
