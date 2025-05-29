/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "hiview_platform.h"
#ifndef _WIN32
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <cinttypes>
#include <csignal>
#include <fstream>
#include <functional>

#include "app_event_publisher.h"
#include "app_event_publisher_factory.h"
#include "common_utils.h"
#include "defines.h"
#include "dispatch_rule_parser.h"
#include "dynamic_module.h"
#include "file_util.h"
#include "hiview_event_report.h"
#include "hiview_global.h"
#include "hiview_platform_config.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

#ifdef PARAM_UPDATE_ENABLE
#include "param_event_manager.h"
#endif

#include "plugin_bundle_config.h"
#include "plugin_config.h"
#include "plugin_factory.h"
#include "running_status_logger.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr uint32_t AID_SYSTEM = 1000;
static const char VERSION[] = "1.0.0.0";
static const char SEPARATOR_VERSION[] = " ";
static const char RECORDER_VERSION[] = "01.00";
static const char PLUGIN_CONFIG_NAME[] = "plugin_config";
static const char HIVIEW_PID_FILE_NAME[] = "hiview.pid";
static const char DEFAULT_CONFIG_DIR[] = "/system/etc/hiview/";
static const char PIPELINE_RULE_CONFIG_DIR[] = "/system/etc/hiview/dispatch_rule/";
static const char DISPATCH_RULE_CONFIG_DIR[] = "/system/etc/hiview/listener_rule/";
static const char DEFAULT_WORK_DIR[] = "/data/log/hiview/";
static const char DEFAULT_COMMERCIAL_WORK_DIR[] = "/log/LogService/";
static const char DEFAULT_PERSIST_DIR[] = "/log/hiview/";
static const char LIB_SEARCH_DIR[] = "/system/lib/";
static const char LIB64_SEARCH_DIR[] = "/system/lib64/";

void LogHiviewBootInfo(bool isStart)
{
    std::string info = "hiview ";
    info.append(isStart ? "begin" : "end").append("; time=[");
    info.append(std::to_string(TimeUtil::GetMilliseconds())).append("] ");
    RunningStatusLogger::GetInstance().LogEventRunningLogInfo(info);
}
}
DEFINE_LOG_TAG("HiView-HiviewPlatform");
HiviewPlatform::HiviewPlatform()
    : isReady_(false),
      defaultConfigDir_(DEFAULT_CONFIG_DIR),
      defaultWorkDir_(DEFAULT_WORK_DIR),
      defaultCommercialWorkDir_(DEFAULT_COMMERCIAL_WORK_DIR),
      defaultPersistDir_(DEFAULT_PERSIST_DIR),
      defaultConfigName_(PLUGIN_CONFIG_NAME),
      unorderQueue_(nullptr),
      sharedWorkLoop_(nullptr)
{
    dynamicLibSearchDir_.push_back(LIB_SEARCH_DIR);
    dynamicLibSearchDir_.push_back(LIB64_SEARCH_DIR);
}

HiviewPlatform::~HiviewPlatform()
{
    if (unorderQueue_ != nullptr) {
        unorderQueue_->Stop();
    }

    if (sharedWorkLoop_ != nullptr) {
        sharedWorkLoop_->StopLoop();
    }
    HiviewGlobal::ReleaseInstance();
}

void HiviewPlatform::SetMaxProxyIdleTime(time_t idleTime)
{
    maxIdleTime_ = idleTime;
}

void HiviewPlatform::SetCheckProxyIdlePeriod(time_t period)
{
    checkIdlePeriod_ = period;
}

bool HiviewPlatform::InitEnvironment(const std::string& platformConfigDir)
{
    LogHiviewBootInfo(true);
    // wait util the samgr is ready
    if (auto res = Parameter::WaitParamSync("bootevent.samgr.ready", "true", 5); res != 0) { // timeout is 5s
        HIVIEW_LOGE("Fail to wait the samgr, err=%{public}d", res);
        return false;
    }
    CreateWorkingDirectories(platformConfigDir);

    // update beta config
    UpdateBetaConfigIfNeed();

    // check whether hiview is already started
    ExitHiviewIfNeed();

    std::string cfgPath = GetPluginConfigPath();
    PluginConfig config(cfgPath);
    if (!config.StartParse()) {
        HIVIEW_LOGE("Fail to parse plugin config. exit!, cfgPath %{public}s", cfgPath.c_str());
        return false;
    }
    StartPlatformDispatchQueue();

    // init global context helper, remove in the future
    HiviewGlobal::CreateInstance(static_cast<HiviewContext&>(*this));
    LoadBusinessPlugin(config);

    // start load plugin bundles
    LoadPluginBundles();

    // maple delay start eventsource
    for (const auto& plugin : eventSourceList_) {
        auto sharedSource = std::static_pointer_cast<EventSource>(plugin);
        if (sharedSource == nullptr) {
            HIVIEW_LOGE("Fail to cast plugin to event source!");
            continue;
        }
        StartEventSource(sharedSource);
    }
    eventSourceList_.clear();
    isReady_ = true;
    // mark the finish of hiview initilization
    Parameter::SetProperty("hiviewdfx.hiview.ready", "1");
    NotifyPluginReady();
    ScheduleCheckUnloadablePlugins();

#ifdef PARAM_UPDATE_ENABLE
    OHOS::Singleton<ParamEventManager>::GetInstance().SubscriberEvent();
#endif
    LogHiviewBootInfo(false);

    return true;
}

void HiviewPlatform::CreateWorkingDirectories(const std::string& platformConfigDir)
{
    HiviewPlatformConfig platformConfig = HiviewPlatformConfig(platformConfigDir);
    HiviewPlatformConfig::PlatformConfigInfo platformConfigInfo;
    bool state = platformConfig.ParsesConfig(platformConfigInfo);
    if (state) {
        if (platformConfigInfo.defaultPluginConfigName != "") {
        defaultConfigName_ = platformConfigInfo.defaultPluginConfigName;
        }
        if (FileUtil::IsDirectory(platformConfigInfo.commercialWorkDir)) {
            defaultCommercialWorkDir_ = platformConfigInfo.commercialWorkDir;
        }
        if (platformConfigInfo.dynamicLib64SearchDir != ""
            || platformConfigInfo.dynamicLibSearchDir != "") {
            dynamicLibSearchDir_.clear();
        }
        if (platformConfigInfo.dynamicLib64SearchDir != "") {
            dynamicLibSearchDir_.push_back(platformConfigInfo.dynamicLib64SearchDir);
        }
        if (platformConfigInfo.dynamicLibSearchDir != "") {
            dynamicLibSearchDir_.push_back(platformConfigInfo.dynamicLibSearchDir);
        }
        ValidateAndCreateDirectories(platformConfigInfo.pluginConfigFileDir,
                                     platformConfigInfo.workDir,
                                     platformConfigInfo.persistDir);
    }
}

void HiviewPlatform::UpdateBetaConfigIfNeed()
{
}

void HiviewPlatform::LoadBusinessPlugin(const PluginConfig& config)
{
    // start to load plugin
    // 1. create plugin
    auto const& pluginInfoList = config.GetPluginInfoList();
    for (auto const& pluginInfo : pluginInfoList) {
        HIVIEW_LOGI("Start to create plugin %{public}s delay:%{public}d", pluginInfo.name.c_str(),
                    pluginInfo.loadDelay);
        if (pluginInfo.loadDelay > 0) {
            auto task = std::bind(&HiviewPlatform::ScheduleCreateAndInitPlugin, this, pluginInfo);
            sharedWorkLoop_->AddTimerEvent(nullptr, nullptr, task, pluginInfo.loadDelay, false);
        } else {
            CreatePlugin(pluginInfo);
        }
    }
    // 2. create pipelines
    auto const& pipelineInfoList = config.GetPipelineInfoList();
    for (auto const& pipelineInfo : pipelineInfoList) {
        HIVIEW_LOGI("Start to create pipeline %{public}s", pipelineInfo.name.c_str());
        CreatePipeline(pipelineInfo);
    }

    // 3. bind pipeline and call onload of event source
    for (auto const& pluginInfo : pluginInfoList) {
        HIVIEW_LOGI("Start to Load plugin %{public}s", pluginInfo.name.c_str());
        InitPlugin(config, pluginInfo);
    }

    // 4. delay start EventSource
    for (auto const& pluginInfo : pluginInfoList) {
        if (pluginInfo.isEventSource) {
            HIVIEW_LOGI("Start to Load eventSource %{public}s", pluginInfo.name.c_str());
            auto& plugin = pluginMap_[pluginInfo.name];
            auto sharedSource = std::static_pointer_cast<EventSource>(plugin);
            if (sharedSource == nullptr) {
                HIVIEW_LOGE("Fail to cast plugin to event source!");
                continue;
            }
            eventSourceList_.push_back(plugin);
        }
    }

    CleanupUnusedResources();
}

std::string HiviewPlatform::SplitBundleNameFromPath(const std::string& filePath)
{
        // the path should be like /system/etc/hiview/bundleName_plugin_config
        auto pos = filePath.find_last_of("/\\");
        if (pos == std::string::npos) {
            return "";
        }

        std::string fileName = filePath.substr(pos + 1);
        pos = fileName.find(defaultConfigName_);
        if (pos == std::string::npos || pos == 0) {
            return "";
        }

        pos = fileName.find_first_of("_");
        if (pos == std::string::npos) {
            return "";
        }

        return fileName.substr(0, pos);
}

std::string HiviewPlatform::SearchPluginBundle(const std::string& name) const
{
    for (const auto& pathPrefix : dynamicLibSearchDir_) {
        std::string bundlePath = pathPrefix  + GetDynamicLibName(name, true);
        printf("bundlePath is %s\n", bundlePath.c_str());
        if (FileUtil::FileExists(bundlePath)) {
            return bundlePath;
        }
    }
    return "";
}

void HiviewPlatform::LoadPluginBundle(const std::string& bundleName, const std::string& filePath)
{
    PluginConfig config(filePath);
    if (!config.StartParse()) {
        HIVIEW_LOGE("Fail to parse plugin config %{public}s, filePath is %{public}s",
            bundleName.c_str(), filePath.c_str());
        return;
    }

    std::string bundlePath = SearchPluginBundle(bundleName);
    if (bundlePath == "") {
        HIVIEW_LOGE("bundleName: %{public}s doesn't exist", bundleName.c_str());
        return;
    }
    auto handle = LoadModule(bundlePath);
    if (handle == DynamicModuleDefault) {
        return;
    }

    LoadBusinessPlugin(config);
    std::shared_ptr<PluginBundle> bundle = std::make_shared<PluginBundle>(bundleName, config, handle);
    pluginBundleInfos_.insert(std::pair<std::string, std::shared_ptr<PluginBundle>>(bundleName, bundle));
}

void HiviewPlatform::LoadPluginBundles()
{
    std::string bundlePath = defaultConfigDir_ + "bundle/plugin_bundle.json";
    std::string extBundlePath = defaultConfigDir_ + "bundle/plugin_bundle_ext.json";
    std::string realBundlepath = FileUtil::FileExists(extBundlePath) ? extBundlePath : bundlePath;
    PluginBundleConfig bundleConfig(realBundlepath);
    std::vector<std::string> bundleNames = bundleConfig.GetBundleNames();
    for (const auto& bundleName : bundleNames) {
        std::string configPath = defaultConfigDir_ + bundleName + "_plugin_config";
        LoadPluginBundle(bundleName, configPath);
    }
}

void HiviewPlatform::ProcessArgsRequest(int argc, char* argv[])
{
#ifndef _WIN32
    umask(0002); // 0002 is block other write permissions, -------w-
    signal(SIGPIPE, SIG_IGN);
    int ch = -1;
    while ((ch = getopt(argc, argv, "v")) != -1) {
        if (ch == 'v') {
            HIVIEW_LOGI("hiview version: %s%s%s", VERSION, SEPARATOR_VERSION, RECORDER_VERSION);
            printf("hiview version: %s%s%s\n", VERSION, SEPARATOR_VERSION, RECORDER_VERSION);
            _exit(1);
        }
    }
#endif // !_WIN32
}

DynamicModule HiviewPlatform::LoadDynamicPlugin(const std::string& name) const
{
    // if the plugin Class is AbcPlugin, the so name should be libabcplugin.z.so
    std::string dynamicPluginName = GetDynamicLibName(name, true);
    auto handle = LoadModule(dynamicPluginName.c_str());
    if (handle == nullptr) {
        // retry load library
        dynamicPluginName = GetDynamicLibName(name, false);
        handle = LoadModule(dynamicPluginName.c_str());
    }
    return handle;
}

std::string HiviewPlatform::GetDynamicLibName(const std::string& name, bool hasOhosSuffix) const
{
#ifdef __HIVIEW_OHOS__
    std::string tmp = "lib" + name;
    if (hasOhosSuffix) {
        tmp.append(".z.so");
    } else {
        tmp.append(".so");
    }

    for (unsigned i = 0; i < tmp.length(); i++) {
        tmp[i] = std::tolower(tmp[i]);
    }
    return tmp;
#elif defined(_WIN32)
    std::string dynamicLibName = "";
    dynamicLibName.append(name);
    dynamicLibName.append(".dll");
    return dynamicLibName;
#else
    // dynamic plugins feature is only enabled in double framework version
    (void)hasOhosSuffix;
    HIVIEW_LOGI("could not load dynamic lib %s, not supported yet.", name.c_str());
    return "";
#endif
}

void HiviewPlatform::CreatePlugin(const PluginConfig::PluginInfo& pluginInfo)
{
    if (pluginInfo.name.empty()) {
        return;
    }
    if (pluginMap_.find(pluginInfo.name) != pluginMap_.end()) {
        HiviewEventReport::ReportPluginLoad(pluginInfo.name, PluginEventSpace::LOAD_DUPLICATE_NAME);
        HIVIEW_LOGW("plugin %{public}s already exists! create plugin failed", pluginInfo.name.c_str());
        return;
    }
    // the dynamic plugin will register it's constructor to factory automatically after opening the binary
    // if we get null in factory, it means something must go wrong.
    DynamicModule handle = DynamicModuleDefault;
    if (!pluginInfo.isStatic) {
        handle = LoadDynamicPlugin(pluginInfo.name);
    }

    std::shared_ptr<PluginRegistInfo> registInfo = PluginFactory::GetGlobalPluginInfo(pluginInfo.name);
    if (registInfo == nullptr) {
        HiviewEventReport::ReportPluginLoad(pluginInfo.name, PluginEventSpace::LOAD_UNREGISTERED);
        if (handle != DynamicModuleDefault) {
            UnloadModule(handle);
        }
        return;
    }

    std::shared_ptr<Plugin> plugin = nullptr;
    if (registInfo->needCreateProxy) {
        plugin = std::make_shared<PluginProxy>();
        plugin->SetType(Plugin::PluginType::PROXY);
        (std::static_pointer_cast<PluginProxy>(plugin))->SetPluginConfig(pluginInfo);
    } else {
        plugin = registInfo->getPluginObject();
    }

    // app event publisher
    if (AppEventPublisherFactory::IsPublisher(pluginInfo.name)) {
        auto appEventHandler = std::make_shared<AppEventHandler>();
        (std::static_pointer_cast<AppEventPublisher>(plugin))->AddAppEventHandler(appEventHandler);
    }

    // Initialize plugin parameters
    plugin->SetName(pluginInfo.name);
    plugin->SetHandle(handle);
    plugin->SetHiviewContext(this);

    // call preload, check whether we should release at once
    if (!plugin->ReadyToLoad()) {
        // if the plugin is a dynamic loaded library, the handle will be closed when calling the destructor
        return;
    }
    // hold the global reference of the plugin
    pluginMap_[pluginInfo.name] = std::move(plugin);
}

void HiviewPlatform::CreatePipeline(const PluginConfig::PipelineInfo& pipelineInfo)
{
    if (pipelines_.find(pipelineInfo.name) != pipelines_.end()) {
        HIVIEW_LOGW("pipeline %{public}s already exists! create pipeline failed", pipelineInfo.name.c_str());
        return;
    }

    std::list<std::weak_ptr<Plugin>> pluginList;
    for (const auto& pluginName : pipelineInfo.pluginNameList) {
        if (pluginMap_.find(pluginName) == pluginMap_.end()) {
            HIVIEW_LOGI("could not find plugin(%{public}s), skip adding to pipeline(%{public}s).",
                pluginName.c_str(), pipelineInfo.name.c_str());
            continue;
        }
        pluginList.push_back(pluginMap_[pluginName]);
    }

    std::shared_ptr<Pipeline> pipeline = std::make_shared<Pipeline>(pipelineInfo.name, pluginList);
    pipelines_[pipelineInfo.name] = std::move(pipeline);

    std::string configPath = PIPELINE_RULE_CONFIG_DIR + pipelineInfo.name;
    HIVIEW_LOGI("config file=%{public}s", configPath.c_str());
    if (!FileUtil::FileExists(configPath)) {
        HIVIEW_LOGI("file=%{public}s does not exist", configPath.c_str());
        return;
    }
    DispatchRuleParser ruleParser(configPath);
    if (auto rule = ruleParser.GetRule(); rule != nullptr) {
        pipelineRules_[pipelineInfo.name] = rule;
    } else {
        HIVIEW_LOGE("failed to parse config file=%{public}s", configPath.c_str());
    }
}

void HiviewPlatform::InitPlugin(const PluginConfig& config __UNUSED, const PluginConfig::PluginInfo& pluginInfo)
{
    if (pluginMap_.find(pluginInfo.name) == pluginMap_.end()) {
        return;
    }
    auto& plugin = pluginMap_[pluginInfo.name];

    if (pluginInfo.workHandlerType == "thread") {
        auto workLoop = GetAvailableWorkLoop(pluginInfo.workHandlerName);
        plugin->BindWorkLoop(workLoop);
    }

    uint64_t beginTime = TimeUtil::GenerateTimestamp();
    plugin->OnLoad();

    if (std::string configPath = DISPATCH_RULE_CONFIG_DIR + pluginInfo.name; FileUtil::FileExists(configPath)) {
        HIVIEW_LOGI("config file=%{public}s", configPath.c_str());
        DispatchRuleParser ruleParser(configPath);
        if (auto rule = ruleParser.GetRule(); rule != nullptr) {
            AddDispatchInfo(std::weak_ptr<Plugin>(plugin), rule->typeList,
                rule->eventList, rule->tagList, rule->domainRuleMap);
        } else {
            HIVIEW_LOGE("failed to parse config file=%{public}s", configPath.c_str());
        }
    }

    if (pluginInfo.isEventSource) {
        auto sharedSource = std::static_pointer_cast<EventSource>(plugin);
        if (sharedSource == nullptr) {
            HIVIEW_LOGE("Fail to cast plugin to event source!");
            return;
        }
        for (const auto& pipelineName : pluginInfo.pipelineNameList) {
            sharedSource->AddPipeline(pipelines_[pipelineName]);
        }
    }

    if (plugin->GetType() == Plugin::PluginType::PROXY) {
        std::shared_ptr<PluginRegistInfo> registInfo = PluginFactory::GetGlobalPluginInfo(pluginInfo.name);
        if (registInfo != nullptr && registInfo->needStartupLoading) {
            std::shared_ptr<PluginProxy> pluginProxy = std::static_pointer_cast<PluginProxy>(plugin);
            pluginProxy->LoadPluginIfNeed();
        }
    }

    uint64_t endTime = TimeUtil::GenerateTimestamp();
    uint64_t loadDuration = endTime > beginTime ? (endTime - beginTime) : 0;
    HIVIEW_LOGI("Plugin %{public}s loadtime:%{public}" PRIu64 ".", pluginInfo.name.c_str(), loadDuration);
    HiviewEventReport::ReportPluginLoad(pluginInfo.name, PluginEventSpace::LOAD_SUCCESS, loadDuration);
}

void HiviewPlatform::NotifyPluginReady()
{
    auto event = std::make_shared<Event>("platform");
    event->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    event->eventId_ = Event::EventId::PLUGIN_LOADED;
    PostUnorderedEvent(nullptr, event);
}

void HiviewPlatform::StartEventSource(std::shared_ptr<EventSource> source)
{
    auto workLoop = source->GetWorkLoop();
    auto name = source->GetName();
    if (workLoop == nullptr) {
        HIVIEW_LOGW("No work loop available, start event source[%{public}s] in current thead!", name.c_str());
        source->StartEventSource();
    } else {
        HIVIEW_LOGI("Start event source[%{public}s] in thead[%{public}s].", name.c_str(), workLoop->GetName().c_str());
        auto task = std::bind(&EventSource::StartEventSource, source.get());
        workLoop->AddEvent(nullptr, nullptr, task);
    }
}

// only call from main thread
std::shared_ptr<EventLoop> HiviewPlatform::GetAvailableWorkLoop(const std::string& name)
{
    auto it = privateWorkLoopMap_.find(name);
    if (it != privateWorkLoopMap_.end()) {
        return it->second;
    }

    auto privateLoop = std::make_shared<EventLoop>(name);
    if (privateLoop != nullptr) {
        privateWorkLoopMap_.insert(std::make_pair(name, privateLoop));
        privateLoop->StartLoop();
    }
    return privateLoop;
}

void HiviewPlatform::CleanupUnusedResources()
{
    auto iter = pluginMap_.begin();
    while (iter != pluginMap_.end()) {
        if (iter->second == nullptr) {
            iter = pluginMap_.erase(iter);
        } else {
            ++iter;
        }
    }
}

void HiviewPlatform::ScheduleCreateAndInitPlugin(const PluginConfig::PluginInfo& pluginInfo)
{
    // only support thread type
    CreatePlugin(pluginInfo);
    if (pluginMap_.find(pluginInfo.name) == pluginMap_.end()) {
        return;
    }
    auto& plugin = pluginMap_[pluginInfo.name];

    if (pluginInfo.workHandlerType == "thread") {
        auto workLoop = GetAvailableWorkLoop(pluginInfo.workHandlerName);
        plugin->BindWorkLoop(workLoop);
    }
    plugin->OnLoad();
}

void HiviewPlatform::StartLoop()
{
    if (mainWorkLoop_ != nullptr) {
        mainWorkLoop_->StartLoop(false);
    }
}

void HiviewPlatform::StartPlatformDispatchQueue()
{
    if (unorderQueue_ == nullptr) {
        unorderQueue_ = std::make_shared<EventDispatchQueue>("plat_unorder", Event::ManageType::UNORDERED, this);
        unorderQueue_->Start();
    }

    if (sharedWorkLoop_ == nullptr) {
        sharedWorkLoop_ = std::make_shared<EventLoop>("plat_shared");
        sharedWorkLoop_->StartLoop();
    }

    if (mainWorkLoop_ == nullptr) {
        mainWorkLoop_ = std::make_shared<EventLoop>("hiview");
    }
}

std::list<std::weak_ptr<Plugin>> HiviewPlatform::GetPipelineSequenceByName(const std::string& name)
{
    if (!isReady_) {
        return std::list<std::weak_ptr<Plugin>>();
    }

    auto it = pipelines_.find(name);
    if (it != pipelines_.end()) {
        return it->second->GetProcessSequence();
    }
    return std::list<std::weak_ptr<Plugin>>(0);
}

void HiviewPlatform::PostUnorderedEvent(std::shared_ptr<Plugin> plugin, std::shared_ptr<Event> event)
{
    if (!isReady_) {
        return;
    }

    if (plugin == nullptr) {
        HIVIEW_LOGI("maybe platform send event");
    }

    if (unorderQueue_ != nullptr && event != nullptr) {
        event->processType_ = Event::ManageType::UNORDERED;
        unorderQueue_->Enqueue(event);
    }
}

void HiviewPlatform::RegisterUnorderedEventListener(std::weak_ptr<EventListener> listener)
{
    auto ptr = listener.lock();
    if (ptr == nullptr) {
        return;
    }
    auto name = ptr->GetListenerName();
    auto itListenerInfo = listeners_.find(name);
    if (itListenerInfo == listeners_.end()) {
        auto tmp = std::make_shared<ListenerInfo>();
        tmp->listener_ = listener;
        listeners_[name] = tmp;
    } else {
        auto tmp = listeners_[name];
        tmp->listener_ = listener;
    }
}

bool HiviewPlatform::PostSyncEventToTarget(std::shared_ptr<Plugin> caller, const std::string& calleeName,
                                           std::shared_ptr<Event> event)
{
    if (!isReady_) {
        return false;
    }

    auto it = pluginMap_.find(calleeName);
    if (it == pluginMap_.end()) {
        return false;
    }

    auto callee = it->second;
    if (callee == nullptr) {
        return false;
    }

    auto workLoop = callee->GetWorkLoop();
    std::future<bool> ret;
    if (workLoop == nullptr) {
        ret = sharedWorkLoop_->AddEventForResult(callee, event);
    } else {
        ret = workLoop->AddEventForResult(callee, event);
    }
    return ret.get();
}

void HiviewPlatform::PostAsyncEventToTarget(std::shared_ptr<Plugin> caller, const std::string& calleeName,
                                            std::shared_ptr<Event> event)
{
    if (!isReady_) {
        return;
    }

    auto it = pluginMap_.find(calleeName);
    if (it == pluginMap_.end()) {
        return;
    }

    auto callee = it->second;
    if (callee == nullptr) {
        return;
    }

    auto workLoop = callee->GetWorkLoop();
    if (workLoop == nullptr) {
        sharedWorkLoop_->AddEvent(callee, event);
    } else {
        workLoop->AddEvent(callee, event);
    }
    HIVIEW_LOGI("Post async event to %{public}s successfully", calleeName.c_str());
}

std::shared_ptr<EventLoop> HiviewPlatform::GetSharedWorkLoop()
{
    return sharedWorkLoop_;
}

std::shared_ptr<EventLoop> HiviewPlatform::GetMainWorkLoop()
{
    return mainWorkLoop_;
}

bool HiviewPlatform::IsReady()
{
    return isReady_;
}

void HiviewPlatform::RequestUnloadPlugin(std::shared_ptr<Plugin> caller)
{
    if (caller == nullptr) {
        HiviewEventReport::ReportPluginUnload("", PluginEventSpace::UNLOAD_INVALID);
        return;
    }

    std::string name = caller->GetName();
    auto task = std::bind(&HiviewPlatform::UnloadPlugin, this, name);
    // delay 1s to unload target plugin
    const int unloadDelay = 1;
    sharedWorkLoop_->AddTimerEvent(nullptr, nullptr, task, unloadDelay, false);
}

void HiviewPlatform::UnloadPlugin(const std::string& name)
{
    auto it = pluginMap_.find(name);
    if (it == pluginMap_.end()) {
        HiviewEventReport::ReportPluginUnload(name, PluginEventSpace::UNLOAD_NOT_FOUND);
        return;
    }
    auto target = it->second;
    if (target == nullptr) {
        HIVIEW_LOGW("Plugin %{public}s target is null.", name.c_str());
        return;
    }
    auto count = target.use_count();
    if (count > 2) { // two counts for 1.current ref 2.map holder ref
        HIVIEW_LOGW("Plugin %{public}s has more refs(%{public}ld), may caused by unfinished task. unload failed.",
            name.c_str(), count);
        HiviewEventReport::ReportPluginUnload(name, PluginEventSpace::UNLOAD_IN_USE);
        return;
    }
    pluginMap_.erase(name);
    target->OnUnload();

    // By default, reloading is not supported after unloading!
    PluginFactory::UnregisterPlugin(target->GetName());
    // report unloading success event
    HiviewEventReport::ReportPluginUnload(name, PluginEventSpace::UNLOAD_SUCCESS);

    auto looper = target->GetWorkLoop();
    if (looper == nullptr) {
        return;
    }
    if (looper.use_count() <= 3) { // three counts for 1.current ref 2.plugin ref 3.map holder ref
        auto looperName = looper->GetRawName();
        HIVIEW_LOGI("%{public}s has refs(%{public}ld).", looperName.c_str(), looper.use_count());
        looper->StopLoop();
        privateWorkLoopMap_.erase(looperName);
        HIVIEW_LOGI("Stop %{public}s done.", looperName.c_str());
    }
}

std::string HiviewPlatform::GetHiViewDirectory(HiviewContext::DirectoryType type)
{
    switch (type) {
        case HiviewContext::DirectoryType::CONFIG_DIRECTORY:
            return defaultConfigDir_;
        case HiviewContext::DirectoryType::WORK_DIRECTORY:
            return defaultWorkDir_;
        case HiviewContext::DirectoryType::PERSIST_DIR:
            return defaultPersistDir_;
        default:
            break;
    }
    return "";
}

void HiviewPlatform::ValidateAndCreateDirectory(std::string& defaultPath, const std::string& realPath)
{
    if (defaultPath != realPath) {
        defaultPath = realPath;
    }
    if (FileUtil::IsDirectory(defaultPath)) {
        return;
    }
    FileUtil::CreateDirWithDefaultPerm(defaultPath, AID_SYSTEM, AID_SYSTEM);
}

void HiviewPlatform::ValidateAndCreateDirectories(const std::string& localPath, const std::string& workPath,
    const std::string& persistPath)
{
    ValidateAndCreateDirectory(defaultConfigDir_, localPath);
    ValidateAndCreateDirectory(defaultWorkDir_, workPath);
    ValidateAndCreateDirectory(defaultPersistDir_, persistPath);
}

#ifndef _WIN32
void HiviewPlatform::ExitHiviewIfNeed()
{
    int selfPid = getpid();
    std::string selfProcName = CommonUtils::GetProcNameByPid(selfPid);
    if (selfProcName != "hiview") {
        return;
    }

    std::string pidFile = defaultWorkDir_ + "/" + std::string(HIVIEW_PID_FILE_NAME);
    if (!FileUtil::FileExists(pidFile)) {
        return;
    }

    std::string content;
    FileUtil::LoadStringFromFile(pidFile, content);
    int32_t pid = -1;
    if (!StringUtil::StrToInt(content, pid)) {
        return;
    }

    std::string procName = CommonUtils::GetProcNameByPid(pid);
    if (procName == "hiview") {
        printf("Hiview is already started, exit! \n");
        exit(1);
    }
    FileUtil::SaveStringToFile(pidFile, std::to_string(selfPid));
}
#else
void HiviewPlatform::ExitHiviewIfNeed()
{
}
#endif

std::string HiviewPlatform::GetPluginConfigPath()
{
    return defaultConfigDir_ + defaultConfigName_;
}

void HiviewPlatform::AppendPluginToPipeline(const std::string& pluginName, const std::string& pipelineName)
{
    auto it = pipelines_.find(pipelineName);
    if (it == pipelines_.end()) {
        HIVIEW_LOGW("Fail to find pipeline with name :%{public}s", pipelineName.c_str());
        return;
    }
    auto ptr = GetPluginByName(pluginName);
    if (ptr == nullptr) {
        HIVIEW_LOGW("Fail to find plugin with name :%{public}s", pluginName.c_str());
        return;
    }
    it->second->AppendProcessor(ptr);
    HIVIEW_LOGI("plugin %{public}s add to pipeline %{public}s succeed.", pluginName.c_str(), pipelineName.c_str());
}

void HiviewPlatform::RequestLoadBundle(const std::string& bundleName)
{
    if (pluginBundleInfos_.find(bundleName) != pluginBundleInfos_.end()) {
        HIVIEW_LOGW("Bundle already loaded.");
        return;
    }

    std::string configPath = defaultConfigDir_ + bundleName + "_plugin_config";
    LoadPluginBundle(bundleName, configPath);
}

void HiviewPlatform::RequestUnloadBundle(const std::string& bundleName, uint64_t delay)
{
    auto task = [this, bundleName]() {
        HIVIEW_LOGI("start to unload the bundle %{public}s.", bundleName.c_str());
        if (pluginBundleInfos_.find(bundleName) != pluginBundleInfos_.end()) {
            pluginBundleInfos_.erase(bundleName);
        }
    };
    sharedWorkLoop_->AddTimerEvent(nullptr, nullptr, task, delay, false);
}

std::shared_ptr<Plugin> HiviewPlatform::InstancePluginByProxy(std::shared_ptr<Plugin> proxy)
{
    if (proxy == nullptr) {
        return nullptr;
    }

    auto proxyPtr = std::static_pointer_cast<PluginProxy>(proxy);
    std::shared_ptr<PluginRegistInfo> registInfo = PluginFactory::GetGlobalPluginInfo(proxyPtr->GetName());
    if (registInfo == nullptr) {
        HIVIEW_LOGE("Failed to find registInfo:%{public}s", proxyPtr->GetName().c_str());
        return nullptr;
    }

    auto plugin = registInfo->getPluginObject();
    plugin->SetName(proxyPtr->GetName());
    plugin->SetHiviewContext(this);
    plugin->BindWorkLoop(proxyPtr->GetWorkLoop());
    plugin->OnLoad();
    auto config = proxyPtr->GetPluginConfig();
    if (config.isEventSource) {
        auto sharedSource = std::static_pointer_cast<EventSource>(plugin);
        for (auto& pipelineName : config.pipelineNameList) {
            sharedSource->AddPipeline(pipelines_[pipelineName]);
        }
        eventSourceList_.push_back(plugin);
    }
    return plugin;
}

std::shared_ptr<Plugin> HiviewPlatform::GetPluginByName(const std::string& name)
{
    auto it = pluginMap_.find(name);
    if (it == pluginMap_.end()) {
        return nullptr;
    }
    return it->second;
}

std::string HiviewPlatform::GetHiviewProperty(const std::string& key, const std::string& defaultValue)
{
    auto propPair = hiviewProperty_.find(key);
    if (propPair != hiviewProperty_.end()) {
        return propPair->second;
    }
    return Parameter::GetString(key, defaultValue);
}

bool HiviewPlatform::SetHiviewProperty(const std::string& key, const std::string& value, bool forceUpdate)
{
    auto propPair = hiviewProperty_.find(key);
    if (forceUpdate || (propPair == hiviewProperty_.end())) {
        hiviewProperty_[key] = value;
        return true;
    }
    return Parameter::SetProperty(key, value);
}

void HiviewPlatform::CheckUnloadablePlugins()
{
    for (auto const &pluginKv : pluginMap_) {
        if (pluginKv.second->GetType() != Plugin::PluginType::PROXY) {
            continue;
        }
        auto ptr = std::static_pointer_cast<PluginProxy>(pluginKv.second);
        if (ptr == nullptr) {
            continue;
        }
        std::shared_ptr<EventLoop> eventloop = ptr->GetWorkLoop();
        if (eventloop != nullptr) {
            auto task = std::bind(&PluginProxy::DestroyInstanceIfNeed, ptr.get(), maxIdleTime_);
            if (eventloop->AddEvent(nullptr, nullptr, task) != 0) {
                continue;
            }
            HIVIEW_LOGW("AddEvent failed");
        }
        ptr->DestroyInstanceIfNeed(maxIdleTime_);
    }
}

void HiviewPlatform::ScheduleCheckUnloadablePlugins()
{
    auto task = std::bind(&HiviewPlatform::CheckUnloadablePlugins, this);
    sharedWorkLoop_->AddTimerEvent(nullptr, nullptr, task, checkIdlePeriod_, true);
}

void HiviewPlatform::AddDispatchInfo(std::weak_ptr<Plugin> plugin, const std::unordered_set<uint8_t>& types,
    const std::unordered_set<std::string>& eventNames, const std::unordered_set<std::string>& tags,
    const std::unordered_map<std::string, DomainRule>& domainRulesMap)
{
    auto ptr = plugin.lock();
    if (ptr == nullptr) {
        return;
    }
    auto name = ptr->GetName();
    auto itDispatchInfo = dispatchers_.find(name);
    std::shared_ptr<DispatchInfo> data = nullptr;
    if (itDispatchInfo == dispatchers_.end()) {
        auto tmp = std::make_shared<DispatchInfo>();
        tmp->plugin_ = plugin;
        dispatchers_[name] = tmp;
        data = dispatchers_[name];
    } else {
        data = itDispatchInfo->second;
    }
    if (!types.empty()) {
        data->typesInfo_.insert(types.begin(), types.end());
    }
    if (!tags.empty()) {
        data->tagsInfo_.insert(tags.begin(), tags.end());
    }
    if (!eventNames.empty()) {
        data->eventsInfo_.insert(eventNames.begin(), eventNames.end());
    }
    if (!domainRulesMap.empty()) {
        data->domainsInfo_.insert(domainRulesMap.begin(), domainRulesMap.end());
    }
}

void HiviewPlatform::AddListenerInfo(uint32_t type, const std::string& name, const std::set<std::string>& eventNames,
    const std::map<std::string, DomainRule>& domainRulesMap)
{
    auto itListenerInfo = listeners_.find(name);
    std::shared_ptr<ListenerInfo> data = nullptr;
    if (itListenerInfo == listeners_.end()) {
        auto tmp = std::make_shared<ListenerInfo>();
        listeners_[name] = tmp;
        data = listeners_[name];
    } else {
        data = itListenerInfo->second;
    }
    if (!eventNames.empty()) {
        auto it = data->eventsInfo_.find(type);
        if (it != data->eventsInfo_.end()) {
            it->second.insert(eventNames.begin(), eventNames.end());
        } else {
            data->eventsInfo_[type] = eventNames;
        }
    }
    if (!domainRulesMap.empty()) {
        auto it = data->domainsInfo_.find(type);
        if (it != data->domainsInfo_.end()) {
            it->second.insert(domainRulesMap.begin(), domainRulesMap.end());
        } else {
            data->domainsInfo_[type] = domainRulesMap;
        }
    }
}

void HiviewPlatform::AddListenerInfo(uint32_t type, const std::string& name)
{
    auto itListenerInfo = listeners_.find(name);
    std::shared_ptr<ListenerInfo> data = nullptr;
    if (itListenerInfo == listeners_.end()) {
        auto tmp = std::make_shared<ListenerInfo>();
        listeners_[name] = tmp;
        data = listeners_[name];
    } else {
        data = itListenerInfo->second;
    }
    data->messageTypes_.push_back(type);
}

std::vector<std::weak_ptr<EventListener>> HiviewPlatform::GetListenerInfo(uint32_t type,
    const std::string& eventName, const std::string& domain)
{
    std::vector<std::weak_ptr<EventListener>> ret;
    for (auto& pairListener : listeners_) {
        auto listenerInfo = pairListener.second;
        if (listenerInfo->Match(type, eventName, domain)) {
            ret.push_back(listenerInfo->listener_);
        }
    }
    return ret;
}

std::vector<std::weak_ptr<Plugin>> HiviewPlatform::GetDisPatcherInfo(uint32_t type,
    const std::string& eventName, const std::string& tag, const std::string& domain)
{
    std::vector<std::weak_ptr<Plugin>> ret;
    for (auto& pairDispatcher : dispatchers_) {
        auto dispatcherInfo = pairDispatcher.second;
        if (dispatcherInfo->Match(type, eventName, tag, domain)) {
            ret.push_back(dispatcherInfo->plugin_);
        }
    }
    return ret;
}
} // namespace HiviewDFX
} // namespace OHOS
