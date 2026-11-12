# Routing — leak_detectors 知识路由

> 本文档描述 leak_detectors 的三维路由（任务触发 / 路径触发 / 词汇触发）+ 功能线分析（调用链）。
> Routing 的核心产出形式是功能线分析，调用链即路由链路。
> 代码位置：`base/hiviewdfx/hiview/plugins/reliability/leak_detectors`

---

## 1. 任务触发路由

| 场景 | 先读什么 | 说明 |
|------|---------|------|
| 新增一种泄漏检测类型 | `base/fault_common.h`（`DetectorType` enum）+ `fault_detector_manager.cpp`（`PrepareFaultDetectorEnv`）+ 任一现有 detector（推荐 `thread_leak/`） | 需要新增 enum 值 + 在 `detectorList_` 注册 + 实现 `FaultDetectorBase` 子类 + 5 状态机（judge/dump/report/appreport/removal） |
| 新增内核内存监控类型 | `leak_config/kernel_leak_config.json` 的 `kernel_type` + `kernel_leak/kernel_leak_detector.cpp` | JSON 增加 kernel_type 条目 |
| 调整采样间隔 | `fault_detector_manager.cpp` 的 `taskInterval_` + 各 detector 的 `HandleDetector(interval)` | 统一由 manager 调度 |
| 排查"事件没上报" | `base/fault_common.h`（事件名常量）+ 各 detector 的 `report_state` | 检查 report 状态是否被触发 |
| 排查"plugin 没加载" | `leak_config/leakdetectors_plugin_config` + `fault_detector_manager.cpp` 的 `ReadyToLoad` | `ReadyToLoad` 返回 false 会阻止加载（三个开关都 disable 时） |
| 排查"FD dump 超时" | `fd_leak/fd_leak_dump_state.cpp` + fork 子进程逻辑 | fork+pipe+waitpid 超时控制 |
| 排查"某进程不被内存监控" | `native_leak/native_leak_detector.cpp` 的 `monitoredPidList_` + `leak_config/native_leak_config.json` | 名字必须与 `/proc/pid/comm` 一致 |
| 启用/关闭某类泄漏检测 | `setparam hiview.fdleakcheck / hiview.threadleakcheck / hiview.memleakcheck disable` | 各 detector 通过 `IsXxxLeakEnable` 读取 |
| 启用测试模式 | `setparam persist.hiview.leak_detector enable` | `IsBetaOrDevelopMode` 读取，进入 develop 模式 |
| 修改 Commercial/Beta 配置 | `config/fault_config_manager.cpp` 的 `ParseConfigFile` | 先 Commercial base，Beta/Develop 模式再 Beta override |
| 新增 Telemetry 配额类型 | `telemetry_manager.h`（`TelemetryType` enum）+ `EventQuota` struct | 配额管理受 `#ifdef DFR_LEAKDETECTOR_CONFIG_TELEMETRY` 控制 |
| 修改状态机流程 | `base/fault_common.h`（`FaultStateType` 17 状态）+ 各 detector 的 state_context | 状态机是核心编排逻辑 |
| 启用 RSS 泄漏检测 | 仅非 Beta 版本自动启用，无需手动开关 | `PrepareFaultDetectorEnv` 中 `if (!IsBetaVersion())` |
| 修改事件处理 | `fault_detector_manager.cpp` 的 `OnEvent` + `event_handler/` | 系统事件入口（VMA/低内存/进程 kill/内核事件等） |
| 排查"kill 回调 AppEvent 发送失败" | `kernel_leak/kernel_leak_info.h` 的 `OverBaseProcesses.uid` + `kernel_leak/kernel_leak_detector.cpp` | uid 未设置默认 0，hiappevent 校验失败导致 AppEvent（external 维测）无法发送（DTS2026050926030） |
| 排查"kill 后 PreloadApplication 不恢复" | `native_leak/native_leak_removal_state.cpp` 的 `HandleKillProcess` | kill 后必须调 `SetProcessDied(CheckIfProcessAlive())` 回写结果，否则 `IsDied()` 始终 false（DTS2026050926030） |
| 排查"WaitingKillList 中进程被重复 dump" | `kernel_leak/kernel_leak_detector.cpp` 的 `OverLimitEvent` 状态转移 | 状态转移前必须检查 `WaitingKillList::IsInWaitingKillList`，命中则进入 `PROC_FINISHED_STATE`（DTS2026050926030） |
| 排查"JSVM/ArkWeb JS engine OOM 不上报" | `fault_detector_manager.cpp` 的 `HandleJsvmOomEvent`/`HandleV8OomEvent` + `notify_leak/notify_leak_detector.cpp` 的 rawheap 文件处理 | OOM 事件经 NotifyLeakDetector 走 OOMDump 流程，ArkWeb JS engine 会生成 2 个 rawheap 文件需替换保留较新者 |

---

## 2. 路径触发路由

| 改什么路径 | 注意什么 | 先读什么 |
|-----------|---------|---------|
| `leak_config/leak_detector_config.json` | 安装路径 `/system/etc/hiview/`；**Commercial=默认base，Beta=差异化override** | `config/fault_config_manager.cpp` 的 `ParseConfigFile` |
| `leak_config/kernel_leak_config.json` | Beta 节点含 enable/upload_interval/sample_time/pressure/rs_watchpoint/kernel_type | `kernel_leak/kernel_leak_detector.cpp` |
| `leak_config/native_leak_config.json` | 含 av_codec_config/threshold/resource_quota | `native_leak/native_leak_detector.cpp` |
| `leak_config/leakdetectors_plugin_config` | 格式严格（`Name[thread:ClassName]:0 static`），唯一 plugin `FaultDetectorManager` | `fault_detector_manager.cpp` 的 `OnLoad` |
| `base/fault_common.h` 的 `FaultStateType` enum | 17 状态串联，修改顺序易导致状态机错乱 | 各 detector 的 state_context |
| `base/fault_common.h` 的 `DetectorType` enum | 新增 detector 类型需同步 `PrepareFaultDetectorEnv` | `fault_detector_manager.cpp` |
| `base/fault_common.h` 的 `MEMCHECK_MAGIC` | 与内核 memcheck 驱动约定，不匹配则 ioctl 全部失败 | `native_leak/native_leak_sample.cpp` |
| `base/fault_detector_base.h` | 抽象基类，修改影响全部 6 个 detector | 所有 detector 子类 |
| `config/fault_config_manager.cpp` 的 `ParseConfigFile` | Commercial + Beta 合并逻辑，顺序敏感（先 Commercial 后 Beta） | `config/fault_config.{h,cpp}` |
| `telemetry_manager.h` 的 `#ifdef DFR_LEAKDETECTOR_CONFIG_TELEMETRY` | 条件编译关闭时全部方法为 stub，返回 false | `telemetry_manager.cpp` |

---

## 3. 词汇触发路由

| 术语 | 含义 | 指向 |
|------|------|------|
| FaultDetectorManager | **唯一 hiview Plugin 入口**，编排 6 类 detector | `fault_detector_manager.h:42` |
| DetectorType | 6 类检测器枚举：FD/THREAD/NOTIFY/KERNEL/NATIVE/RSS_LEAK_DETECTOR | `base/fault_common.h` |
| FaultStateType | 17 状态枚举（PROC_IDLE_STATE...PROC_FINISHED_STATE） | `base/fault_common.h:296-314` |
| FaultDetectorBase | 所有 detector 的抽象基类 | `base/fault_detector_base.h` |
| FaultStateBase | 所有状态对象的抽象基类 | `base/fault_state_base.h` |
| FaultStateContextBase | 状态上下文基类，持有 5 状态对象 | `base/fault_state_context_base.h` |
| FaultConfigManager | 配置解析管理器，实现 Commercial base + Beta override | `config/fault_config_manager.cpp` |
| TelemetryManager | Telemetry 灰度配额管理，条件编译 | `telemetry_manager.h:178` |
| TelemetryType | SYSTEM_TELEMETRY=1001 / RANDOM_TELEMETRY=1002 | `telemetry_manager.h:145-148` |
| EventQuota | Telemetry 配额结构体（deliveryTs/bundleName/faultType/quota/type/filterList 等） | `telemetry_manager.h:150-170` |
| Commercial / Beta | **Commercial=默认base，Beta=差异化override**；`ParseConfigFile` 先 Commercial 后 Beta override | `config/fault_config_manager.cpp` |
| IsBetaOrDevelopMode | 判断是否为 Beta 或 Develop 模式（`const.logsystem.versiontype=beta` 或 `persist.hiview.leak_detector`） | `config/fault_config_manager.cpp` |
| MEMCHECK_MAGIC | `0x5377FEFA`，与内核 memcheck 驱动约定 | `base/fault_common.h` |
| MEMCHECKIO | `0xAF`，ioctl 命令字基础 | `base/fault_common.h` |
| LOGGER_MEMCHECK_RSS_READ | 用户态 PSS 采样 ioctl 命令 | `native_leak/native_leak_sample.cpp` |
| PSS_MEMORY / KERNEL_MEMORY | 用户态 PSS 内存类型 vs 内核态 meminfo 内存类型 | 各 detector 的 info 文件 |
| monitoredPidList_ | 监控 PID 列表，各 detector 维护 | `base/fault_detector_base.h` |
| leakingPidsInfo_ | 待处理/处理中的泄漏 PID 信息 | `base/fault_detector_base.h` |
| blockList_ | 阻止列表 | `base/fault_detector_base.h` |
| appThresholds_ | App 资源阈值 | `base/fault_detector_base.h` |
| cmdState_ | native_leak dump 状态机（ENABLE/DISABLE/LONG_DELAY/SAVE/PROCESS） | `native_leak/native_leak_dump_state.cpp` |
| sample_time / pressure | **Telemetry 灰度机制**：按 内存压力等级动态调整采样频率 | `leak_config/kernel_leak_config.json` |
| RSS_LEAK | 仅非 Beta 版本启用的 RSS 维度检测，由 RssLeakDispatcher 编排 | `rss_leak/rss_leak_dispatcher.cpp` |
| RssLeakDispatcher | RSS 泄漏编排器，继承 FaultDetectorBase | `rss_leak/rss_leak_dispatcher.cpp` |
| RssLeakTracker | RSS 泄漏跟踪器 | `rss_leak/rss_leak_tracker.cpp` |
| resourceType | OOM/泄漏上报资源类型（js_heap/jsvm_heap/arkweb js heap） | `notify_leak/notify_leak_app_report_state.cpp` |

---

## 4. 功能线分析

### 4.0 功能线分类维度

本模块共 6 条功能线，按**检测器类型**（业务域）分类，编号规则 `FL-DD`（DD=detector 编号）。

| 功能线 | 检测器 | 状态机 |
|--------|--------|--------|
| FL-01 | FD 泄漏检测 | judge → dump → report → appreport → removal |
| FL-02 | 线程泄漏检测 | judge → dump → report → appreport → removal |
| FL-03 | 内核态内存泄漏检测 | judge → dump → report → removal |
| FL-04 | 用户态内存泄漏检测 | judge → dump（5 子状态）→ report → appreport → removal |
| FL-05 | Notify 泄漏检测 | judge → dump（fork 子进程）→ report → removal |
| FL-06 | RSS 内存泄漏检测 | judge → dump → report → removal（仅非 Beta） |

### FL-01: FD 泄漏检测功能线

| 阶段 | 调用点 | 说明 |
|------|--------|------|
| 入口 | `FaultDetectorManager::OnLoad` → `PrepareFaultDetectorEnv` → `detectorList_.push_back(FD_LEAK_DETECTOR)` | `hiview.fdleakcheck` 开关启用时加入 |
| 编排 | `MonitorProcess` switch-case → `FdLeakDetector::HandleDetector(taskInterval_)` | `fault_detector_manager.cpp` |
| 判定 | `fd_leak_judge_state`：扫描 `/proc/pid/fd` 数量超阈值 | 入 `leakingPidsInfo_` |
| Dump | `fd_leak_dump_state`：fork 子进程读取 `/proc/pid/fd` 详情，`CountFdLinkNum` 按 socket/pipe/dmabuf/eventfd/eventpoll 分类 | hiview 无权限直接读 |
| 落盘 | `fd_leak_dump_state`：写入 `{pid}_fd_leak.txt` | 落盘目录 `/data/log/reliability/resource_leak/fd_leak` |
| 上报 | `fd_leak_report_state`：构造 `EVENT_ID_FD_LEAK` 事件，`PostUnorderedEvent` | 事件 ID 常量 |
| App 上报 | `fd_leak_appreport_state`：App 维度上报 | |
| 移除 | `fd_leak_removal_state`：清理已处理 PID | |

### FL-02: 线程泄漏检测功能线

| 阶段 | 调用点 | 说明 |
|------|--------|------|
| 入口 | `PrepareFaultDetectorEnv` → `detectorList_.push_back(THREAD_LEAK_DETECTOR)` | `hiview.threadleakcheck` 开关启用时 |
| 编排 | `MonitorProcess` → `ThreadLeakDetector::HandleDetector` | |
| 判定 | `thread_leak_judge_state`：扫描 `/proc/pid/task` 数量超阈值 | 按 `__LP64__` 选择 32/64 位阈值 |
| Dump | `thread_leak_dump_state`：写 `{pid}_thread_leak.txt`，含 tid/name/startTime | |
| 上报 | `thread_leak_report_state`：构造 `EVENT_ID_THREAD_LEAK` 事件 | |
| App 上报 | `thread_leak_appreport_state` | |
| 移除 | `thread_leak_removal_state` | |

### FL-03: 内核态内存泄漏检测功能线

| 阶段 | 调用点 | 说明 |
|------|--------|------|
| 入口 | `PrepareFaultDetectorEnv` → `detectorList_.push_back(KERNEL_LEAK_DETECTOR)` | `hiview.memleakcheck` 开关启用时 |
| 编排 | `MonitorProcess` → `KernelLeakDetector::HandleDetector` | |
| 判定 | `kernel_leak_judge_state`：读 `/proc/meminfo`，正则匹配 `meminfoName`（slub/vmalloc/ashmem/ion/cma/gpu） | 周期采样判定 |
| Dump | `kernel_leak_dump_state` | |
| 上报 | `kernel_leak_report_state` | |
| 移除 | `kernel_leak_removal_state` | |

### FL-04: 用户态内存泄漏检测功能线

| 阶段 | 调用点 | 说明 |
|------|--------|------|
| 入口 | `PrepareFaultDetectorEnv` → `detectorList_.push_back(NATIVE_LEAK_DETECTOR)` | `hiview.memleakcheck` 开关启用时 |
| 编排 | `MonitorProcess` → `NativeLeakDetector::HandleDetector` | 核心编排器 |
| 判定 | `native_leak_judge_state`：ioctl `/dev/bbox` 读 PSS/JavaPSS/NativePSS，最近 8 次均超阈值 → 判定泄漏 | 依赖内核 memcheck 驱动 |
| Dump | `native_leak_dump_state`：5 状态机（ENABLE→DISABLE→LONG_DELAY→SAVE→PROCESS） | `cmdState_` 状态机 |
| 落盘 | `native_leak_dump_state`：写入 `MEMORY_LEAK_PATH` | |
| 上报 | `native_leak_report_state`：构造 `EVENT_ID_MEMORY_LEAK=901003023` 事件 | |
| App 上报 | `native_leak_appreport_state` | |
| 移除 | `native_leak_removal_state` | |
| 辅助 | `ServiceLeakCheck` / `SysloadDetection` / `JudgeMemmgrProcessOverThreshold` / `SetSplitMemory` | `NativeLeakDetector` 独有方法 |

### FL-05: Notify 泄漏检测功能线

| 阶段 | 调用点 | 说明 |
|------|--------|------|
| 入口 | `PrepareFaultDetectorEnv` → `detectorList_.push_back(NOTIFY_LEAK_DETECTOR)` | **仅在内存泄漏启用时加载** |
| 编排 | `MonitorProcess` → `NotifyLeakDetector::HandleDetector` | |
| 判定 | `notify_leak_judge_state` | |
| Dump | `notify_leak_dump_state` + `NotifyLeakDealForkProcess`：fork 子进程处理 | |
| 上报 | `notify_leak_report_state` | |
| 移除 | `notify_leak_removal_state` | |
| OOM 路径 | `HandleJsvmOomEvent`/`HandleV8OomEvent` → `NotifyLeakDetector::ProcessEventListenening("OOMDump", ...)` | ArkWeb JS engine 生成 2 个 rawheap 文件，`TryReplaceArkwebRawheap` 保留较新者，`AdjustArkwebWaitTime` 延迟 30s 等待第 2 个文件 |

### FL-06: RSS 内存泄漏检测功能线

| 阶段 | 调用点 | 说明 |
|------|--------|------|
| 入口 | `PrepareFaultDetectorEnv` → `if (!IsBetaVersion()) detectorList_.push_back(RSS_LEAK_DETECTOR)` | **仅非 Beta 版本启用** |
| 编排 | `MonitorProcess` → `RssLeakDispatcher::HandleDetector` | dispatcher 编排 tracker + telemetry handler |
| 跟踪 | `RssLeakTracker`：RSS 维度跟踪与判定 | |
| 上报 | `RssAppTelemetryHandler` / `RssRandomTelemetryHandler` / `RssSystemTelemetryHandler` | 按 app/random/system 三类上报 |
| 进程退出 | `RssLeakDispatcher::NotifyProcessDied` | 通知 tracker 与 telemetry handler 清理 |

---

## 5. 状态机通用流程（跨 detector）

所有 detector 遵循统一的状态机流程（`FaultStateType` 17 状态），核心路径：

```
PROC_IDLE_STATE → PROC_SAMPLE_STATE → PROC_JUDGE_STATE
  → PROC_DUMP_STATE → PROC_DUMP_KILL_STATE → PROC_TELEMETRY_STATE
  → PROC_REPORT_STATE → PROC_REPORT_OVERLIMIT_STATE
  → PROC_APP_REPORT_STATE → PROC_APP_REPORT_KILL_STATE
  → PROC_PREDICTION_STATE → PROC_PREVENTION_STATE → PROC_TORLERANCE_STATE
  → PROC_REMOVAL_STATE → PROC_CREATE_STATE → PROC_DIED_STATE → PROC_FINISHED_STATE
```

各 detector 的 state_context 持有 5 个核心状态对象：
- `judgeState_`：判定是否泄漏
- `dumpState_`：dump 详情并落盘
- `reportState_`：上报事件
- `appReportState_`：App 维度上报
- `removalState_`：清理已处理 PID

状态切换通过 `FaultDetectorBase::OnChangeState` + `ExeNextStateProcess` 驱动。
