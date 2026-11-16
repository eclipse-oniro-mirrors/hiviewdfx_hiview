# Code Map — leak_detectors 代码导航知识

> 本文档描述 leak_detectors 子模块的代码结构、模块职责、任务到路径映射、高风险路径。
> AGENTS.md 仅作为索引，详细内容在此文档。
> 代码位置：`base/hiviewdfx/hiview/plugins/reliability/leak_detectors`

---

## 1. 顶层模块职责与边界

| 模块 | 职责 | 边界 |
|------|------|------|
| `fault_detector_manager.{h,cpp}` | **唯一 hiview Plugin 入口**。`FaultDetectorManager` 继承 `Singleton`，持有 6 类 detector，编排监控循环 `MonitorProcess()`，处理系统事件（VMA/低内存/进程 kill/内核事件/ashmem/ark stats/set memory/kernel zone/sysload/kernel thread/hisysevent/ui tree/abc kill/fork dump/xperf/JSVM OOM/ArkWeb JS engine OOM/RS FD warning 等） | 不直接做采样/判定/落盘，委托给各 detector |
| `base/` | 框架抽象基类：`FaultDetectorBase` / `FaultStateBase` / `FaultInfoBase` / `FaultStateContextBase` + `fault_common.h`（枚举/常量/事件名） | 所有 detector 继承自此层，修改基类影响全部 detector |
| `config/` | 配置解析：`FaultConfigManager`（Commercial base + Beta override 合并）、`FaultConfig`（参数模型） | 解析 `leak_detector_config.json` |
| `detector_utils/` | 共享工具：`FaultDetectorUtil`（进程信息/文件操作）、`FaultStateFactory`（状态对象工厂）、`FaultLeakObject`（泄漏对象数据模型） | 不含业务判定逻辑 |
| `event_handler/` | 事件分发：`EventHandler` 接收系统事件并路由到对应 handler | 与 hiview event pipeline 对接 |
| `fd_leak/` | FD 泄漏检测：扫描 `/proc/pid/fd`，fork 子进程 dump 详情，按 fd 类型（socket/pipe/dmabuf/eventfd/eventpoll）分类计数 | 仅检测 FD，使用状态机 |
| `thread_leak/` | 线程泄漏检测：扫描 `/proc/pid/task` 数量，超阈值判定泄漏 | 仅检测线程数 |
| `kernel_leak/` | 内核态内存泄漏：监控 `/proc/meminfo`（slub/vmalloc/ashmem/ion/cma/gpu/kernel zone），周期采样判定 | 不含用户态 PSS 监控 |
| `native_leak/` | 用户态内存泄漏：监控 PSS（ioctl `/dev/bbox`），含 5 状态机（judge/dump/report/appreport/removal）、服务泄漏检测、sysload 检测、memmgr 超阈值、split memory | 不含 native heap 精确栈采集 |
| `notify_leak/` | Notify 泄漏检测：监控通知句柄泄漏，含 fork 子进程处理（`NotifyLeakDealForkProcess`） | 仅在内存泄漏启用时加载 |
| `rss_leak/` | RSS 内存泄漏：仅非 Beta 版本启用（`if (!IsBetaVersion())`），由 `RssLeakDispatcher` 编排，`RssLeakTracker` 跟踪，按 app/random/system 三类 telemetry handler 上报 | 不在 Beta/Develop 模式下运行 |
| `lite_diag/` | 轻量级诊断 数据采集 | 辅助模块 |
| `old_log_clean/` | 旧日志清理逻辑 | 维护性模块 |
| `process_kill/` | 进程 kill 处理 | 辅助模块 |
| `resource_overload/` | 资源过载处理 | 辅助模块 |
| `telemetry_manager.{h,cpp}` | Telemetry 灰度配额管理：`#ifdef DFR_LEAKDETECTOR_CONFIG_TELEMETRY` 条件编译，管理 EventQuota/GrayQuota、配额文件读写、压缩配额、电源连接配额 | 条件编译关闭时全部方法为 stub |
| `telemetry_reporter.{h,cpp}` | Telemetry 上报 | 与 TelemetryManager 配合 |
| `leak_config/` | 配置文件目录：`leak_detector_config.json`（主配置）、`kernel_leak_config.json`、`native_leak_config.json`、`leakdetectors_plugin_config`（plugin 加载清单） | 安装到 `/system/etc/hiview/` |

---

## 2. 关键目录结构

```
leak_detectors/
├── fault_detector_manager.{h,cpp}   # 唯一 Plugin 入口，6 detector 编排器
├── telemetry_manager.{h,cpp}        # Telemetry 灰度配额（条件编译）
├── telemetry_reporter.{h,cpp}       # Telemetry 上报
├── base/                            # 框架抽象层
│   ├── fault_common.h               # 枚举（DetectorType/FaultStateType）/常量/事件名
│   ├── fault_detector_base.{h,cpp}  # FaultDetectorBase 抽象基类
│   ├── fault_state_base.{h,cpp}     # FaultStateBase 抽象基类
│   ├── fault_info_base.{h,cpp}      # FaultInfoBase 抽象基类
│   └── fault_state_context_base.{h,cpp}
├── config/                          # 配置解析
│   ├── fault_config_manager.{h,cpp}
│   └── fault_config.{h,cpp}
├── detector_utils/                  # 共享工具
├── event_handler/                   # 事件分发
├── fd_leak/                         # FD 泄漏（detector + state_context + judge/dump/report/appreport/removal_state + info）
├── thread_leak/                     # 线程泄漏（同上结构）
├── kernel_leak/                     # 内核态内存泄漏（同上结构）
├── native_leak/                     # 用户态内存泄漏（同上结构 + 服务泄漏/sysload/memmgr/split memory）
├── notify_leak/                     # Notify 泄漏（同上结构 + DealForkProcess）
├── rss_leak/                        # RSS 内存泄漏（仅非 Beta，dispatcher + tracker + common + 3 telemetry handler）
├── lite_diag/                   # 轻量诊断
├── old_log_clean/                   # 旧日志清理
├── process_kill/                    # 进程 kill
├── resource_overload/               # 资源过载
├── leak_config/                     # 配置文件
│   ├── leak_detector_config.json
│   ├── kernel_leak_config.json
│   ├── native_leak_config.json
│   └── leakdetectors_plugin_config
└── docs/knowledge/                  # 知识库
    ├── code_map.md
    ├── routing.md
    ├── expert.md
    └── verify.md
```

---

## 3. 任务到路径映射

| 任务 | 先看这里 | 原因 |
|------|---------|------|
| 新增一种泄漏检测类型 | `base/fault_common.h`（`DetectorType` enum）+ `fault_detector_manager.cpp`（`PrepareFaultDetectorEnv`）+ 任一现有 detector（推荐 `thread_leak/`） | 需要新增 enum 值 + 在 `detectorList_` 注册 + 实现 `FaultDetectorBase` 子类 + 状态机 |
| 新增内核内存监控类型 | `leak_config/kernel_leak_config.json` 的 `kernel_type` + `kernel_leak/kernel_leak_detector.cpp` | JSON 增加 kernel_type 条目 |
| 调整采样间隔 | `fault_detector_manager.cpp` 的 `taskInterval_` + 各 detector 的 `HandleDetector(interval)` | 统一由 manager 调度 |
| 排查"事件没上报" | `base/fault_common.h`（事件名常量）+ 各 detector 的 `report_state` | 检查 report 状态是否被触发 |
| 排查"plugin 没加载" | `leak_config/leakdetectors_plugin_config` + `fault_detector_manager.cpp` 的 `ReadyToLoad` | `ReadyToLoad` 返回 false 会阻止加载（三个开关都 disable 时） |
| 排查"FD dump 超时" | `fd_leak/` 的 dump_state + fork 子进程逻辑 | fork+pipe+waitpid 超时控制 |
| 排查"某进程不被内存监控" | `native_leak/` 的 monitoredPidList_ + `leak_config/native_leak_config.json` | 名字必须与 `/proc/pid/comm` 一致 |
| 启用/关闭某类泄漏检测 | `setparam hiview.fdleakcheck / hiview.threadleakcheck / hiview.memleakcheck disable` | 各 detector 通过 `IsXxxLeakEnable` 读取 |
| 启用测试模式 | 系统参数 `persist.hiview.leak_detector`（develop mode） | `IsBetaOrDevelopMode` 读取 |
| 修改 Commercial/Beta 配置 | `config/fault_config_manager.cpp` 的 `ParseConfigFile` | 先 Commercial base，Beta/Develop 模式再 Beta override |
| 新增 Telemetry 配额类型 | `telemetry_manager.h`（`TelemetryType` enum）+ `EventQuota` struct | 配额管理受条件编译控制 |
| 修改状态机流程 | `base/fault_common.h`（`FaultStateType` 17 状态）+ 各 detector 的 state_context | 状态机是核心编排逻辑 |
| 修改 RSS 泄漏检测 | `rss_leak/` 的 `RssLeakDispatcher` / `RssLeakTracker` / `RssLeakCommon` + 3 个 telemetry handler | 仅非 Beta 版本启用 |
| 修改事件处理 | `fault_detector_manager.cpp` 的 `OnEvent` + `event_handler/` | 系统事件入口 |
| 修改进程 kill 上报 | `fault_detector_manager.cpp` 的 `HandleProcessKillEvent` / `HandleProcessKill` | kill 事件独立处理 |
| 修改 GPU 内存上报 | `fault_detector_manager.cpp` 的 `GpuMemoryOverReport` | GPU 内存独立处理 |
| 修改 JSVM/ArkWeb JS engine OOM 上报 | `fault_detector_manager.cpp` 的 `HandleJsvmOomEvent` / `HandleV8OomEvent` + `notify_leak/` 的 resourceType 分发 | OOM 事件经 NotifyLeakDetector 走 OOMDump 流程 |

---

## 4. 高风险 / 高频变更路径

| 路径 | 风险 | 证据 |
|------|------|------|
| `base/fault_common.h` 的 `FaultStateType` enum | 17 状态串联，修改顺序易导致状态机错乱 | PROC_IDLE_STATE...PROC_FINISHED_STATE |
| `base/fault_detector_base.h` | 抽象基类，修改影响全部 6 个 detector | `FaultDetectorBase` 被所有 detector 继承 |
| `fault_detector_manager.cpp` 的 `MonitorProcess` | switch-case 分发到各 detector，新增 detector 需同步修改 | 监控循环核心 |
| `fault_detector_manager.cpp` 的 `PrepareFaultDetectorEnv` | `detectorList_` 填充逻辑，RSS_LEAK 仅非 Beta 启用 | `if (!IsBetaVersion())` |
| `config/fault_config_manager.cpp` 的 `ParseConfigFile` | Commercial + Beta 合并逻辑，顺序敏感 | 先 Commercial 后 Beta override |
| `native_leak/native_leak_detector.cpp` | 核心编排器，状态机耦合 `cmdState_`/`runningDebug_` | 5 状态机（ENABLE/DISABLE/LONG_DELAY/SAVE/PROCESS） |
| `native_leak/native_leak_sample.cpp` | 用户态采样走 `/dev/bbox` ioctl，依赖内核 memcheck 驱动 | `LOGGER_MEMCHECK_RSS_READ` |
| `leak_config/leakdetectors_plugin_config` | 格式严格（`Name[thread:ClassName]:0 static`），错误导致 plugin 不加载 | 当前内容 `FaultDetectorManager[]:0 static` |
| `telemetry_manager.h` | `#ifdef DFR_LEAKDETECTOR_CONFIG_TELEMETRY` 条件编译，stub 分支全部返回 false | 关闭时所有方法为空实现 |
| `base/fault_common.h` 的 `MEMCHECK_MAGIC = 0x5377FEFA` | 与内核 memcheck 驱动约定，不匹配则 ioctl 全部失败 | magic 值硬编码 |
| `kernel_leak/kernel_leak_detector.cpp` 的 `OverLimitEvent` 状态转移 | 必须检查 WaitingKillList，否则在 kill 等待期间重复 dump 或中断 kill 流程 | DTS2026050926030 |
| `kernel_leak/kernel_leak_info.h` 的 `OverBaseProcesses.uid` | uid 默认 0 导致 hiappevent 校验失败、AppEvent（external 维测）无法发送 | DTS2026050926030 |
| `native_leak/native_leak_removal_state.cpp` 的 `HandleKillProcess` | kill 后必须调 `SetProcessDied` 回写结果，否则 `PreloadApplication` 恢复逻辑失效 | DTS2026050926030 |

---

## 5. 嵌套指引

- 每个 detector（fd_leak/thread_leak/kernel_leak/native_leak/notify_leak/rss_leak）内部遵循对称结构：detector + state_context + judge_state + dump_state + report_state + appreport_state + removal_state + info，详见 routing.md 功能线 FL-01~FL-06。
- `base/` 是框架抽象层，修改基类签名需同步所有 6 个 detector 子类。
- `config/` 与 `leak_config/` 是解析端与数据端的关系：`FaultConfigManager` 读取 `leak_config/*.json`。
- `telemetry_manager` 与 `telemetry_reporter` 是配额管理与上报执行的关系，受同一条件编译开关控制。

---

## 6. 子模块代码导航

### 6.1 base/（框架抽象层）

| 文件 | 职责 |
|------|------|
| `fault_common.h` | **核心定义文件**：`DetectorType` enum（NONE/FD/THREAD/NOTIFY/KERNEL/NATIVE/RSS_LEAK_DETECTOR）、`FaultStateType` enum（17 状态）、属性键常量（`hiview.*leakcheck`/`const.logsystem.versiontype`/`persist.hiview.leak_detector`）、`MEMCHECK_MAGIC`/`MEMCHECKIO`、事件名常量 |
| `fault_detector_base.{h,cpp}` | `FaultDetectorBase` 抽象基类：虚方法 `HandleDetector`/`OnChangeState`/`ExeNextStateProcess`/`GetStateObj`/`SetAppResourceLimit`；保护成员 `monitoredPidList_`/`blockList_`/`leakingPidsInfo_`/`appThresholds_` |
| `fault_state_base.{h,cpp}` | `FaultStateBase` 抽象基类：状态行为接口 |
| `fault_info_base.{h,cpp}` | `FaultInfoBase` 抽象基类：泄漏信息数据模型 |
| `fault_state_context_base.{h,cpp}` | `FaultStateContextBase`：状态上下文基类 |

### 6.2 config/（配置解析）

| 文件 | 职责 |
|------|------|
| `fault_config_manager.{h,cpp}` | `FaultConfigManager`：`ParseConfigFile` 先读 Commercial base，Beta/Develop 模式再 Beta override；`ParseConfigByVersion` 按版本节点解析 |
| `fault_config.{h,cpp}` | `FaultConfig`：配置参数模型 |

### 6.3 fd_leak/（FD 泄漏检测）

| 文件 | 职责 |
|------|------|
| `fd_leak_detector.{h,cpp}` | `FdLeakDetector`：继承 `FaultDetectorBase`，`HandleDetector` 入口 |
| `fd_leak_state_context.{h,cpp}` | 持有 5 状态对象（judge/dump/report/appreport/removal） |
| `fd_leak_judge_state.{h,cpp}` | 判定状态：扫描 `/proc/pid/fd`，超阈值入 `leakingPidsInfo_` |
| `fd_leak_dump_state.{h,cpp}` | dump 状态：fork 子进程读取 fd 详情，按类型分类计数 |
| `fd_leak_report_state.{h,cpp}` | 上报状态：构造 `EVENT_ID_FD_LEAK` 事件 |
| `fd_leak_appreport_state.{h,cpp}` | App 上报状态 |
| `fd_leak_removal_state.{h,cpp}` | 移除状态：清理已处理 PID |
| `fd_leak_info.{h,cpp}` | FD 泄漏信息数据模型 |

### 6.4 thread_leak/（线程泄漏检测）

| 文件 | 职责 |
|------|------|
| `thread_leak_detector.{h,cpp}` | `ThreadLeakDetector`：继承 `FaultDetectorBase` |
| `thread_leak_state_context.{h,cpp}` | 持有状态对象 |
| `thread_leak_judge_state.{h,cpp}` | 判定状态：扫描 `/proc/pid/task` 数量 |
| `thread_leak_dump_state.{h,cpp}` | dump 状态：落盘 `{pid}_thread_leak.txt` |
| `thread_leak_report_state.{h,cpp}` | 上报状态：构造 `EVENT_ID_THREAD_LEAK` 事件 |
| `thread_leak_appreport_state.{h,cpp}` | App 上报状态 |
| `thread_leak_removal_state.{h,cpp}` | 移除状态 |
| `thread_leak_info.{h,cpp}` | 线程泄漏信息数据模型 |

### 6.5 kernel_leak/（内核态内存泄漏检测）

| 文件 | 职责 |
|------|------|
| `kernel_leak_detector.{h,cpp}` | `KernelLeakDetector`：继承 `FaultDetectorBase`，监控 `/proc/meminfo`（slub/vmalloc/ashmem/ion/cma/gpu/kernel zone） |
| `kernel_leak_state_context.{h,cpp}` | 持有状态对象 |
| `kernel_leak_judge_state.{h,cpp}` | 判定状态：周期采样 meminfo，正则匹配 |
| `kernel_leak_dump_state.{h,cpp}` | dump 状态 |
| `kernel_leak_report_state.{h,cpp}` | 上报状态 |
| `kernel_leak_removal_state.{h,cpp}` | 移除状态 |
| `kernel_leak_info.{h,cpp}` | 内核内存泄漏信息数据模型 |

### 6.6 native_leak/（用户态内存泄漏检测）

| 文件 | 职责 |
|------|------|
| `native_leak_detector.{h,cpp}` | `NativeLeakDetector`：核心编排器，含 `ProcessUserEvent`/`ServiceLeakCheck`/`SysloadDetection`/`JudgeMemmgrProcessOverThreshold`/`SetSplitMemory` |
| `native_leak_state_context.{h,cpp}` | 持有 5 状态对象（judge/dump/report/appreport/removal） |
| `native_leak_judge_state.{h,cpp}` | 判定状态：最近 8 次 PSS 采样均超阈值 → 判定泄漏 |
| `native_leak_dump_state.{h,cpp}` | dump 状态：5 状态机（ENABLE/DISABLE/LONG_DELAY/SAVE/PROCESS） |
| `native_leak_report_state.{h,cpp}` | 上报状态：构造 `EVENT_ID_MEMORY_LEAK` 事件 |
| `native_leak_appreport_state.{h,cpp}` | App 上报状态 |
| `native_leak_removal_state.{h,cpp}` | 移除状态 |
| `native_leak_info.{h,cpp}` | 用户态内存泄漏信息数据模型 |
| `native_leak_xperf_detector.{h,cpp}` | 向 xperf 发送 native 泄漏消息 |

### 6.7 notify_leak/（Notify 泄漏检测）

| 文件 | 职责 |
|------|------|
| `notify_leak_detector.{h,cpp}` | `NotifyLeakDetector`：继承 `FaultDetectorBase`，仅在内存泄漏启用时加载；处理 JSVM/ArkWeb JS engine OOM rawheap 文件替换与等待 |
| `notify_leak_state_context.{h,cpp}` | 持有状态对象 |
| `notify_leak_judge_state.{h,cpp}` | 判定状态 |
| `notify_leak_dump_state.{h,cpp}` | dump 状态 |
| `notify_leak_report_state.{h,cpp}` | 上报状态 |
| `notify_leak_removal_state.{h,cpp}` | 移除状态 |
| `notify_leak_deal_fork_process.{h,cpp}` | fork 子进程处理 notify 泄漏 |
| `notify_leak_info.{h,cpp}` | Notify 泄漏信息数据模型 |

### 6.8 rss_leak/（RSS 内存泄漏检测）

| 文件 | 职责 |
|------|------|
| `rss_leak_dispatcher.{h,cpp}` | `RssLeakDispatcher`：继承 `FaultDetectorBase`，**仅非 Beta 版本启用**（`if (!IsBetaVersion())`），编排 tracker 与 telemetry handler |
| `rss_leak_tracker.{h,cpp}` | `RssLeakTracker`：RSS 维度跟踪与判定 |
| `rss_leak_common.{h,cpp}` | `RssLeakCommon`：RSS 泄漏公共逻辑 |
| `rss_app_telemetry_handler.{h,cpp}` | App 维度 RSS telemetry 上报 |
| `rss_random_telemetry_handler.{h,cpp}` | 随机 RSS telemetry 上报 |
| `rss_system_telemetry_handler.{h,cpp}` | 系统维度 RSS telemetry 上报 |

### 6.9 telemetry_manager / telemetry_reporter

| 文件 | 职责 |
|------|------|
| `telemetry_manager.{h,cpp}` | `TelemetryManager`：`#ifdef DFR_LEAKDETECTOR_CONFIG_TELEMETRY` 条件编译；管理 `EventQuota`/`GrayQuota`、配额文件读写、压缩配额、电源连接配额、Telemetry 类型（SYSTEM_TELEMETRY=1001/RANDOM_TELEMETRY=1002） |
| `telemetry_reporter.{h,cpp}` | `TelemetryReporter`：Telemetry 上报执行 |

### 6.10 leak_config/（配置文件）

| 文件 | 职责 |
|------|------|
| `leak_detector_config.json` | 主配置（Commercial + Beta 双套，含 sample_time/pressure 灰度 + memory_type 各类开关） |
| `kernel_leak_config.json` | 内核内存配置（Beta 节点：enable/upload_interval/sample_time/pressure/rs_watchpoint/rs_watchpoint_pc/kernel_type） |
| `native_leak_config.json` | 用户态内存配置（av_codec_config/threshold/resource_quota） |
| `leakdetectors_plugin_config` | hiview plugin 加载清单（唯一 plugin：`FaultDetectorManager[]:0 static`） |
