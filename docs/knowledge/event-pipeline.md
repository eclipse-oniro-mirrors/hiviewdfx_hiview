# 事件流水线知识

本文只记录事件从 HiSysEvent 桩点到插件处理的主链路中容易被改错的边界。插件注册、生命周期见 `plugin-lifecycle.md`，故障日志处理见 `faultlogger-management.md`，路径与权限见 `path-and-ipc-safety.md`。

## 主链路

事件处理应保持阶段清晰：

1. HiSysEvent 桩点通过 Unix domain socket 将原始事件发送到 EventServer。
2. `SocketDevice::ReceiveMsg` 校验 `uCredPid_`/`uCredUid_` 后交给 `SysEventReceiver::HandlerEvent`。
3. `SysEventReceiver` 将 `EventRaw::RawData` 包装为 `SysEvent` 并调用 `EventSource::PublishPipelineEvent`。
4. `SysEventSource` 按已匹配的 `DispatchRule` 找到目标 `Pipeline`，调用 `Pipeline::ProcessEvent`。
5. 流水线按配置顺序依次调用各插件的 `IsInterestedPipelineEvent` + `OnEvent`。
6. 插件通过 `OnEvent` 处理事件，可调用 `PipelineEvent::OnContinue` 推进到下一个插件、`OnPending` 暂挂、`OnFinish` 结束。

策略决策（事件分发到哪条流水线、订阅匹配）不要下沉到底层 socket 接收或 RawData 编解码代码。改变分发规则、订阅匹配或流水线顺序时，先理解 `DispatchRule` 和 `pipelineRules_` 的关系。

## 事件订阅两套接口（禁止混合）

| 模式 | 入口 | 数据结构 | 代理可加载 |
| --- | --- | --- | --- |
| 独立 EventListener | `RegisterUnorderedEventListener` | 平台 `listeners_` map | 不能动态加载/卸载 |
| 插件动态订阅 | `Plugin::AddDispatchInfo` + `OnEventListeningCallback` | 平台 `dispatchers_` map | 可动态加载 |

两套接口的订阅规则**互相不可见**：用 `AddListenerInfo` 注册的事件只会被 `listeners_` 中的 `EventListener` 接收；用 `AddDispatchInfo` 注册的事件只会被 `dispatchers_` 中的 `Plugin` 接收。混合调用会导致事件丢失。

## 高频路径

`PublishPipelineEvent`、`Pipeline::ProcessEvent`、`Plugin::OnEvent`、`PostUnorderedEvent` 属于高频路径。不要在循环中增加整表扫描、字符串格式化、INFO 日志或重复上下文创建。事件分发线程为高优先级（`sysevent_source`/`sysevent_store` 线程 nice -20），不要做阻塞 IO。

## 测试指引

- 事件源与流水线集成：使用 `HolisticPlatformTest`（`test/BUILD.gn`），它加载全部示例插件和 `holistic_platform/plugin_config`。
- 流水线分发：使用 `PluginPipelineTest`（`base/test/BUILD.gn`）。
- 事件 JSON 解析与定义：使用 `EventJsonParserTest`（`base/test/BUILD.gn`）。
- 分发规则解析：使用 `DispatchRuleParserTest`（`base/test/BUILD.gn`）。
- SysEvent 对象：使用 `SysEventTest`（`base/test/BUILD.gn`）。
- 事件分发队列：使用 `EventDispatchQueueTest`（`core/test/BUILD.gn`）。
- 示例插件参考实现在 `test/plugins/examples/`（`event_source_example`、`event_processor_example1~5`）。
- 依赖真实事件源 socket 或 samgr 时，补充 `board-verification.md` 中的板侧证据。
