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

#include "common_parcelable_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "app_caller_parcelable.h"
#include "fuzz_data_source.h"
#include "graphic_usage_parcelable.h"
#include "message_parcel.h"
#include "trace_param_parcelable.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr size_t BINARY_MOD = 2;
constexpr size_t MAX_STR_LEN = 100;

UCollectClient::AppCaller GenerateAppCaller(FuzzDataSource& source)
{
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
    return appCaller;
}
}

static void AppCallerParcelableFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    AppCallerParcelable parcelable(GenerateAppCaller(source));
    (void)parcelable.GetAppCaller().actionId;

    MessageParcel parcel;
    parcelable.Marshalling(parcel);
    auto* unmarshalled = AppCallerParcelable::Unmarshalling(parcel);
    if (unmarshalled != nullptr) {
        (void)unmarshalled->GetAppCaller().bundleName;
        delete unmarshalled;
    }
}

static void TraceParamParcelableFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    UCollectClient::TraceParam traceParam;
    (void)source.GetValue(traceParam.bufferSize);
    (void)source.GetValue(traceParam.fileSizeLimit);
    (void)source.GetValue(traceParam.totalSize);
    uint8_t isOverWrite = 0;
    (void)source.GetValue(isOverWrite);
    traceParam.isOverWrite = (isOverWrite % BINARY_MOD == 0);
    std::string strData;
    (void)source.GetString(strData, MAX_STR_LEN);
    traceParam.clockType = strData;
    TraceParamParcelable parcelable(traceParam);
    (void)parcelable.GetTraceParam().clockType;

    MessageParcel parcel;
    parcelable.Marshalling(parcel);
    auto* unmarshalled = TraceParamParcelable::Unmarshalling(parcel);
    if (unmarshalled != nullptr) {
        (void)unmarshalled->GetTraceParam().bufferSize;
        delete unmarshalled;
    }
}

static void GraphicUsageParcelableFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    UCollectClient::GraphicUsage graphicUsage;
    (void)source.GetValue(graphicUsage.gl);
    int32_t graphValue = 0;
    (void)source.GetValue(graphValue);
    graphicUsage.graph = graphValue + 1;
    GraphicUsageParcelable parcelable(graphicUsage);
    (void)parcelable.GetGraphicUsage().gl;

    GraphicUsageParcelable defaultParcelable;
    defaultParcelable.UpdateGraphicUsage(graphicUsage);
    (void)defaultParcelable.GetGraphicUsage().graph;

    MessageParcel parcel;
    parcelable.Marshalling(parcel);
    auto* unmarshalled = GraphicUsageParcelable::Unmarshalling(parcel);
    if (unmarshalled != nullptr) {
        (void)unmarshalled->GetGraphicUsage().gl;
        delete unmarshalled;
    }
}
} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::AppCallerParcelableFuzzTest(data, size);
    OHOS::HiviewDFX::TraceParamParcelableFuzzTest(data, size);
    OHOS::HiviewDFX::GraphicUsageParcelableFuzzTest(data, size);
    return 0;
}
