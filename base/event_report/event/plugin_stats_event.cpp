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
#include "plugin_stats_event.h"

#include "hisysevent_util.h"
#include "hiview_event_common.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int TOP_K_NUM = 3;
constexpr uint32_t NUM_ADD_PER = 1;
const std::vector<uint32_t> INIT_TOP_K_TIME(TOP_K_NUM, 0);
const std::vector<std::string> INIT_TOP_K_EVENT(TOP_K_NUM, "");
}
using namespace PluginStatsEventSpace;

PluginStatsEvent::PluginStatsEvent(const std::string &name, HiSysEvent::EventType type)
    : LoggerEvent(name, type)
{
    paramMap_ = {
        {std::string(KEY_OF_PLUGIN_NAME), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_PROC_NAME), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_PROC_TIME), DEFAULT_UINT32}, {std::string(KEY_OF_TOTAL_TIME), DEFAULT_UINT64},
        {std::string(KEY_OF_AVG_TIME), DEFAULT_UINT32}, {std::string(KEY_OF_TOP_K_TIME), INIT_TOP_K_TIME},
        {std::string(KEY_OF_TOP_K_EVENT), INIT_TOP_K_EVENT}, {std::string(KEY_OF_TOTAL), DEFAULT_UINT32}
    };
}

void PluginStatsEvent::UpdateTotalNum()
{
    paramMap_[KEY_OF_TOTAL] = paramMap_[KEY_OF_TOTAL].GetUint32() + NUM_ADD_PER;
}

void PluginStatsEvent::UpdateTotalTime()
{
    paramMap_[KEY_OF_TOTAL_TIME] = paramMap_[KEY_OF_TOTAL_TIME].GetUint64() + paramMap_[KEY_OF_PROC_TIME].GetUint32();
}

void PluginStatsEvent::UpdateAvgTime()
{
    paramMap_[KEY_OF_AVG_TIME] = static_cast<uint32_t>(paramMap_[KEY_OF_TOTAL_TIME].GetUint64()
        / paramMap_[KEY_OF_TOTAL].GetUint32());
}

void PluginStatsEvent::UpdateTopK()
{
    uint32_t procTime = paramMap_[KEY_OF_PROC_TIME].GetUint32();
    auto topKTime = paramMap_[KEY_OF_TOP_K_TIME].GetUint32Vec();
    if (procTime <= topKTime.back()) {
        return;
    }

    auto it = std::find_if(topKTime.begin(), topKTime.end(), [procTime] (auto t) {
        return procTime > t;
    });
    auto index = it - topKTime.begin();
    topKTime.insert(topKTime.begin() + index, procTime);
    topKTime.pop_back();
    paramMap_[KEY_OF_TOP_K_TIME] = topKTime;

    auto topKEvent = paramMap_[KEY_OF_TOP_K_EVENT].GetStringVec();
    topKEvent.insert(topKEvent.begin() + index, paramMap_[KEY_OF_PROC_NAME].GetString());
    topKEvent.pop_back();
    paramMap_[KEY_OF_TOP_K_EVENT] = topKEvent;
}

void PluginStatsEvent::InnerUpdate(const std::string &name, const ParamValue& value)
{
    LoggerEvent::InnerUpdate(name, value);
    if (name.compare(KEY_OF_PROC_TIME) == 0) {
        UpdateTotalNum();
        UpdateTotalTime();
        UpdateAvgTime();
        UpdateTopK();
    }
}

void PluginStatsEvent::Report()
{
    std::string pluginName = paramMap_[KEY_OF_PLUGIN_NAME].GetString();
    std::vector<std::string> topKEvents = paramMap_[KEY_OF_TOP_K_EVENT].GetStringVec();
    std::vector<char*> translatedTopKEvents;
    TranslateStrVector(topKEvents, translatedTopKEvents);
    HiSysEventParam params[] = {
        BUILD_PARAM(PLUGIN_NAME_LITERAL, HISYSEVENT_STRING, s, PARAM_STR(pluginName)),
        BUILD_PARAM(AVG_TIME_LITERAL, HISYSEVENT_UINT32, ui32, paramMap_[KEY_OF_AVG_TIME].GetUint32()),
        BUILD_ARRAY_PARAM(TOP_K_TIME_LITERAL, HISYSEVENT_UINT32_ARRAY, uint32_t,
            paramMap_[KEY_OF_TOP_K_TIME].GetUint32Vec()),
        BUILD_ARRAY_PARAM(TOP_K_EVENT_LITERAL, HISYSEVENT_STRING_ARRAY, char*, translatedTopKEvents),
        BUILD_PARAM(TOTAL_LITERAL, HISYSEVENT_UINT32, ui32, paramMap_[KEY_OF_TOTAL].GetUint32()),
    };
    (void)OH_HiSysEvent_Write(HiSysEvent::Domain::HIVIEWDFX, eventName_.c_str(),
        TranslateEventType(eventType_), params, sizeof(params) / sizeof(HiSysEventParam));
}
} // namespace HiviewDFX
} // namespace OHOS
