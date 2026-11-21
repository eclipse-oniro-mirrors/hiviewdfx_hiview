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
#include "event_source_test.h"

#include <list>
#include <memory>

#include "defines.h"
#include "event_source.h"
#include "pipeline.h"
#include "plugin.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
class TestEventSource : public EventSource {
public:
    void Recycle(PipelineEvent* event __UNUSED) override {}
    void PauseDispatch(std::weak_ptr<Plugin> plugin __UNUSED) override {}
};

class TestPipelineEventProducer : public PipelineEventProducer {
public:
    void Recycle(PipelineEvent* event __UNUSED) override {}
    void PauseDispatch(std::weak_ptr<Plugin> plugin __UNUSED) override {}
};
}

/**
 * @tc.name: EventSourceAddPipelineTest001
 * @tc.desc: add nullptr pipeline to event source
 * @tc.type: FUNC
 */
HWTEST_F(EventSourceTest, EventSourceAddPipelineTest001, TestSize.Level3)
{
    auto source = std::make_shared<TestEventSource>();
    source->SetName("TestEventSource");
    source->AddPipeline(nullptr);
    auto producer = std::make_shared<TestPipelineEventProducer>();
    auto event = std::make_shared<PipelineEvent>("test", producer.get());
    ASSERT_FALSE(source->PublishPipelineEvent(event));
}

/**
 * @tc.name: EventSourceAddPipelineTest002
 * @tc.desc: add valid pipeline to event source
 * @tc.type: FUNC
 */
HWTEST_F(EventSourceTest, EventSourceAddPipelineTest002, TestSize.Level3)
{
    auto source = std::make_shared<TestEventSource>();
    source->SetName("TestEventSource");

    auto plugin = std::make_shared<Plugin>();
    plugin->SetName("TestPlugin");
    std::list<std::weak_ptr<Plugin>> pluginList;
    pluginList.push_back(plugin);
    auto pipeline = std::make_shared<Pipeline>("TestPipeline", pluginList);
    source->AddPipeline(pipeline);

    auto producer = std::make_shared<TestPipelineEventProducer>();
    auto event = std::make_shared<PipelineEvent>("test", producer.get());
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    ASSERT_TRUE(source->PublishPipelineEvent(event));
}

/**
 * @tc.name: EventSourcePublishEventTest001
 * @tc.desc: publish nullptr event
 * @tc.type: FUNC
 */
HWTEST_F(EventSourceTest, EventSourcePublishEventTest001, TestSize.Level3)
{
    auto source = std::make_shared<TestEventSource>();
    source->SetName("TestEventSource");
    ASSERT_FALSE(source->PublishPipelineEvent(nullptr));
}

/**
 * @tc.name: EventSourcePublishEventTest002
 * @tc.desc: publish event with no pipeline added
 * @tc.type: FUNC
 */
HWTEST_F(EventSourceTest, EventSourcePublishEventTest002, TestSize.Level3)
{
    auto source = std::make_shared<TestEventSource>();
    source->SetName("TestEventSource");
    auto producer = std::make_shared<TestPipelineEventProducer>();
    auto event = std::make_shared<PipelineEvent>("test", producer.get());
    ASSERT_FALSE(source->PublishPipelineEvent(event));
}

/**
 * @tc.name: EventSourcePublishEventTest003
 * @tc.desc: publish event when pipeline has no processors
 * @tc.type: FUNC
 */
HWTEST_F(EventSourceTest, EventSourcePublishEventTest003, TestSize.Level3)
{
    auto source = std::make_shared<TestEventSource>();
    source->SetName("TestEventSource");
    source->StartEventSource();

    std::list<std::weak_ptr<Plugin>> emptyList;
    auto pipeline = std::make_shared<Pipeline>("EmptyPipeline", emptyList);
    source->AddPipeline(pipeline);

    auto producer = std::make_shared<TestPipelineEventProducer>();
    auto event = std::make_shared<PipelineEvent>("test", producer.get());
    ASSERT_FALSE(source->PublishPipelineEvent(event));
}
