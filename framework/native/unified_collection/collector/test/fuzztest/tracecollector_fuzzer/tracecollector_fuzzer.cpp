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

#include "tracecollector_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "fuzz_data_source.h"
#include "trace_decorator.h"
#include "utility/trace_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
constexpr size_t BINARY_MOD = 2;
constexpr size_t MAX_STR_LEN = 100;
constexpr int32_t HIVIEW_UID = 1201;
}
static void TraceCollectorFuzzTest(const uint8_t* data, size_t size)
{
#if defined(UNIFIED_COLLECTOR_TRACE_ENABLE)
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    uint32_t maxDuration = 0;
    (void)source.GetValue(maxDuration);
    uint64_t happenTime = 0;
    (void)source.GetValue(happenTime);

    // TraceCollector::Create returns a collector decorated by TraceDecorator,
    // which works with the trace collector impl and the trace strategies
    auto traceCollector = TraceCollector::Create();
    if (traceCollector == nullptr) {
        return;
    }
    (void)traceCollector->DumpTrace(UCollect::TraceCaller::RELIABILITY);
    (void)traceCollector->DumpTrace(UCollect::TraceCaller::HIVIEW);
    std::string strData;
    (void)source.GetString(strData, MAX_STR_LEN);
    uint8_t isOverflow = 0;
    (void)source.GetValue(isOverflow);
    (void)traceCollector->DumpTrace(strData, isOverflow % BINARY_MOD == 0);
    (void)traceCollector->DumpTraceWithDuration(UCollect::TraceCaller::XPERF, maxDuration, happenTime);
    UCollectClient::AppCaller appCaller;
    (void)source.GetValue(appCaller.actionId);
    appCaller.bundleName = strData;
    appCaller.bundleVersion = strData;
    appCaller.threadName = strData;
    (void)source.GetValue(appCaller.foreground);
    (void)source.GetValue(appCaller.uid);
    (void)source.GetValue(appCaller.pid);
    (void)source.GetValue(appCaller.happenTime);
    (void)source.GetValue(appCaller.beginTime);
    (void)source.GetValue(appCaller.endTime);
    uint8_t isBusinessJank = 0;
    (void)source.GetValue(isBusinessJank);
    appCaller.isBusinessJank = (isBusinessJank % BINARY_MOD == 0);
    (void)traceCollector->DumpAppTrace(appCaller);
    (void)traceCollector->DumpTraceWithFilter(UCollect::TeleModule::XPOWER, maxDuration, happenTime);
    (void)traceCollector->FilterTraceOn(UCollect::TeleModule::XPERF, happenTime);
    (void)traceCollector->FilterTraceOff(UCollect::TeleModule::RELIABILITY);
    traceCollector->PrepareTrace();
    TraceDecorator::SaveStatSpecialInfo();
    TraceDecorator::ResetStatInfo();
#else
    (void)data;
    (void)size;
#endif
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }
    setuid(OHOS::HiviewDFX::UCollectUtil::HIVIEW_UID); // hiview uid
    OHOS::HiviewDFX::UCollectUtil::TraceCollectorFuzzTest(data, size);
    return 0;
}
