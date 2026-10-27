# Trace 采集流控知识

本文只记录 trace 采集的状态机切换、配额管理、overflow 判定的边界。统一采集器接口见 `unified-collection.md`，路径与权限见 `path-and-ipc-safety.md`。

## 主链路

trace 采集处理应保持阶段清晰：

1. `UnifiedCollector` 监听 trace 开关事件（`OnSwitchRecordTraceStateChanged`/`OnMainThreadJank`）。
2. `TraceCollectorImpl::DumpTrace(caller)` 开始采集。
3. `TraceStateMachine` 管理状态切换（Common/Command/App/Dynamic/Telemetry）。
4. `TraceFlowController` 校验配额（zip/io/app overflow）。
5. 采集完成后存储到 `TraceStorage`/`AppTraceStorage`。
6. `TraceCacheMonitor` 在低内存阈值时缓存 trace。

策略决策（是否允许采集、配额是否超限）不要下沉到 trace 采集的底层执行代码。改变状态切换或配额逻辑时，先理解状态机约束和配额配置。

## 状态机

`framework/native/unified_collection/trace_manager/` 的 `TraceStateMachine` 管理以下状态：

| 状态 | 类 | 含义 |
| --- | --- | --- |
| Common | `TraceCommonState` | 常规 trace 采集 |
| Command | `TraceCommandState` | 命令行触发 |
| App | `TraceAppState` | 应用 trace |
| Dynamic | `TraceDynamicState` | 动态 trace |
| Telemetry | `TraceTelemetryState` | 遥测 trace |

状态切换有严格约束：不是所有状态都能互相切换，非法切换会被拒绝。Telemetry 状态有独立的状态机（`telemetry/TelemetryStateMachine`）。

## 存储与清理

- `TraceStorage`/`AppTraceStorage`/`TraceBehaviorStorage`/`TeleMetryStorage`/`AppEventTaskStorage` 管理不同来源的 trace 存储。
- `TraceDbCallback` 处理数据库回调。
- `StoreTraceSize` 记录已用大小。
- `RecordCaller` 记录调用者，用于配额追踪。

## 高频路径

trace 采集和配额校验不是高频路径，但 `TraceCacheMonitor` 的低内存检查是周期性任务。不要在配额校验中增加阻塞 IO 或长耗时操作。

## 修改前检查

- 状态切换是否合法（不是所有状态都能互切）？
- 新增调用者是否需要在 `trace_quota_config.json` 配置配额？
- 配额校验是否覆盖 zip/io/app 三种 overflow？
- trace 参数（buffer/duration/prefix）是否在允许范围内？
- Telemetry 配额是否与 `telemetry_storage` 的查询逻辑一致？
- 低内存阈值（`hiview_unified_collector_low_mem_threshold`，`hiview.gni`）是否影响 TraceCacheMonitor？

## 测试指引

- Trace 策略：使用 `TraceStrategyTest`、`TraceStrategyExTest`。
- Trace 实现：使用 `TraceImplTest`、`TraceUtilsTest`。
- Trace 管理：使用 `TraceManagerTest`。
- 客户端接口：使用 `UCollectionTraceUnitTest`，受 `hiview_unified_collector_trace_enable` 控制。
- 统一采集器 CPU 存储：使用 `CpuStorageTest`。
- 涉及真实 trace 采集、低内存阈值或设备 trace 节点时，补充 `board-verification.md` 中的板侧证据。
