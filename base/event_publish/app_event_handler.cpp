/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "app_event_handler.h"

#include <sstream>

#include "bundle_mgr_client.h"
#include "event_publish.h"
#include "hiview_logger.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-AppEventHandler");

int32_t GetUidByBundleName(const std::string& bundleName)
{
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::GET_BUNDLE_DEFAULT, info,
        AppExecFwk::Constants::ALL_USERID)) {
        HIVIEW_LOGE("Failed to query uid from bms.");
    } else {
        HIVIEW_LOGD("bundleName of uid=%{public}d", info.uid);
    }
    return info.uid;
}

template <typename T>
void AddVectorToJsonString(const std::string& key, const std::vector<T>& vec, std::stringstream& jsonStr,
    bool isLastValue = false)
{
    jsonStr << "\"" << key << "\":";
    if (vec.empty()) {
        jsonStr << "[]" << (isLastValue ? "}" : ",");
        return;
    }
    jsonStr << "[";
    std::copy(vec.begin(), vec.end() - 1, std::ostream_iterator<T>(jsonStr, ","));
    jsonStr << vec.back() << "]" << (isLastValue ? "}" : ",");
}

template <typename T>
void AddValueToJsonString(const std::string& key, const T& value, std::stringstream& jsonStr, bool isLastValue = false)
{
    jsonStr << "\"" << key << "\":";
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        jsonStr << "\"" << value << "\"";
    } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
        jsonStr << (value ? "true" : "false");
    } else {
        jsonStr << value;
    }
    jsonStr << (isLastValue ? "}" : ",");
}

void AddObjectToJsonString(const std::string& name, std::stringstream& jsonStr)
{
    jsonStr << "\"" << name << "\":{";
}

void AddTimeToJsonString(std::stringstream& jsonStr)
{
    auto time = TimeUtil::GetMilliseconds();
    AddValueToJsonString("time", time, jsonStr);
}

void AddBundleInfoToJsonString(const AppEventHandler::BundleInfo& event, std::stringstream& jsonStr)
{
    AddValueToJsonString("bundle_version", event.bundleVersion, jsonStr);
    AddValueToJsonString("bundle_name", event.bundleName, jsonStr);
}

void AddProcessInfoToJsonString(const AppEventHandler::ProcessInfo& event, std::stringstream& jsonStr)
{
    AddValueToJsonString("process_name", event.processName, jsonStr);
}

void AddAbilityInfoToJsonString(const AppEventHandler::AbilityInfo& event, std::stringstream& jsonStr)
{
    AddValueToJsonString("ability_name", event.abilityName, jsonStr);
}
}

int AppEventHandler::PostEvent(const AppLaunchInfo& event)
{
    if (event.bundleName.empty()) {
        HIVIEW_LOGW("bundleName empty.");
        return -1;
    }
    int32_t uid = GetUidByBundleName(event.bundleName);
    std::stringstream jsonStr;
    jsonStr << "{";
    AddTimeToJsonString(jsonStr);
    AddBundleInfoToJsonString(event, jsonStr);
    AddProcessInfoToJsonString(event, jsonStr);
    AddValueToJsonString("start_type", event.startType, jsonStr);
    AddValueToJsonString("icon_input_time", event.iconInputTime, jsonStr);
    AddValueToJsonString("animation_finish_time", event.animationFinishTime, jsonStr);
    AddValueToJsonString("extend_time", event.extendTime, jsonStr);
    AddValueToJsonString("response_latency", event.responseLatency, jsonStr);
    AddValueToJsonString("laun_to_start_ability_dur", event.launToStartAbilityDur, jsonStr);
    AddValueToJsonString("startability_processstart_dur", event.startAbilityProcessStartDur, jsonStr);
    AddValueToJsonString("processstart_to_appattach_dur", event.processStartToAppAttachDur, jsonStr);
    AddValueToJsonString("appattach_to_appforeground_dur", event.appAttachToAppForegroundDur, jsonStr);
    AddValueToJsonString("startability_appforeground_dur", event.startAbilityAppForegroundDur, jsonStr);
    AddValueToJsonString("appforegr_abilityonforegr_dur", event.appForegrAbilityOnForegrDur, jsonStr);
    AddValueToJsonString("abilityonforeg_startwindow_dur", event.abilityOnForegStartWindowDur, jsonStr, true);
    jsonStr << std::endl;
    EventPublish::GetInstance().PushEvent(uid, "APP_LAUNCH", HiSysEvent::EventType::BEHAVIOR, jsonStr.str());
    return 0;
}

int AppEventHandler::PostEvent(const ScrollJankInfo& event)
{
    if (event.bundleName.empty()) {
        HIVIEW_LOGW("bundleName empty.");
        return -1;
    }
    int32_t uid = GetUidByBundleName(event.bundleName);
    std::stringstream jsonStr;
    jsonStr << "{";
    AddTimeToJsonString(jsonStr);
    AddBundleInfoToJsonString(event, jsonStr);
    AddProcessInfoToJsonString(event, jsonStr);
    AddAbilityInfoToJsonString(event, jsonStr);
    AddValueToJsonString("begin_time", event.beginTime, jsonStr);
    AddValueToJsonString("duration", event.duration, jsonStr);
    AddValueToJsonString("total_app_frames", event.totalAppFrames, jsonStr);
    AddValueToJsonString("total_app_missed_frames", event.totalAppMissedFrames, jsonStr);
    AddValueToJsonString("max_app_frametime", event.maxAppFrametime, jsonStr);
    AddValueToJsonString("max_app_seq_frames", event.maxAppSeqFrames, jsonStr);
    AddValueToJsonString("total_render_frames", event.totalRenderFrames, jsonStr);
    AddValueToJsonString("total_render_missed_frames", event.totalRenderMissedFrames, jsonStr);
    AddValueToJsonString("max_render_frametime", event.maxRenderFrametime, jsonStr);
    AddValueToJsonString("max_render_seq_frames", event.maxRenderSeqFrames, jsonStr);
    AddVectorToJsonString("external_log", event.externalLog, jsonStr);
    AddValueToJsonString("log_over_limit", event.logOverLimit, jsonStr, true);
    jsonStr << std::endl;
    EventPublish::GetInstance().PushEvent(uid, "SCROLL_JANK", HiSysEvent::EventType::FAULT, jsonStr.str());
    return 0;
}

int AppEventHandler::PostEvent(const ResourceOverLimitInfo& event)
{
    if (event.bundleName.empty()) {
        HIVIEW_LOGW("bundleName empty.");
        return -1;
    }
    std::stringstream jsonStr;
    jsonStr << "{";
    AddTimeToJsonString(jsonStr);
    AddBundleInfoToJsonString(event, jsonStr);
    AddValueToJsonString("pid", event.pid, jsonStr);
    AddValueToJsonString("uid", event.uid, jsonStr);
    AddValueToJsonString("resource_type", event.resourceType, jsonStr);
    AddValueToJsonString("app_running_unique_id", event.appRunningUniqueId, jsonStr);
    if (event.resourceType == "pss_memory" || event.resourceType == "ion_memory" ||
        event.resourceType == "gpu_memory") {
        AddObjectToJsonString("memory", jsonStr);
        AddValueToJsonString("pss", event.pss, jsonStr);
        AddValueToJsonString("rss", event.rss, jsonStr);
        AddValueToJsonString("vss", event.vss, jsonStr);
        AddValueToJsonString("gpu", event.gpu, jsonStr);
        AddValueToJsonString("ion", event.ion, jsonStr);
        AddValueToJsonString("sys_avail_mem", event.avaliableMem, jsonStr);
        AddValueToJsonString("sys_free_mem", event.freeMem, jsonStr);
        AddValueToJsonString("sys_total_mem", event.totalMem, jsonStr, true);
        jsonStr << "," << std::endl;
    } else if (event.resourceType == "js_heap") {
        AddObjectToJsonString("memory", jsonStr);
        AddValueToJsonString("limit_size", event.limitSize, jsonStr);
        AddValueToJsonString("live_object_size", event.liveobjectSize, jsonStr, true);
        jsonStr << "," << std::endl;
    } else if (event.resourceType == "fd") {
        AddObjectToJsonString("fd", jsonStr);
        AddValueToJsonString("num", event.fdNum, jsonStr);
        AddValueToJsonString("top_fd_type", event.topFdType, jsonStr);
        AddValueToJsonString("top_fd_num", event.topFdNum, jsonStr, true);
        jsonStr << "," << std::endl;
    } else if (event.resourceType == "thread") {
        AddObjectToJsonString("thread", jsonStr);
        AddValueToJsonString("num", event.threadNum, jsonStr, true);
        jsonStr << "," << std::endl;
    } else {
        return -1;
    }
    AddVectorToJsonString("external_log", event.logPath, jsonStr, true);

    EventPublish::GetInstance().PushEvent(event.uid, "RESOURCE_OVERLIMIT", HiSysEvent::EventType::FAULT, jsonStr.str());
    return 0;
}

int AppEventHandler::PostEvent(const CpuUsageHighInfo& event)
{
    if (event.bundleName.empty()) {
        HIVIEW_LOGW("bundleName empty");
        return -1;
    }
    int32_t uid = GetUidByBundleName(event.bundleName);
    std::stringstream jsonStr;
    jsonStr << "{";
    AddTimeToJsonString(jsonStr);
    AddBundleInfoToJsonString(event, jsonStr);
    AddValueToJsonString("foreground", event.isForeground, jsonStr);
    AddValueToJsonString("usage", event.usage, jsonStr);
    AddValueToJsonString("begin_time", event.beginTime, jsonStr);
    AddValueToJsonString("fault_type", event.faultType, jsonStr);
    AddValueToJsonString("end_time", event.endTime, jsonStr);
    AddVectorToJsonString("threads", event.threads, jsonStr);
    AddVectorToJsonString("external_log", event.externalLog, jsonStr);
    AddValueToJsonString("log_over_limit", event.logOverLimit, jsonStr, true);
    jsonStr << std::endl;
    EventPublish::GetInstance().PushEvent(uid, "CPU_USAGE_HIGH", HiSysEvent::EventType::FAULT, jsonStr.str());
    return 0;
}

int AppEventHandler::PostEvent(const BatteryUsageInfo& event)
{
    if (event.bundleName.empty()) {
        HIVIEW_LOGW("bundleName empty");
        return -1;
    }
    int32_t uid = GetUidByBundleName(event.bundleName);
    std::stringstream jsonStr;
    jsonStr << "{";
    AddTimeToJsonString(jsonStr);
    AddBundleInfoToJsonString(event, jsonStr);
    AddValueToJsonString("begin_time", event.beginTime, jsonStr);
    AddValueToJsonString("end_time", event.endTime, jsonStr);
    AddVectorToJsonString("foreground_usage", event.usage.fgUsages, jsonStr);
    AddVectorToJsonString("background_usage", event.usage.bgUsages, jsonStr);
    AddVectorToJsonString("cpu_foreground_energy", event.cpuEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("cpu_background_energy", event.cpuEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("gpu_foreground_energy", event.gpuEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("gpu_background_energy", event.gpuEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("ddr_foreground_energy", event.ddrEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("ddr_background_energy", event.ddrEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("display_foreground_energy", event.displayEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("display_background_energy", event.displayEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("audio_foreground_energy", event.audioEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("audio_background_energy", event.audioEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("modem_foreground_energy", event.modemEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("modem_background_energy", event.modemEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("rom_foreground_energy", event.romEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("rom_background_energy", event.romEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("wifi_foreground_energy", event.wifiEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("wifi_background_energy", event.wifiEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("sensor_foreground_energy", event.sensorEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("sensor_background_energy", event.sensorEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("gps_foreground_energy", event.gpsEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("gps_background_energy", event.gpsEnergy.bgUsages, jsonStr);
    AddVectorToJsonString("others_foreground_energy", event.othersEnergy.fgUsages, jsonStr);
    AddVectorToJsonString("others_background_energy", event.othersEnergy.bgUsages, jsonStr, true);
    jsonStr << std::endl;
    EventPublish::GetInstance().PushEvent(uid, "BATTERY_USAGE", HiSysEvent::EventType::STATISTIC, jsonStr.str());
    return 0;
}

int AppEventHandler::PostEvent(const AppKilledInfo& event)
{
    if (event.bundleName.empty() && event.uid == 0) {
        HIVIEW_LOGE("invalid bundleName and uid");
        return -1;
    }
    int32_t uid;
    if (event.uid != 0) {
        uid = event.uid;
    } else {
        uid = GetUidByBundleName(event.bundleName);
        HIVIEW_LOGI("get uid: %{public}d by bundleName: %{public}s", uid, event.bundleName.c_str());
    }

    std::stringstream jsonStr;
    jsonStr << "{";
    AddTimeToJsonString(jsonStr);
    AddValueToJsonString("reason", event.reason, jsonStr);
    AddValueToJsonString("foreground", event.isForeground, jsonStr, true);
    jsonStr << std::endl;
    EventPublish::GetInstance().PushEvent(uid, "APP_KILLED", HiSysEvent::EventType::STATISTIC, jsonStr.str());
    return 0;
}

int AppEventHandler::PostEvent(const AudioJankFrameInfo& event)
{
    if (event.bundleName.empty()) {
        HIVIEW_LOGW("bundleName empty.");
        return -1;
    }
    int32_t uid = GetUidByBundleName(event.bundleName);
    std::stringstream jsonStr;
    jsonStr << "{";
    AddTimeToJsonString(jsonStr);
    AddBundleInfoToJsonString(event, jsonStr);
    AddValueToJsonString("max_frame_time", event.maxFrameTime, jsonStr);
    AddValueToJsonString("happen_time", event.happenTime, jsonStr);
    AddValueToJsonString("fault_type", event.faultType, jsonStr, true);
    jsonStr << std::endl;
    EventPublish::GetInstance().PushEvent(uid, "AUDIO_JANK_FRAME", HiSysEvent::EventType::FAULT, jsonStr.str());
    return 0;
}

bool AppEventHandler::IsAppListenedEvent(int32_t uid, const std::string& eventName)
{
    return EventPublish::GetInstance().IsAppListenedEvent(uid, eventName);
}
} // namespace HiviewDFX
} // namespace OHOS