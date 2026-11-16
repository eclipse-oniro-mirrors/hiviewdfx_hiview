# Expert — leak_detectors 专家经验

> 本文档描述 leak_detectors 的禁止事项、架构不变量、常见失败模式、修复策略。
> Expert 知识是 Agent 在修改代码时必须遵守的约束，违反可能导致线上故障。
> 代码位置：`base/hiviewdfx/hiview/plugins/reliability/leak_detectors`

---

## 1. 禁止事项清单

1. **禁止直接在 hiview 主线程读取 `/proc/pid/fd`** — hiview 无权限，必须 fork 子进程（fd_leak_dump_state / notify_leak_deal_fork_process）。
2. **禁止修改 `MEMCHECK_MAGIC = 0x5377FEFA`** — 与内核 memcheck 驱动约定，不匹配则 ioctl 全部失败（`base/fault_common.h`）。
3. **禁止跳过 `ReadyToLoad` 中的参数检查** — 三个开关都 disable 时 `ReadyToLoad` 返回 false 阻止加载，商用版必须能通过 `hiview.*leakcheck=disable` 关闭（`fault_detector_manager.cpp`）。
4. **禁止修改 `leakdetectors_plugin_config` 的格式** — `Name[thread:ClassName]:0 static` 格式严格，错误导致 plugin 不加载（`leak_config/leakdetectors_plugin_config`）。
5. **禁止在 `native_leak_dump_state` 的 `cmdState_` 状态机中被异常跳过状态** — 会破坏 ENABLE/DISABLE/LONG_DELAY/SAVE/PROCESS 5 状态机，导致后续泄漏进程无法处理。
6. **禁止假设 Commercial 节点改动不生效** — 新版代码 `FaultConfigManager::ParseConfigFile` **已实现 Commercial base + Beta override 合并逻辑**：先 `ParseConfigByVersion("Commercial")` 作 base，Beta/Develop 模式再 `ParseConfigByVersion("Beta")` override。
7. **禁止假设 `sample_time/pressure` 灰度机制已完全启用** — `pressureList` 在 `kernel_leak_config.json` 中被解析，但完整 内存压力动态采样调整逻辑需确认各 detector 是否已接入。
8. **禁止在 Beta 版本启用 RSS_LEAK** — `PrepareFaultDetectorEnv` 中 `if (!IsBetaVersion())` 是硬性约束，RSS_LEAK 仅非 Beta 版本启用。
9. **禁止修改 `FaultStateType` enum 的状态顺序** — 17 状态串联，顺序敏感，修改易导致状态机错乱（`base/fault_common.h:296-314`）。
10. **禁止修改 `FaultDetectorBase` 抽象基类签名而不同步全部 6 个 detector 子类** — 基类被所有 detector 继承，签名变更影响全部子类。
11. **禁止在 OverLimitEvent 等事件处理路径的状态转移中忽略 WaitingKillList** — 进程在 WaitingKillList 等待 kill 期间再次触发 OverLimitEvent 时，必须进入 `PROC_FINISHED_STATE` 而非 `PROC_DUMP_STATE`/`PROC_REMOVAL_STATE`，否则会重复 dump 或中断 kill 流程（`kernel_leak/kernel_leak_detector.cpp` 的 `OverLimitEvent`，DTS2026050926030）。
12. **禁止 `OverBaseProcesses` 结构体缺失 `uid` 字段** — `uid` 未设置时默认为 0，下游 hiappevent 校验不通过，AppEvent（external 维测）事件无法发送，是最严重的维测信息缺失（`kernel_leak/kernel_leak_info.h`，DTS2026050926030）。
13. **禁止 kill 操作后不调用 `SetProcessDied` 回写结果** — 否则 `IsDied()` 始终为 false，`DoWaitKillPreload` kill 结果检查失效，`PreloadApplication` 恢复逻辑无法触发（`native_leak/native_leak_removal_state.cpp` 的 `HandleKillProcess`，DTS2026050926030）。
14. **禁止在 ArkWeb JS engine OOM 路径直接丢弃较早的 rawheap 文件而不比较时间戳** — ArkWeb JS engine OOM 生成 2 个 rawheap 文件（85% + 95% 阈值），`TryReplaceArkwebRawheap` 必须按文件名时间戳段比较，保留较新者，丢弃较旧者；直接覆盖会丢失有效的 95% 阈值 dump（`notify_leak/notify_leak_detector.cpp`）。

---

## 2. 架构不变量

1. **唯一 Plugin 入口是 `FaultDetectorManager`** — 通过 `REGISTER` 注册，`OnLoad` 启动定时任务（`update_memtrack_trace_time`/`start_sysload`/`pressure_threshold_set`/`detectorProcessLoop`），`OnUnload` 停止。不再是旧版的 3 个独立 plugin（`fault_detector_manager.h:42`）。
2. **6 类 detector 动态注册** — `PrepareFaultDetectorEnv` 根据系统参数开关填充 `detectorList_`：FD/THREAD/KERNEL/NATIVE 受 `hiview.*leakcheck` 控制，NOTIFY 仅在内存泄漏启用时加载，RSS 仅非 Beta 版本启用（`fault_detector_manager.cpp`）。
3. **统一监控循环** — `MonitorProcess` 通过 switch-case 分发到各 detector 的 `HandleDetector(taskInterval_)`，采样与上报分离通过 `TASK_LOOP_INTERVAL=5s` 定时器并行运行。
4. **状态机驱动** — 所有 detector 遵循 `FaultStateType` 17 状态枚举，各 state_context 持有 5 核心状态对象（judge/dump/report/appreport/removal），通过 `OnChangeState` + `ExeNextStateProcess` 驱动状态切换。
5. **Commercial = 默认 base，Beta = 差异化 override（已实现）** — `FaultConfigManager::ParseConfigFile` 先 `ParseConfigByVersion("Commercial")` 作 base，`IsBetaOrDevelopMode()` 返回 true 时再 `ParseConfigByVersion("Beta")` override。Commercial 节点改动**会生效**（`config/fault_config_manager.cpp`）。
6. **Telemetry 条件编译** — `TelemetryManager` 受 `#ifdef DFR_LEAKDETECTOR_CONFIG_TELEMETRY` 控制，关闭时全部方法为 stub（返回 false/空），不影响主流程（`telemetry_manager.h:178-211`）。
7. **leakdetectors.so 单一产物** — 编译产物装载唯一 plugin `FaultDetectorManager`，通过 `leakdetectors_plugin_config` 配置（`FaultDetectorManager[]:0 static`）。
8. **事件处理集中化** — 所有系统事件（VMA/低内存/进程 kill/内核事件/ashmem/ark stats/set memory/kernel zone/sysload/kernel thread/hisysevent/ui tree/abc kill/fork dump/xperf/JSVM OOM/ArkWeb JS engine OOM/RS FD warning）由 `FaultDetectorManager::OnEvent` 统一接收，分发到对应 handler。
9. **WaitingKillList 是 kill 流程的核心机制** — 进程被判定后台查杀后不立即 kill，加入 WaitingKillList 延迟校验仍满足条件再清理。任何在 WaitingKillList 等待期间可能再次触发的事件（如 OverLimitEvent）状态转移必须检查 `WaitingKillList::IsInWaitingKillList`，在 list 中的进程应进入 `PROC_FINISHED_STATE`（DTS2026050926030）。
10. **`OverBaseProcesses.uid` 是 AppEvent 发送的关键字段** — postAppevent 下游 hiappevent 对 pid+uid 做校验，uid 默认 0 会校验失败导致 AppEvent（external 维测）无法发送。新增事件处理路径构造 `OverBaseProcesses` 必须通过 `DfrUtil::GetInstance().GetProcessUid(pid)` 设置 uid（DTS2026050926030）。
11. **OOM resourceType 必须按 NotifyType 区分** — `NotifyLeakAppReportState` 的 `resourceType` 字段：JS_LEAK→"js_heap"、JSVM_LEAK→"jsvm_heap"、ARKWEB_LEAK→"arkweb js heap"。`HandleJsvmOomEvent`/`HandleV8OomEvent` 在构造 `NotifyProcessInfo` 时设置对应 resourceType，`NotifyLeakAppReportState` 通过 `FillResourceTypeByNotifyType` 二次确认。

---

## 3. 常见失败模式 / 反模式

| 失败模式 | 表现 | 正确做法 |
|---------|------|---------|
| 新增 detector 但忘记加 `DetectorType` enum | 编译失败或运行时无法识别 | 同步修改 `base/fault_common.h` 的 `DetectorType` |
| 新增 detector 但忘记在 `PrepareFaultDetectorEnv` 注册 | detector 不加载，无任何日志 | 同步修改 `fault_detector_manager.cpp` 的 `PrepareFaultDetectorEnv` |
| 新增 detector 但忘记在 `MonitorProcess` switch-case 添加分支 | detector 不被调度 | 同步修改 `MonitorProcess` |
| `leakdetectors_plugin_config` 格式错误 | plugin 不加载，无任何日志 | 严格遵循 `Name[thread:ClassName]:0 static` 格式 |
| `MEMCHECKIO` ioctl 失败 | 用户态采样全部返回 `res != 0` | 检查内核 memcheck 驱动是否加载、`/dev/bbox` 权限 |
| `cmdState_` 状态机被异常跳过 | `runningDebug_` 卡住，后续泄漏进程无法处理 | 状态切换失败时必须正确清理 |
| 修改 Commercial 节点期望生效 | **会生效**（新版已实现合并逻辑） | 直接修改 Commercial 节点即可，Beta/Develop 模式下 Beta override |
| Beta 版本启用 RSS_LEAK | 违反架构不变量 | `PrepareFaultDetectorEnv` 中 `if (!IsBetaVersion())` 是硬性约束 |
| 修改 `FaultStateType` enum 顺序 | 状态机错乱 | 禁止修改顺序，只能追加新状态 |
| 修改 `FaultDetectorBase` 签号 | 编译失败 | 同步修改全部 6 个 detector 子类 |
| `leak_detector_config.json` 中 Commercial/Beta 节点缺失 | 解析失败 | Commercial 是默认 base 必须存在，Beta 可选 |
| OverLimitEvent 状态转移未检查 WaitingKillList | 进程在 WaitingKillList 等待期间被重复 dump 或 kill 流程中断 | 状态转移前调用 `WaitingKillList::IsInWaitingKillList`，命中则进入 `PROC_FINISHED_STATE` |
| `OverBaseProcesses` 缺 uid | AppEvent（external 维测）发送失败，hiappevent 校验不通过 | 必须调用 `DfrUtil::GetInstance().GetProcessUid(pid)` 设置 uid |
| `HandleKillProcess` 后未调 `SetProcessDied` | `IsDied()` 始终 false，`PreloadApplication` 恢复逻辑失效 | kill 后调用 `nlInfo->SetProcessDied(nlInfo->CheckIfProcessAlive())` 回写结果 |
| ArkWeb JS engine OOM 第二个 rawheap 文件丢失 | dump 时使用旧的 85% 阈值文件，丢失 95% 阈值 dump | `TryReplaceArkwebRawheap` 比较时间戳保留较新者；`AdjustArkwebWaitTime` 延迟 30s 等待第二个文件 |
| OOM resourceType 与 NotifyType 不匹配 | hiappevent 上报 resourceType 错误，下游分析归类错误 | `FillResourceTypeByNotifyType` 按 NotifyType 返回对应字符串 |

---

## 4. 修复策略

| 问题 | 错误修法 | 正确修法 |
|------|---------|---------|
| 某进程不被内存监控 | 在代码中硬编码进程名 | 加到 `leak_config/native_leak_config.json` 配置 |
| 某内核内存类型不上报 | 修改 `meminfoName` | 检查 `kernel_leak_config.json` 中该类型的 `enable` 字段 |
| Beta 版本不监控 | 修改 `etc/memleak_tracker.cfg` | `leak_detector_config.json` 的 Beta 节点也要 enable |
| Plugin 加载失败 | 改 `OnLoad` 逻辑 | 先看 `ReadyToLoad` 是否返回 false（三个开关都 disable） |
| 商用版需要不同阈值 | 改 Commercial 节点 | **直接改 Commercial 节点即可**（新版已实现 Commercial base + Beta override） |
| 想启用 内存压力动态采样 | 改 `pressureList` 值 | 需确认各 detector 是否已接入 内存压力读取与 `sampleInterval_` 调整逻辑 |
| FD 泄漏检测超时 | 增大 `dumpTime` 配置值 | 先排查 fork 子进程是否被阻塞 |
| 新增 detector 不工作 | 只实现 detector 类 | 同步修改：`DetectorType` enum + `PrepareFaultDetectorEnv` 注册 + `MonitorProcess` 分发 + `leakdetectors_plugin_config` |
| Telemetry 配额不生效 | 修改 `TelemetryManager` 代码 | 检查 `DFR_LEAKDETECTOR_CONFIG_TELEMETRY` 宏是否定义 |
| 想在 Beta 版本启用 RSS_LEAK | 修改 `PrepareFaultDetectorEnv` 去掉 `IsBetaVersion` 判断 | 违反架构不变量，不应修改 |
| kill 回调 AppEvent 发送失败 | 改 hiappevent 校验逻辑 | 检查 `OverBaseProcesses.uid` 是否通过 `GetProcessUid(pid)` 设置（uid=0 校验失败） |
| kill 后 PreloadApplication 不恢复 | 改 PreloadApplication 逻辑 | 检查 `HandleKillProcess` 后是否调用 `SetProcessDied` 回写 kill 结果 |
| WaitingKillList 中进程被重复 dump | 改 dump_state 逻辑 | OverLimitEvent 状态转移前检查 `WaitingKillList::IsInWaitingKillList`，命中则 `PROC_FINISHED_STATE` |
| ArkWeb JS engine OOM 只 dump 一次 | 增大 waitTime | `AdjustArkwebWaitTime` 对 arkweb_jsheap 文件强制返回 `max(waitTime, PENDING_RAWHEAP_WAIT=30s)` |
| ArkWeb JS engine OOM 旧文件残留 | 不处理 | `TryReplaceArkwebRawheap` 比较时间戳后 `DfrUtil::RemoveFile` 删除较旧文件 |

---

## 5. 历史问题知识映射（预留）

当有历史问题/缺陷需要提取知识时，按九类→四类映射：

| 九类知识 | 映射到 | 本文档章节 |
|---------|--------|-----------|
| 架构边界 | expert | §2 架构不变量 |
| 高风险入口 | code_map | §4 高风险路径 |
| 隐式约束 | expert | §1 禁止事项 + §2 架构不变量 |
| 失效模式 | expert | §3 失败模式 |
| 根因模式 | expert | §4 修复策略 |
| 修复策略 | expert + routing | §4 修复策略 + routing §1 任务触发 |
| 验证回归 | verify | verify §3 Done 定义 + §5 观测点 |
| 运维观测 | verify | verify §5 关键观测点 |
| 变更风险 | expert | §2 架构不变量 + §3 失败模式 |
