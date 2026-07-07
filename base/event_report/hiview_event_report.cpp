/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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
#include "hiview_event_report.h"

#include "hisysevent_util.h"
#include "hiview_event_common.h"
#include "hiview_event_cacher.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "plugin_fault_event_factory.h"
#include "plugin_load_event_factory.h"
#include "plugin_unload_event_factory.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(LABEL_DOMAIN, "HiView-HiviewEventReport");
void HiviewEventReport::ReportPluginLoad(const std::string &name, uint32_t result, uint32_t duration)
{
    auto factory = std::make_unique<PluginLoadEventFactory>();
    auto event = factory->Create();
    event->Update(PluginEventSpace::KEY_OF_PLUGIN_NAME, name);
    event->Update(PluginEventSpace::KEY_OF_RESULT, result);
    event->Update(PluginEventSpace::KEY_OF_DURATION, duration);
    event->Report();
    HIVIEW_LOGI("report plugin load event=%{public}s", event->ToJsonString().c_str());
}

void HiviewEventReport::ReportPluginUnload(const std::string &name, uint32_t result)
{
    auto factory = std::make_unique<PluginUnloadEventFactory>();
    auto event = factory->Create();
    event->Update(PluginEventSpace::KEY_OF_PLUGIN_NAME, name);
    event->Update(PluginEventSpace::KEY_OF_RESULT, result);
    event->Report();
    HIVIEW_LOGI("report plugin unload event=%{public}s", event->ToJsonString().c_str());
}

void HiviewEventReport::ReportPluginFault(const std::string &name, const std::string &reason)
{
    auto factory = std::make_unique<PluginFaultEventFactory>();
    auto event = factory->Create();
    event->Update(PluginFaultEventSpace::KEY_OF_PLUGIN_NAME, name);
    event->Update(PluginFaultEventSpace::KEY_OF_REASON, reason);
    event->Report();
    HIVIEW_LOGI("report plugin fault event=%{public}s", event->ToJsonString().c_str());
}

void HiviewEventReport::ReportPluginStats()
{
    std::vector<std::shared_ptr<LoggerEvent>> events;
    HiviewEventCacher::GetInstance().GetPluginStatsEvents(events);
    for (auto event : events) {
        event->Report();
        HIVIEW_LOGI("report plugin stat event=%{public}s", event->ToJsonString().c_str());
    }
    HiviewEventCacher::GetInstance().ClearPluginStatsEvents();
}

void HiviewEventReport::UpdatePluginStats(const std::string &name, const std::string &procName, uint32_t procTime)
{
    HIVIEW_LOGD("UpdatePluginStats pluginName=%{public}s, procName=%{public}s, time=%{public}d",
        name.c_str(), procName.c_str(), procTime);
    HiviewEventCacher::GetInstance().UpdatePluginStatsEvent(name, procName, procTime);
}

void HiviewEventReport::ReportCpuScene(const std::string &sceneId)
{
    if (!Parameter::IsBetaVersion()) {
        HIVIEW_LOGD("no need to report cpu scene event");
        return;
    }
    std::string packageName = "hiview";
    HiSysEventParam params[] = {
        BUILD_PARAM("PACKAGE_NAME", HISYSEVENT_STRING, s, PARAM_STR(packageName)),
        BUILD_PARAM("SCENE_ID", HISYSEVENT_STRING, s, PARAM_STR(sceneId)),
        BUILD_PARAM("HAPPEN_TIME", HISYSEVENT_UINT64, ui64, TimeUtil::GetMilliseconds()),
    };
    int ret = OH_HiSysEvent_Write(CpuSceneEvent::DOMAIN, "CPU_SCENE_ENTRY", HISYSEVENT_BEHAVIOR,
        params, sizeof(params) / sizeof(HiSysEventParam));
    if (ret != 0) {
        HIVIEW_LOGW("failed to report cpu scene event, sceneId=%{public}s, ret=%{public}d", sceneId.c_str(), ret);
    } else {
        HIVIEW_LOGI("succ to report cpu scene event, sceneId=%{public}s", sceneId.c_str());
    }
}
} // namespace HiviewDFX
} // namespace OHOS
