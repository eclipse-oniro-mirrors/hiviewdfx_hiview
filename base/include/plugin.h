/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_BASE_PLUGIN_H
#define HIVIEW_BASE_PLUGIN_H

#include <atomic>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include "defines.h"
#include "dynamic_module.h"
#include "event.h"
#include "event_loop.h"

class HiEvent;
namespace OHOS {
namespace HiviewDFX {
class HiviewContext;
class DllExport Plugin : public EventHandler, public std::enable_shared_from_this<Plugin> {
public:
    enum class PluginType {
        STATIC,
        DYNAMIC,
        PROXY,
    };
public:
    Plugin() : handle_(DynamicModuleDefault), context_(nullptr), useCount_(0) {};
    virtual ~Plugin();
    // do not store the event in the callback
    // create a new one through copy constructor is preferred
    virtual bool OnEvent(std::shared_ptr<Event>& event) override;

    /* If the plugin is at the top of a pipeline, this function will be used to determine
     * whether the current event can be processed by the pipeline.
     */
    virtual bool CanProcessEvent(std::shared_ptr<Event> event) override;

    /* If the plugin is in a pipeline, this function will be used to determine
     * whether the current pipeline event can be processed by the plugin.
     */
    virtual bool IsInterestedPipelineEvent(std::shared_ptr<Event> event) override;
    virtual bool CanProcessMoreEvents() override;
    bool OnEventProxy(std::shared_ptr<Event> event) override;
    virtual std::string GetHandlerInfo() override;

    // check whether the plugin should be loaded in current environment
    // the plugin will not be load if return false
    // called by plugin framework in main thread
    virtual bool ReadyToLoad()
    {
        return true;
    };

    // the plugin instance will be stored in platform after calling this function
    // and other loaded plugin can pass event to this plugin
    // called by plugin framework in main thread
    virtual void OnLoad(){};

    // called by parameter upgrade module
    virtual void OnConfigUpdate(const std::string &localCfgPath, const std::string &cloudCfgPath) {};

    // release resource and clear pending workloads
    // after calling this function ,the reference of this plugin will be deleted
    // called by plugin framework in main thread
    virtual void OnUnload(){};

    // dump plugin info from dump command line
    virtual void Dump(int fd __UNUSED, const std::vector<std::string>& cmds __UNUSED) {};

    // create an event with specific type
    virtual std::shared_ptr<Event> GetEvent(Event::MessageType type)
    {
        auto event = std::make_shared<Event>(name_);
        event->messageType_ = type;
        return event;
    };

    // called by audit module
    virtual std::string GetPluginInfo();

    // Listener callback
    virtual void OnEventListeningCallback(const Event &msg)
    {
        return;
    }

    void AddDispatchInfo(const std::unordered_set<uint8_t>& types,
        const std::unordered_set<std::string> &eventNames = {}, const std::unordered_set<std::string> &tags = {},
            const std::unordered_map<std::string, DomainRule>& domainRulesMap = {});

    // reinsert the event into the workloop
    // delay in seconds
    void DelayProcessEvent(std::shared_ptr<Event> event, uint64_t delay);

    const std::string& GetName();
    const std::string& GetVersion();
    void SetName(const std::string& name);
    void SetVersion(const std::string& version);
    void BindWorkLoop(std::shared_ptr<EventLoop> loop);
    void UpdateActiveTime();
    void UpdateTimeByDelay(time_t delay);
    std::shared_ptr<EventLoop> GetWorkLoop();

    HiviewContext* GetHiviewContext()
    {
        return context_;
    };

    void SetHiviewContext(HiviewContext* context)
    {
        std::call_once(contextSetFlag_, [&]() { context_ = context; });
    };

    void SetHandle(DynamicModule handle)
    {
        handle_ = handle;
        if (handle_ != nullptr) {
            type_ = PluginType::DYNAMIC;
        }
    };

    void SetBundleName(const std::string& bundle)
    {
        bundle_ = bundle;
    }

    bool IsBundlePlugin()
    {
        return !bundle_.empty();
    }

    bool HasLoaded()
    {
        return loaded_;
    }

    void SetType(PluginType type)
    {
        type_ = type;
    }

    PluginType GetType() const
    {
        return type_;
    }

    time_t GetLastActiveTime() const
    {
        return lastActiveTime_;
    }

    int64_t GetUseCount() const
    {
        return useCount_;
    }

    void AddUseCount()
    {
        ++useCount_;
    }

    void SubUseCount()
    {
        --useCount_;
    }
protected:
    std::string name_;
    std::string bundle_;
    std::string version_;
    std::shared_ptr<EventLoop> workLoop_;
    PluginType type_ = PluginType::STATIC;
    std::atomic<time_t> lastActiveTime_;

private:
    DynamicModule handle_;
    // the reference of the plugin platform
    // always available in framework callbacks
    HiviewContext* context_;
    std::once_flag contextSetFlag_;
    std::atomic<bool> loaded_;
    std::atomic<int64_t> useCount_;
};
class HiviewContext {
public:
    virtual ~HiviewContext(){};
    // post event to broadcast queue, the event will be delivered to all plugin that concern this event
    virtual void PostUnorderedEvent(std::shared_ptr<Plugin> plugin __UNUSED, std::shared_ptr<Event> event __UNUSED) {};

    // register listener to unordered broadcast queue
    virtual void RegisterUnorderedEventListener(std::weak_ptr<EventListener> listener __UNUSED) {};

    // send a event to a specific plugin and wait the return of the OnEvent.
    virtual bool PostSyncEventToTarget(std::shared_ptr<Plugin> caller __UNUSED, const std::string& callee __UNUSED,
                                       std::shared_ptr<Event> event __UNUSED)
    {
        return true;
    }

    // send a event to a specific plugin and return at once
    virtual void PostAsyncEventToTarget(std::shared_ptr<Plugin> caller __UNUSED, const std::string& callee __UNUSED,
                                        std::shared_ptr<Event> event __UNUSED) {};

    // request plugin platform to release plugin instance
    // do not recommend unload an plugin in pipeline scheme
    // platform will check the reference count if other module still holding the reference of this module
    virtual void RequestUnloadPlugin(std::shared_ptr<Plugin> caller __UNUSED) {};

    // get the shared event loop reference
    virtual std::shared_ptr<EventLoop> GetSharedWorkLoop()
    {
        return nullptr;
    }

    // get predefined pipeline list
    virtual std::list<std::weak_ptr<Plugin>> GetPipelineSequenceByName(const std::string& name __UNUSED)
    {
        return std::list<std::weak_ptr<Plugin>>(0);
    }

    // check if all non-pending loaded plugins are loaded
    virtual bool IsReady()
    {
        return false;
    }

    enum class DirectoryType {
        CONFIG_DIRECTORY,
        WORK_DIRECTORY,
        PERSIST_DIR,
    };
    // get hiview available directory
    virtual std::string GetHiViewDirectory(DirectoryType type __UNUSED)
    {
        return "";
    }
    virtual std::string GetHiviewProperty(const std::string& key __UNUSED, const std::string& defaultValue)
    {
        return defaultValue;
    }

    virtual bool SetHiviewProperty(const std::string& key __UNUSED, const std::string& value __UNUSED,
        bool forceUpdate __UNUSED)
    {
        return true;
    }

    virtual void AppendPluginToPipeline(const std::string& pluginName __UNUSED,
                                        const std::string& pipelineName __UNUSED) {};
    virtual void RequestLoadBundle(const std::string& bundleName __UNUSED) {};

    // request plugin platform to release plugin bundle
    virtual void RequestUnloadBundle(const std::string& bundleName, uint64_t delay = 0) {};

    virtual std::shared_ptr<Plugin> InstancePluginByProxy(std::shared_ptr<Plugin> proxy __UNUSED)
    {
        return nullptr;
    }

    virtual std::shared_ptr<Plugin> GetPluginByName(const std::string& name)
    {
        return nullptr;
    }

    virtual void AddListenerInfo(uint32_t type, const std::string& name, const std::set<std::string>& eventNames,
        const std::map<std::string, DomainRule>& domainRulesMap) {};

    virtual void AddListenerInfo(uint32_t type, const std::string& name) {};

    virtual std::vector<std::weak_ptr<EventListener>> GetListenerInfo(uint32_t type,
        const std::string& eventName, const std::string& domain)
    {
        return {};
    }

    virtual void AddDispatchInfo(std::weak_ptr<Plugin> plugin, const std::unordered_set<uint8_t>& types,
        const std::unordered_set<std::string>& eventNames, const std::unordered_set<std::string>& tags,
            const std::unordered_map<std::string, DomainRule>& domainRulesMap) {};

    virtual std::vector<std::weak_ptr<Plugin>> GetDisPatcherInfo(uint32_t type,
        const std::string& eventName, const std::string& tag, const std::string& domain)
    {
        return {};
    }
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_PLUGIN_H
