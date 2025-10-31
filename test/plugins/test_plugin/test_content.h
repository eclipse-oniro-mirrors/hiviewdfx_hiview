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

#ifndef HIVIEWDFX_HIVIEW_TRACE_TEST_CONTENT_H
#define HIVIEWDFX_HIVIEW_TRACE_TEST_CONTENT_H
#include "common_event_manager.h"

namespace OHOS::HiviewDFX {

void TestXperfDump(const EventFwk::CommonEventData &data);
void TestReliabilityDump(const EventFwk::CommonEventData &data);
void TestXpowerDump(const EventFwk::CommonEventData &data);
void TestXperfExDump(const EventFwk::CommonEventData &data);
void TestScreenDump(const EventFwk::CommonEventData &data);
void TestTelemetryStart(const EventFwk::CommonEventData &data);
void TestTelemetryEnd(const EventFwk::CommonEventData &data);
void TelemetryDumpXpower(const EventFwk::CommonEventData &data);
void TelemetryDumpXPerf(const EventFwk::CommonEventData &data);
void TestTraceOnXpower(const EventFwk::CommonEventData &data);
void TestTraceOffXpower(const EventFwk::CommonEventData &data);
void TestTraceOnXPerf(const EventFwk::CommonEventData &data);
void TestTraceOffXPerf(const EventFwk::CommonEventData &data);
void GetFileAttr(const EventFwk::CommonEventData &data);
}
#endif // HIVIEWDFX_HIVIEW_TRACE_TEST_CONTENT_H
