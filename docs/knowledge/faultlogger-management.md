# 故障日志管理知识

本文只记录故障日志写入、查询、配额、GWP-ASan 的边界。事件流水线见 `event-pipeline.md`，路径与权限见 `path-and-ipc-safety.md`，eventlogger catcher 见 `eventlogger-catcher.md`。

## 主链路

故障日志处理应保持阶段清晰：

1. 系统服务/应用崩溃，faultloggerd 采集崩溃信息。
2. 通过 `faultlogger_client.h` 的 `AddFaultLog` 或 HiSysEvent 上报 `CPP_CRASH`/`APP_FREEZE` 等事件。
3. `Faultlogger` 插件 `OnEvent` 接收流水线事件或 `AddFaultLog` IPC 调用。
4. `FaultloggerBase` 按 eventName 分发到对应 `FaultLogEventPipeline`/`FaultLogEventIpc` 子类。
5. 子类按入口分两条：
   - **Pipeline 入口**（`FaultLogEventPipeline`，`faultlog_event_pipeline.cpp:27-34`）：`FillFaultLogInfo`→`FaultLogEventInterface::AddFaultLog`（内含 `UpdateCommonInfo`→`Analysis`→`UpdateFaultLogInfo`→`SaveFaultLogToFile`）→`UpdateSysEvent`→`ReportToAppEvent`。
   - **IPC 入口**（`FaultLogEventIpc`，`faultlog_event_ipc.cpp:19-29`）：先调公共 `FaultLogEventInterface::AddFaultLog`，失败返回；成功后再 `SaveFaultInfoToRawDb`→`ReportEventToAppEvent`→`DoFaultLogLimit`，**没有** `FillFaultLogInfo`/`UpdateSysEvent`。
   其中 `SaveFaultLogToFile`（`faultlog_event_interface.cpp:48`）经 `FaultLogManager` 调用底层 `WriteFaultLogToFile`（`faultlog_formatter.cpp:429`）。CPP_CRASH/APP_FREEZE 走 IPC 入口，按上述顺序定位调用链。
6. `FaultLogManager` 管理文件存储、配额清理、查询。

策略决策（故障类型路由、日志格式化、配额清理）不要下沉到 IPC Stub 或事件接收代码。

## 故障类型与处理类

`FaultLogType` 枚举（公共 `plugins/faultlogger/interfaces/cpp/innerkits/include/faultlogger_client.h:51`；内部 `faultlog_info_inner.h` 额外含 `ALL=0`/`ADDR_SANITIZER=10`/`MAX_TYPE`）：

| 类型 | 枚举值 | 处理类 | 路径 |
| --- | --- | --- | --- |
| C/C++ Crash | `CPP_CRASH=2` | `FaultLogCppCrash` | `service/bdfr_base/event/cpp_crash/faultlog_cppcrash.h:27` |
| JS Error | `JS_CRASH=3` | `FaultLogJsError` | `service/bdfr_base/event/js_cj_error/faultlog_jserror.h:22` |
| App Freeze | `APP_FREEZE=4` | `FaultLogFreeze` | `service/bdfr_base/event/freeze/faultlog_freeze.h:25` |
| Rust Panic | `RUST_PANIC=8` | `FaultLogRustPanic` | `service/bdfr_base/event/rust_panic/faultlog_rust_panic.h:22` |
| CJ Error | `CJ_ERROR=9` | `FaultLogCjError` | `service/bdfr_base/event/js_cj_error/faultlog_cjerror.h:23` |
| Addr Sanitizer | `ADDR_SANITIZER=10` | `FaultLogSanitizer` | `service/bdfr_base/event/sanitizer/faultlog_sanitizer.h:46` |

`MAX_TYPE`（`faultlog_info_inner.h:59`）作为哨兵值用于 fuzz 取模（如 `faultLogType % FaultLogType::MAX_TYPE`）。

## 写入路径约束

`plugins/faultlogger/service/bdfr_base/constants.h:30`：

| 常量 | 路径 | 用途 |
| --- | --- | --- |
| `FAULTLOG_FAULT_LOGGER_FOLDER` | `/data/log/faultlog/faultlogger/` | 故障日志主目录 |
| `FAULTLOG_WARNING_LOG_FOLDER` | `/data/log/warninglog/` | 警告日志 |
| `FAULTLOG_FREEZE_FOLDER` | `/data/log/faultlog/freeze/` | 冻屏日志 |
| `FAULTLOG_TEMP_FOLDER` | `/data/log/faultlog/temp/` | 临时文件 |
| `FAULTLOG_FAULT_HILOG_FOLDER` | `/data/log/faultlog/hilog/` | 故障相关 hilog |

`FaultLogManager` 根据 `faultLogType` 决定写入 warning 还是 faultlogger 目录（`faultlog_manager.cpp:158`）。

## 文件大小与数量配额

`plugins/faultlogger/service/bdfr_base/fault_file/faultlog_manager.cpp`：

| 常量 | 值 | 含义 |
| --- | --- | --- |
| `MAX_FAULT_LOG_PER_HAP` | 10 | 每个 HAP 最多保留 10 份同类故障日志 |
| `WARNING_LOG_MAX_SIZE` | 3MB | 警告日志目录上限 |
| `FAULT_LOG_MAX_SIZE` | 20MB | 故障日志目录上限 |
| `WARNING_LOG_MIN_KEEP_NUM` | 15 | 警告日志最少保留 15 个文件 |

- `RemoveOldFile`（`faultlog_manager.cpp:180`）调用 `ClearSameLogFilesIfNeeded` 按 `MAX_FAULT_LOG_PER_HAP` 清理。
- `faultlog_formatter.cpp:552`：temp 目录 cppcrash json 文件上限 50MB。
- `faultlog_cppcrash.cpp:535`：读取 cppcrash 日志内容上限 2MB。
- `plugins/freeze_detector/freeze_manager.cpp:34`：`FREEZE_EXT_MAX_FILE_NUM = 20`（freeze_ext 目录上限）。

## 查询权限边界

- `QuerySelfFaultLog`（`faultlogger_client.h`）：**只能查询自身故障日志**，按调用方 uid/pid 过滤。
- `faultlogger_service_ohos.cpp:170`：`faultType < FaultLogType::ALL || faultType > FaultLogType::APP_FREEZE` 时拒绝查询。
- IPC 查询通过 `IFaultLogQueryResult`（Stub/Proxy）返回结果集。

## GWP-ASan 相关

- **权限要求**：`EnableGwpAsanInner` 需 `ohos.permission.ENABLE_GWPASAN_INNER` 权限（`faultlogger_base.cpp:55`，`CheckCallerIsAllowed`）。
- **参数校验**：`sampleRate`/`maxSimutaneousAllocations`/`duration` 必须 > 0（`faultlogger_service_ohos.cpp:199`）；`EnableGwpAsanInner` 要求 `processName` 非空（`:223`）。
- **DAC**：`gwp_asan.*` 参数 DAC 为 `root:shell:775`（`hiview.para.dac:30`）。
- **安全风险**：GWP-ASan 开关可能影响被检测进程稳定性，误用可能导致进程崩溃。需安全评审。
- IPC 接口码 `ENABLE_GWP_ASAN_GRAYSALE`/`DISABLE`/`GET`/`ENABLE_GWP_ASAN_INNER`（`hiviewfaultlogger_ipc_interface_code.h:25`），**只能追加，不能修改已有值**，需 `@leonchan5` review。

## IPC 接口码保护

`plugins/faultlogger/service/idl/include/hiviewfaultlogger_ipc_interface_code.h`（`CODEOWNERS:16` 需 `@leonchan5` review）：

- `FaultLoggerServiceInterfaceCode`（SAID: 1202）：ADD_FAULTLOG、QUERY_SELF_FAULTLOG、ENABLE/DISABLE/GET_GWP_ASAN_GRAYSALE、DESTROY、ENABLE_GWP_ASAN_INNER。
- `FaultLogQueryResultInterfaceCode`：HASNEXT、GETNEXT。

**约束**：枚举值只能追加，不能修改/删除/重排已分配值，否则客户端与服务端枚举错位。

## 延迟释放

`export_faultlogger_interface.h:23`：`FAULTLOGGER_LIB_DELAY_RELEASE_TIME = 5 * 60`（5 分钟）—— faultlogger 库加载后延迟 5 分钟释放，避免频繁加载/卸载。

## 故障日志处理子类差异

| 类 | 基类 | 关键逻辑 |
| --- | --- | --- |
| `FaultLogCppCrash` | `FaultLogEventIpc` | `ParseCppCrashJson`/`ReadStackFromPipe`/`DealMiniDumpEvent`/`TruncateAppCrashLog`/`CheckHilogTime` |
| `FaultLogFreeze` | `FaultLogEventIpc` | `GetFreezeJsonCollector`/`ReportAppFreezeToAppEvent`/`MergeFreezeExtToLog`/`GetFreezeType` |
| `FaultLogRustPanic` | `FaultLogEventPipeline` | Rust panic 栈处理 |
| `FaultLogSanitizer` | `FaultLogEventPipeline` | `ParserArkTsStackInfo`/`ProcessArkTsLine`（调用 `dfx_ark`） |

`FaultLogEventPipeline`（流水线事件处理）和 `FaultLogEventIpc`（IPC 事件处理）是两条不同入口，前者经 `Faultlogger::OnEvent`，后者经 `IFaultLoggerService::AddFaultLog`。

## 测试指引

- 故障日志服务：使用 `FaultloggerUnittest`（`plugins/faultlogger/test/BUILD.gn:41`）。
- 故障日志客户端：使用 `FaultloggerClientUnittest`（`plugins/faultlogger/test/BUILD.gn:114`）。
- 故障日志业务核心：使用 `FaultloggerBaseUnittest`、`AsanUnittest`、`FreezeJsonGeneratorUnittest`（`service/bdfr_base/test/BUILD.gn`）。
- 原生接口：使用 `FaultloggerNativeInterfaceTest`（`interfaces/cpp/innerkits/test/BUILD.gn:28`，moduletest）。
- JS NAPI：使用 `plugins/faultlogger/interfaces/js/test/unittest:unittest`。
- Fuzz：约 46 个 fuzz target 覆盖 faultlogger 各子模块（`plugins/faultlogger/BUILD.gn:58`）。
- 测试资源：`service/bdfr_base/test/resource/faultlogger/` 有 `cppcrash-*`、`appfreeze-*`、`freeze-*` 样本日志。
- 涉及真实崩溃采集、GWP-ASan 或板侧故障日志时，补充 `board-verification.md` 中的板侧证据。
