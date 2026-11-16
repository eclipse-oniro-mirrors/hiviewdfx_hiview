# leak_detectors — AGENTS.md 知识索引

> 资源泄漏检测子系统（FD / 线程 / 内核内存 / 用户态内存 / Notify / RSS），作为 hiview reliability 插件子模块，由唯一 Plugin `FaultDetectorManager` 编排 6 类 detector，按周期采样 /proc 与 ioctl 接口，状态机驱动判定泄漏后落盘日志并向 event pipeline 广播。
>
> 本文件是知识索引，详细内容在 `docs/knowledge/` 下四个独立文档中。
>
> 代码位置：`base/hiviewdfx/hiview/plugins/reliability/leak_detectors`

---

## 1. 项目概览

- **定位**：hiview reliability 的资源泄漏检测子模块，负责 OpenHarmony 设备运行期的六类资源泄漏检测（FD / 线程 / 内核内存 / 用户态内存 / Notify / RSS），生成可靠性日志并上报。
- **技术栈**：C++17、OpenHarmony hiview plugin 框架、jsoncpp、系统参数（`system::GetParameter`）、POSIX（fork/pipe/waitpid、ioctl、prctl）、/proc 文件系统、ffrt（异步任务调度）。
- **规模**：36,000+ 行代码；唯一 Plugin 入口 `FaultDetectorManager` + 6 类 detector（fd_leak/thread_leak/kernel_leak/native_leak/notify_leak/rss_leak）+ base/config/detector_utils/event_handler/lite_diag/old_log_clean/process_kill/resource_overload + telemetry_manager/telemetry_reporter + leak_config 配置目录。
- **构建产物**：编译进 hiview 插件，通过 `leakdetectors_plugin_config` 装载唯一 plugin `FaultDetectorManager`。

### 关键项目背景（非代码可见）

- **Commercial = 默认 base，Beta = 差异化 override（已实现）**：`FaultConfigManager::ParseConfigFile` 先读 Commercial base，Beta/Develop 模式再 Beta override。Commercial 节点改动**会生效**（`config/fault_config_manager.cpp`）。
- **RSS_LEAK 仅非 Beta 版本启用**：`PrepareFaultDetectorEnv` 中 `if (!IsBetaVersion())` 是硬性约束（`fault_detector_manager.cpp`）。
- **Telemetry 条件编译**：`#ifdef DFR_LEAKDETECTOR_CONFIG_TELEMETRY` 控制 TelemetryManager 实现，关闭时全部方法为 stub（`telemetry_manager.h`）。
- **状态机驱动**：所有 detector 遵循 `FaultStateType` 17 状态枚举，各 state_context 持有 5 核心状态对象（judge/dump/report/appreport/removal）。

---

## 2. 知识导航（四类知识索引）

| 知识类型 | 文档路径 | 核心内容 |
|---------|---------|---------|
| **Code Map** | [`docs/knowledge/code_map.md`](knowledge/code_map.md) | 6 detector 模块职责边界、目录结构、16 项任务到路径映射、7 个高风险路径、10 个子模块代码导航 |
| **Routing** | [`docs/knowledge/routing.md`](knowledge/routing.md) | 三维路由（任务/路径/词汇）+ 6 条功能线调用链（FL-01~FL-06）+ 状态机通用流程 |
| **Expert** | [`docs/knowledge/expert.md`](knowledge/expert.md) | 10 条禁止事项、8 条架构不变量、11 条失败模式、11 条修复策略 |
| **Verify** | [`docs/knowledge/verify.md`](knowledge/verify.md) | 编译命令、UT/MT 测试命令、Done 定义、验证循环、7 类观测点、排障速查表 |

---

## 3. 快速导航

### 3.1 按任务类型导航

| 任务类型 | 对应文档章节 |
|-----------|-------------------|
| 理解项目整体结构 | Code Map §1 顶层模块职责 + §2 目录结构 |
| 定位某功能代码位置 | Code Map §3 任务到路径映射 |
| 修改某配置文件 | Routing §2 路径触发路由 |
| 理解某个术语 | Routing §3 词汇触发路由 |
| 理解某功能调用链 | Routing §4 功能线分析（FL-01~FL-06） |
| 理解状态机流程 | Routing §5 状态机通用流程 |
| 知道什么不能做 | Expert §1 禁止事项清单 |
| 知道架构约束 | Expert §2 架构不变量 |
| 排查失败 | Expert §3 失败模式 + §4 修复策略 |
| 编译验证 | Verify §1 最小编译命令 |
| 跑测试 | Verify §2 最小测试命令 |
| 确认任务完成 | Verify §3 Done 定义 |
| 设备验证 | Verify §4 验证循环 + §5 观测点 |
| 排障命令 | Verify §6 排障命令速查 |

### 3.2 按功能线导航

| 功能线 | 文档位置 | 说明 |
|--------|---------|------|
| FL-01 FD 泄漏检测 | Routing §4.1 | `PrepareFaultDetectorEnv → MonitorProcess → FdLeakDetector::HandleDetector → judge → dump(fork) → report → appreport → removal` |
| FL-02 线程泄漏检测 | Routing §4.2 | `PrepareFaultDetectorEnv → MonitorProcess → ThreadLeakDetector::HandleDetector → judge → dump → report → appreport → removal` |
| FL-03 内核态内存泄漏 | Routing §4.3 | `PrepareFaultDetectorEnv → MonitorProcess → KernelLeakDetector::HandleDetector → judge → dump → report → removal` |
| FL-04 用户态内存泄漏 | Routing §4.4 | `PrepareFaultDetectorEnv → MonitorProcess → NativeLeakDetector::HandleDetector → judge → dump(5状态机) → report → appreport → removal` |
| FL-05 Notify 泄漏检测 | Routing §4.5 | `PrepareFaultDetectorEnv（仅 memleak 启用时）→ MonitorProcess → NotifyLeakDetector::HandleDetector → judge → dump(fork) → report → removal` |
| FL-06 RSS 内存泄漏 | Routing §4.6 | `PrepareFaultDetectorEnv（仅非 Beta）→ MonitorProcess → RssLeakDispatcher::HandleDetector → judge → dump → report → removal` |

---

## 4. 知识质量自评

| 维度 | 满分 | 得分 | 详细评分 |
|------|------|------|---------|
| Code Map | 20 | 18 | 6 detector 模块边界清晰，16 项任务映射，7 个高风险路径标识完整；子模块代码导航覆盖 10 个子目录 |
| Knowledge Routing | 30 | 27 | 6 条功能线完整（FL-01~FL-06），状态机通用流程补充；词汇路由覆盖核心术语；任务/路径触发路由齐全 |
| Constraints and Boundaries | 30 | 26 | 10 条禁止事项覆盖关键约束（MEMCHECK_MAGIC/plugin_config格式/状态机顺序/Beta+RSS等）；8 条架构不变量；11 条失败模式 + 11 条修复策略 |
| Verification Loop | 20 | 17 | 编译命令（含 GN 参数）、UT/MT 测试命令齐全；7 类观测点覆盖加载/采样/落盘/上报/配置/Telemetry；Done 定义完整。扣分：缺少真实编译输出验证 |
| **总计** | **100** | **88** | **A (Pass)** |

### 关键缺陷检查

- [x] 不会修改公共 API 而无兼容性审查（plugin 接口由 hiview 框架定义，唯一入口 FaultDetectorManager）
- [x] 不会绕过权限/安全/协议边界（fork 子进程读 /proc 是明确设计）
- [x] 不会绕过生成代码的真实来源（无生成代码）
- [x] 能找到关键代码路径（6 条功能线完整）
- [x] 能验证变更（编译命令 + UT/MT + 观测点齐全）
- [x] 遇到领域术语知道加载什么知识（词汇路由表覆盖核心术语）

**结论**：A 级（88/100），知识可直接使用，作为 leak_detectors 子模块的团队基线。

---

## 5. 知识维护

- **修改代码时**：同步更新对应 `docs/knowledge/` 文档
- **新增功能线时**：在 Routing §4 新增 FL 条目，同步更新 Code Map
- **新增 detector 时**：同步修改 Code Map §1 + §6、Routing §4、Expert §1（禁止事项）+ §2（架构不变量）
- **发现新失败模式时**：在 Expert §3 新增条目
- **变更构建/测试方式时**：更新 Verify 文档
- **变更 GN 参数时**：同步更新 Verify §1 编译命令 + §6 排障速查
- **历史问题提取**：按 Expert §5 九类→四类映射规则，分散更新到四个文档
