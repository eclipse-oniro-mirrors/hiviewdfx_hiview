# EventLogger Catcher 机制知识

本文只记录 eventlogger 的 catcher 编排、条件编译、采集大小/超时限制。故障日志管理见 `faultlogger-management.md`，事件流水线见 `event-pipeline.md`。

## 主链路

eventlogger 的采集流程应保持阶段清晰：

1. `EventLogger`（继承 `EventListener`+`Plugin`）监听冻结/崩溃等事件。
2. `StartLogCollect` 创建 `EventLogTask`。
3. `EventLogTask::AddLog(cmd)` 根据 cmd 字符串创建对应 `EventLogCatcher` 加入 `tasks_`。
4. `StartCompose()` 依次执行各 catcher 的 `Catch(fd, jsonFd)`。
5. 生成的日志文件发布事件供后续处理。

策略决策（catcher 选择、采集顺序、大小分配）不要下沉到单个 catcher 的 `Catch` 实现中。改变采集编排时，先理解 `event_logger_config` 配置和 `AddCapture` 的条件编译逻辑。

## Catcher 条件编译

各 catcher 受特性宏控制（`hiview.gni:105-114`，`plugins/eventlogger/log_catcher/BUILD.gn`）：

| Catcher | 宏 | 职责 |
| --- | --- | --- |
| `OpenStacktraceCatcher` | `STACKTRACE_CATCHER_ENABLE` | 进程栈采集（`ForkAndDumpStackTrace` fork 子进程） |
| `BinderCatcher`/`PeerBinderCatcher` | `BINDER_CATCHER_ENABLE` | binder 事务/对端调用链（`ParseBinderCallchain`） |
| `LightHilogCatcher` | `HILOG_CATCHER_ENABLE` | 轻量 hilog |
| `DmesgCatcher` | `DMESG_CATCHER_ENABLE` | 内核日志（`WRITE_TYPE`: DMESG/SYS_RQ/HUNG_TASK/SYSRQ_HUNGTASK） |
| `MemoryCatcher`/`CpuCoreInfoCatcher` | `USAGE_CATCHER_ENABLE` | 内存/CPU 信息 |
| `FfrtCatcher` | `OTHER_CATCHER_ENABLE` | FFRT 线程信息 |
| `ShellCatcher` | 多宏 | 通用 shell 命令（`CATCHER_TYPE`: CPU/WMS/AMS/PMS/DPMS/RS/MMI/DMS/SNAPSHOT/HILOG/SCBSESSION 等） |
| `HitraceCatcher` | `HITRACE_CATCHER_ENABLE` | trace 采集 |

特性宏在 `plugins/eventlogger/log_catcher/BUILD.gn` 中通过 `defines += ["XXX_CATCHER_ENABLE"]` 注入。新增 catcher 时必须同时在 `hiview.gni` 声明开关、在 `log_catcher/BUILD.gn` 加条件编译。

## 采集大小与超时限制

`EventLogCatcher` 基类（`log_catcher/include/event_log_catcher.h:22`）：
- `Initialize(strParam1, intParam1, intParam2)` → `Catch(fd, jsonFd)` 返回采集字节数。
- `GetLogSize`/`SetLogSize`：单个 catcher 的采集大小上限。
- `GetFdSize`：已写入的 fd 大小。

`EventLogTask`（`log_catcher/include/event_log_task.h:40`）：
- `Status` 枚举：RUNNABLE/RUNNING/SUCCESS/TIMEOUT/EXCEED_SIZE/SUB_TASK_FAIL/FAIL/DESTROY。
- 超时和超限会中断后续 catcher 执行。
- `event_logger_config`（`config/event_logger_config.h`）按事件名配置采集策略（大小、超时、catcher 列表）。

## AddCapture 条件编译

`EventLogTask::AddCapture()` 内含大量条件编译的 capture 方法，每个方法在对应宏启用时才编译进二进制。新增 catcher 时需在 `AddCapture` 中添加对应的条件分支。

## PeerBinderCatcher 特殊性

`PeerBinderCatcher`（`peer_binder_catcher.h:28`）是最复杂的 catcher：
- `BinderInfoParser` 解析 `/proc/transaction_proc`。
- `ParseBinderCallchain` 追踪 binder 调用链。
- `CatcherStacktrace` 采集对端进程栈。
- 支持 hiperf 辅助采集。
- 受 `BINDER_CATCHER_ENABLE` 和 `has_hiperf`（`hiview.gni:65`）双重控制。

## DmesgCatcher 设备操作风险

`DmesgCatcher`（`dmesg_catcher.h:28`）的 `WRITE_TYPE` 中：
- `SYS_RQ`：写 `/proc/sysrq-trigger`——**破坏性操作**，需权限检查（`/proc/sysrq-trigger` 需 `root:system 220`）。
- `HUNG_TASK`/`SYSRQ_HUNGTASK`：触发内核 hung task 检测。
- `DumpKernelStacktrace`：采集内核栈。

## 测试指引

- eventlogger 插件：使用 `EventloggerPluginTest`。
- EventLogTask 与 catcher：使用 `EventLoggerTest`。
- 事件字段校验：使用 `EventFieldValidatorTest`。
- event_logger_config 校验：使用 `EventLoggerConfigValidateTest`。
- 涉及真实进程栈采集、binder 调用链、dmesg/sysrq 时，补充 `board-verification.md` 中的板侧证据，注意 sysrq 为破坏性操作。
