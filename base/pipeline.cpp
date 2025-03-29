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
#include "pipeline.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "thread_util.h"
#include "time_util.h"
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-Pipeline");
void PipelineEvent::OnRepack()
{
    startDeliver_ = false;
    ResetTimestamp();
}

void PipelineEvent::OnPending()
{
    hasPending_ = true;
}

bool PipelineEvent::OnContinue()
{
    if ((!hasFinish_) && processors_.empty()) {
        return OnFinish();
    }

    // the framework will call OnContinue when the event is assigned to a pipeline
    if (!startDeliver_) {
        startDeliver_ = true;
    }

    std::weak_ptr<Plugin> plugin = processors_.front();
    processors_.pop_front();
    if (auto pluginPtr = plugin.lock()) {
        if (!pluginPtr->CanProcessMoreEvents()) {
            if (handler_ != nullptr) {
                handler_->PauseDispatch(plugin);
            }
        }

        if (!pluginPtr->IsInterestedPipelineEvent(shared_from_this())) {
            if ((!HasFinish() && !HasPending())) {
                return OnContinue();
            }
            return true;
        }

        if (auto workLoop = pluginPtr->GetWorkLoop()) {
            workLoop->AddEvent(pluginPtr, shared_from_this());
        } else {
            pluginPtr->OnEventProxy(shared_from_this());
        }
    } else {
        return OnContinue();
    }
    return true;
}

bool PipelineEvent::OnFinish()
{
    {
        uint64_t nowTime = TimeUtil::GenerateTimestamp();
        processTime_ = nowTime > createTime_ ? (nowTime - createTime_) : 0;
    }
    if (handler_ != nullptr) {
        handler_->Recycle(this);
    }

    hasFinish_ = true;
    return true;
}

uint32_t PipelineEvent::GetPendingProcessorSize()
{
    return processors_.size();
}

void PipelineEvent::SetPipelineInfo(const std::string& pipelineName, std::list<std::weak_ptr<Plugin>>& processors)
{
    pipelineName_ = pipelineName;
    processors_ = processors;
}

std::string PipelineEvent::GetPipelineInfo()
{
    return pipelineName_;
}

void PipelineEvent::FillPipelineInfo(std::shared_ptr<Plugin> caller, const std::string& pipelineName,
                                     std::shared_ptr<PipelineEvent> event, bool deliverFromCurrent)
{
    if (caller == nullptr || event == nullptr || caller->GetHiviewContext() == nullptr) {
        return;
    }

    auto seq = caller->GetHiviewContext()->GetPipelineSequenceByName(pipelineName);
    if (deliverFromCurrent) {
        while (!seq.empty()) {
            auto& plugin = seq.front();
            if (auto pluginPtr = plugin.lock()) {
                if (pluginPtr->GetName() == caller->GetName()) {
                    break;
                }
            }
            seq.pop_front();
        }
    }
    event->SetPipelineInfo(pipelineName, seq);
}

bool Pipeline::CanProcessEvent(std::shared_ptr<PipelineEvent> event)
{
    if (processors_.empty()) {
        HIVIEW_LOGI("no processor in this pipeline.");
        return false;
    }

    std::weak_ptr<Plugin> plugin = processors_.front();
    if (auto pluginPtr = plugin.lock()) {
        return pluginPtr->CanProcessEvent(std::dynamic_pointer_cast<Event>(event));
    }
    return false;
}

void Pipeline::ProcessEvent(std::shared_ptr<PipelineEvent> event)
{
    event->SetPipelineInfo(name_, processors_);
    event->OnContinue();
}

void Pipeline::AppendProcessor(std::weak_ptr<Plugin> plugin)
{
    processors_.push_back(plugin);
}

void Pipeline::RemoveProcessor(std::weak_ptr<Plugin> plugin)
{
    processors_.remove_if([plugin](std::weak_ptr<Plugin> wp) {
        std::shared_ptr<Plugin> cur = plugin.lock();
        std::shared_ptr<Plugin> sp = wp.lock();
        if (cur != nullptr && sp != nullptr) {
            return cur == sp;
        }
        return false;
    });
}
} // namespace HiviewDFX
} // namespace OHOS
