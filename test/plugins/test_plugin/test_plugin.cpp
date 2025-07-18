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
#include "test_plugin.h"

#include "cjson_util.h"
#include "common_event_manager.h"
#include "hiview_logger.h"
#include "hiview_global.h"
#include "plugin_factory.h"
#include "test_content.h"
#include "trace_collector.h"

namespace OHOS::HiviewDFX {
namespace {
REGISTER(TraceTestPlugin);
DEFINE_LOG_TAG("TraceTestPlugin");

const std::map<std::string, std::function<void(const CommonEventData &)>> TEST_CONTENT = {
    {"xperf_dump", TestXperfDump},
    {"reliability_dump", TestReliabilityDump},
    {"xpower_dump", TestXpowerDump},
    {"other_dump", TestOtherDump},
    {"screen_dump", TestScreenDump},
    {"telemetryStart", TestTelemetryStart},
    {"telemetryEnd", TestTelemetryEnd},
    {"traceOnXpower", TestTraceOnXpower},
    {"traceOffXpower", TestTraceOffXpower},
    {"traceOnXPerf", TestTraceOnXPerf},
    {"traceOffXPerf", TestTraceOffXPerf},
    {"telemetryDumpXpower", TelemetryDumpXpower},
    {"telemetryDumpXPerf", TelemetryDumpXPerf}
};
}
using namespace UCollectUtil;
using namespace UCollect;

void TraceTestPlugin::OnLoad()
{
    HIVIEW_LOGI("Register matchingSkills");
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent("telemetryStart");
    matchingSkills.AddEvent("telemetryEnd");
    matchingSkills.AddEvent("telemetryDumpXpower");
    matchingSkills.AddEvent("telemetryDumpXPerf");
    matchingSkills.AddEvent("traceOnXpower");
    matchingSkills.AddEvent("traceOffXpower");
    matchingSkills.AddEvent("traceOnXPerf");
    matchingSkills.AddEvent("traceOffXPerf");
    matchingSkills.AddEvent("xperf_dump");
    matchingSkills.AddEvent("reliability_dump");
    matchingSkills.AddEvent("xpower_dump");
    matchingSkills.AddEvent("other_dump");
    matchingSkills.AddEvent("screen_dump");

    CommonEventSubscribeInfo info(matchingSkills);
    telemetrySubscriber_ = std::make_shared<TestSubscriber>(info);
    if (!CommonEventManager::SubscribeCommonEvent(telemetrySubscriber_)) {
        HIVIEW_LOGE("register telemetrySubscriber_ fail");
    }
}

bool TraceTestPlugin::OnEvent(std::shared_ptr<Event> &event)
{
    HIVIEW_LOGI("event:%{public}s", event->eventName_.c_str());
    return true;
}

void TraceTestPlugin::OnUnload()
{
    HIVIEW_LOGI("UnRegister matchingSkills");
    if (telemetrySubscriber_ != nullptr) {
        CommonEventManager::UnSubscribeCommonEvent(telemetrySubscriber_);
    }
}

void TestSubscriber::OnReceiveEvent(const CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto it = TEST_CONTENT.find(action);
    if (it == TEST_CONTENT.end()) {
        return;
    }
    it->second(data);
}

TestSubscriber::TestSubscriber(const CommonEventSubscribeInfo &subscribeInfo)
    : CommonEventSubscriber(subscribeInfo) {}

}
