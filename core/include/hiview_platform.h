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
#ifndef HIVIEW_BASE_PLUGIN_PLATFORM_H
#define HIVIEW_BASE_PLUGIN_PLATFORM_H
#include <memory>
#include <mutex>
#include <string>

#include "defines.h"
#include "dynamic_module.h"
#include "event_dispatch_queue.h"
#include "event_loop.h"
#include "event_source.h"
#include "pipeline.h"
#include "plugin.h"
#include "plugin_bundle.h"
#include "plugin_config.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
using PipelineConfigMap = std::map<std::string, std::shared_ptr<DispatchRule>>;
class HiviewPlatform : public HiviewContext, public Singleton<HiviewPlatform> {
public:
    HiviewPlatform();
    ~HiviewPlatform();
    bool InitEnvironment(const std::string& platformConfigDir = "");
    void StartLoop();
    void SetMaxProxyIdleTime(time_t idleTime);
    void SetCheckProxyIdlePeriod(time_t period);

    void PostUnorderedEvent(std::shared_ptr<Plugin> plugin, std::shared_ptr<Event> event) override;
    void RegisterUnorderedEventListener(std::weak_ptr<EventListener> listener) override;
    bool PostSyncEventToTarget(std::shared_ptr<Plugin> caller, const std::string& calleeName,
        std::shared_ptr<Event> event) override;
    void PostAsyncEventToTarget(std::shared_ptr<Plugin> caller, const std::string& calleeName,
        std::shared_ptr<Event> event) override;
    void RequestUnloadPlugin(std::shared_ptr<Plugin> caller) override;
    std::list<std::weak_ptr<Plugin>> GetPipelineSequenceByName(const std::string& name) override;
    std::shared_ptr<EventLoop> GetSharedWorkLoop() override;
    std::shared_ptr<EventLoop> GetMainWorkLoop() override;
    std::string GetHiViewDirectory(DirectoryType type) override;
    std::string GetHiviewProperty(const std::string& key, const std::string& defaultValue) override;
    bool SetHiviewProperty(const std::string& key, const std::string& value, bool forceUpdate) override;
    bool IsReady() override;
    void AppendPluginToPipeline(const std::string& pluginName, const std::string& pipelineName) override;
    void RequestLoadBundle(const std::string& bundleName __UNUSED) override;
    void RequestUnloadBundle(const std::string& bundleName, uint64_t delay = 0) override;
    std::shared_ptr<Plugin> InstancePluginByProxy(std::shared_ptr<Plugin> proxy) override;
    std::shared_ptr<Plugin> GetPluginByName(const std::string& name) override;
    void AddDispatchInfo(std::weak_ptr<Plugin> plugin, const std::unordered_set<uint8_t>& types,
        const std::unordered_set<std::string>& eventNames, const std::unordered_set<std::string>& tags,
        const std::unordered_map<std::string, DomainRule>& domainRulesMap) override;
    std::vector<std::weak_ptr<Plugin>> GetDisPatcherInfo(uint32_t type,
        const std::string& eventName, const std::string& tag, const std::string& domain) override;
    void AddListenerInfo(uint32_t type, const std::string& name, const std::set<std::string>& eventNames,
        const std::map<std::string, DomainRule>& domainRulesMap) override;
    void AddListenerInfo(uint32_t type, const std::string& name) override;
    std::vector<std::weak_ptr<EventListener>> GetListenerInfo(uint32_t type,
        const std::string& eventName, const std::string& domain) override;

    PipelineConfigMap& GetPipelineConfigMap()
    {
        return pipelineRules_;
    }

    const std::map<std::string, std::shared_ptr<Plugin>>& GetPluginMap()
    {
        return pluginMap_;
    }

    const std::map<std::string, std::shared_ptr<Pipeline>>& GetPipelineMap()
    {
        return pipelines_;
    }

    const std::map<std::string, std::shared_ptr<EventLoop>>& GetWorkLoopMap()
    {
        return privateWorkLoopMap_;
    }

    const std::map<std::string, std::shared_ptr<PluginBundle>>& GetPluginBundleInfoMap()
    {
        return pluginBundleInfos_;
    }

private:
    struct ListenerInfo {
        std::weak_ptr<EventListener> listener_;
        std::vector<uint32_t> messageTypes_;
        std::map<uint32_t, std::set<std::string>> eventsInfo_;
        std::map<uint32_t, std::map<std::string, DomainRule>> domainsInfo_;
        bool Match(uint32_t type, const std::string& eventName, const std::string& domain)
        {
            auto it = std::find(messageTypes_.begin(), messageTypes_.end(), type);
            if (it != messageTypes_.end()) {
                return true;
            }
            auto itEventList = eventsInfo_.find(type);
            if (itEventList != eventsInfo_.end()) {
                auto eventList = itEventList->second;
                if (eventList.find(eventName) != eventList.end()) {
                    return true;
                }
            }
            auto itDomainsInfo = domainsInfo_.find(type);
            if (itDomainsInfo != domainsInfo_.end()) {
                auto itDomainRule = itDomainsInfo->second.find(domain);
                if (itDomainRule != itDomainsInfo->second.end()) {
                    return itDomainRule->second.FindEvent(eventName);
                }
            }
            return false;
        }
    };

    struct DispatchInfo {
        std::weak_ptr<Plugin> plugin_;
        std::unordered_set<uint8_t> typesInfo_;
        std::unordered_set<std::string> eventsInfo_;
        std::unordered_set<std::string> tagsInfo_;
        std::unordered_map<std::string, DomainRule> domainsInfo_;
        bool Match(uint8_t type, const std::string& eventName, const std::string& tag,
            const std::string& domain)
        {
            if (typesInfo_.find(type) != typesInfo_.end()) {
                return true;
            }
            if (tagsInfo_.find(tag) != tagsInfo_.end()) {
                return true;
            }
            if (eventsInfo_.find(eventName) != eventsInfo_.end()) {
                return true;
            }
            auto itDomainRule = domainsInfo_.find(domain);
            if (itDomainRule != domainsInfo_.end()) {
                return itDomainRule->second.FindEvent(eventName);
            }
            return false;
        }
    };

    void CreateWorkingDirectories(const std::string& platformConfigDir);
    void StartPlatformDispatchQueue();
    void CreatePlugin(const PluginConfig::PluginInfo& pluginInfo);
    void CreatePipeline(const PluginConfig::PipelineInfo& pipelineInfo);
    void InitPlugin(const PluginConfig& config __UNUSED, const PluginConfig::PluginInfo& pluginInfo);
    void NotifyPluginReady();
    void ScheduleCreateAndInitPlugin(const PluginConfig::PluginInfo& pluginInfo);
    DynamicModule LoadDynamicPlugin(const std::string& name) const;
    std::string GetDynamicLibName(const std::string& name, bool hasOhosSuffix) const;
    std::shared_ptr<EventLoop> GetAvailableWorkLoop(const std::string& name);
    void CleanupUnusedResources();
    void UnloadPlugin(const std::string& name);
    void StartEventSource(std::shared_ptr<EventSource> source);
    void ValidateAndCreateDirectory(std::string& defaultPath, const std::string& realPath);
    void ValidateAndCreateDirectories(const std::string& localPath, const std::string& workPath,
        const std::string& persistPath);
    void LoadBusinessPlugin(const PluginConfig& config);
    void ExitHiviewIfNeed();
    std::string GetPluginConfigPath();
    std::string SplitBundleNameFromPath(const std::string& filePath);
    void UpdateBetaConfigIfNeed();
    void LoadPluginBundles();
    void LoadPluginBundle(const std::string& bundleName, const std::string& filePath);
    void ScheduleCheckUnloadablePlugins();
    void CheckUnloadablePlugins();
    std::string SearchPluginBundle(const std::string& name) const;

    bool isReady_;
    std::string defaultConfigDir_;
    std::string defaultWorkDir_;
    std::string defaultCommercialWorkDir_;
    std::string defaultPersistDir_;
    std::string defaultConfigName_;
    std::vector<std::string> dynamicLibSearchDir_;
    std::shared_ptr<EventDispatchQueue> unorderQueue_;
    std::shared_ptr<EventLoop> sharedWorkLoop_;
    std::shared_ptr<EventLoop> mainWorkLoop_;
    std::map<std::string, std::shared_ptr<Plugin>> pluginMap_;
    std::map<std::string, std::shared_ptr<Pipeline>> pipelines_;
    std::map<std::string, std::shared_ptr<EventLoop>> privateWorkLoopMap_;
    std::map<std::string, std::string> hiviewProperty_;
    std::map<std::string, std::shared_ptr<PluginBundle>> pluginBundleInfos_;

    // Listener data structure:<pluginName, <domain_eventName, Plugin>>
    std::unordered_map<std::string, std::shared_ptr<ListenerInfo>> listeners_;
    std::unordered_map<std::string, std::shared_ptr<DispatchInfo>> dispatchers_;
    PipelineConfigMap pipelineRules_;
    std::vector<std::shared_ptr<Plugin>> eventSourceList_;

    // the max waited time before destroy plugin instance
    const time_t DEFAULT_IDLE_TIME = 300; // 300 seconds
    time_t maxIdleTime_ = DEFAULT_IDLE_TIME;
    time_t checkIdlePeriod_ = DEFAULT_IDLE_TIME / 2; // 2 : half idle time
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_PLUGIN_PLATFORM_H
