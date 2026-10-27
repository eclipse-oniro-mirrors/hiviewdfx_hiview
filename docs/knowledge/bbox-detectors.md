# BBox Detectors 黑盒检测知识

本文只记录内核/硬件黑盒故障（panic/watchdog/modem crash 等）事件接收、history.log 启动扫描、panic 恢复补报、动态库加载的边界。事件流水线见 `event-pipeline.md`，插件注册/生命周期见 `plugin-lifecycle.md`，故障日志管理见 `faultlogger-management.md`，路径与权限见 `path-and-ipc-safety.md`。

## 主链路

黑盒故障处理应保持阶段清晰：

1. 内核/硬件发生 panic/watchdog 等故障，底层经 `/dev/bbox` 或 pstore 上报 `KERNEL_VENDOR` 域事件（PANIC/CUSTOM/HWWATCHDOG 等）。history.log 的来源分两条：
   - 启动扫描消费的 `/data/hisi_logs/history.log`、`/data/log/bbox/history.log` 由底层/前一次启动已写入，Hiview 在 `StartBootScan` 中只读不写。
   - userspace pstore 方案下（`BBOX_USERSPACE` 已定义），`HandleBBoxEvent` 收到 `CUSTOM` 事件时在 Hiview 用户态追加 history.log：`PanicErrorInfoHandle::RKTransData`（仅 `const.product.devicetype == "default"`）读取 `/sys/fs/pstore/blackbox-ramoops-0` → `SaveHistoryLog` 追加写 `HISTORY_LOG_PATH` → `CopyPstoreFileToHistoryLog` 保存剩余日志（`bbox_detectors_base.cpp:64-71`、`panic_error_info_handle.cpp:169-185`、`:236-254`）。`/dev/bbox` 等其他平台的生产端行为需附对应实现依据，不要将其概括为这两条路径共同的固定前序。
2. `SysEventSource` 接收事件，按 dispatch_rule 路由到 `BBoxDetectorPipeline`。
3. `BBoxDetectorPlugin::OnEvent` 过滤 `domain_ == "KERNEL_VENDOR"`，经 `GetBBoxDetectorsInterface` 获取实现，调用 `HandleBBoxEvent`。
4. `BBoxDetectorsBase::HandleBBoxEvent` 去重 → `WaitForLogs` 等待 DONE → `SmartParser::Analysis` 解析栈帧 → 补全 `FIRST_FRAME`/`SECOND_FRAME`/`LAST_FRAME`/`FINGERPRINT` → 写回 SysEvent。
5. 启动时 `BBoxListener` 监听 `PLUGIN_LOADED`，延迟 60s 触发 `StartBootScan` 扫描 history.log 补报未上报事件。
6. panic 恢复场景由 `PanicReport` 命名空间补报 `PANIC` 事件（`REASON=HM_PANIC:HM_PANIC_SYSMGR`，`FIRST_FRAME=RECOVERY_PANIC`，事件名为 `PANIC`，`RECOVERY_PANIC` 只是 `FIRST_FRAME` 字段值，`panic_report_recovery.cpp:273`）。

策略决策（事件去重、栈帧解析、补报时机）不要下沉到插件 `OnEvent` 或 IPC Stub 中。改变采集编排时，先理解 `bdfr_plugin_config` 流水线配置和 `BBoxDetectorPipeline` dispatch_rule。

## 插件注册与流水线

- 注册方式：`REGISTER(BBoxDetectorPlugin)`（`bbox_detector_plugin.cpp:29`），静态注册，插件名等于类名。
- 所属动态库：`BBoxDetectorPlugin` 编译进 `libbdfr.z.so`（`plugins/plugin_build/BUILD.gn:65` target `bdfr`），与 `Faultlogger` 共库；`bdfr` 在 `core/bundle_config/config/plugin_bundle.json` 的 Beta/Commercial 包列表中。
- 插件配置（`plugins/plugin_build/bdfr_plugin_config`）：
  - `BBoxDetectorPlugin[thread:bbox_detector]:0 static` —— 运行在名为 `bbox_detector` 的独立线程，静态加载。
  - `BBoxDetectorPipeline:EventValidator PrivacyController BBoxDetectorPlugin SysEventDispatcher SysEventStore`。
  - `SysEventSource:faultloggerPipeline BBoxDetectorPipeline` —— `SysEventSource` 同时驱动两条流水线。
- dispatch_rule（`config/BBoxDetectorPipeline`，安装到 `/system/etc/hiview/dispatch_rule/`）：声明 `KERNEL_VENDOR` 域下 PANIC/CUSTOM/HWWATCHDOG/MODEMCRASH/DPACRASH/BOOTFAIL 等 28 个事件名（`events` 与 `KERNEL_VENDOR.include` 列表完全一致），**不包含 HUNGTASK**。`core/hiview_platform.cpp:58` 从 `dispatch_rule/` 目录加载。域过滤不等同于该域下任意事件都会路由到此流水线。
- `OnEvent` 只处理 `domain_ == "KERNEL_VENDOR"`（`bbox_detector_plugin.cpp:67`），其他域直接返回 false。
- `BBoxListener`（`bbox_detector_plugin.cpp:32`）继承 `EventListener`，订阅 `PLUGIN_MAINTENANCE` + `PLUGIN_LOADED` 无序事件触发启动扫描；订阅两套接口不能混用，见 `plugin-lifecycle.md`。

## 动态库与延迟释放

- 接口：`BBoxDetectorsInterface`（`bdfr_base/include/bbox_detectors_interface.h:26`）纯虚基类，导出 C 工厂符号 `NewBBoxDetectorsInterface`（`bbox_detectors_interface.h:42`）。
- 实现类 `BBoxDetectorsBase`（`bdfr_base/bbox_detectors_base.h:25`）编译进 `libbdfr_base.z.so`（由 `plugins/faultlogger/service/bdfr_base/BUILD.gn:73` 的 `bdfr_base` target 产出，依赖 `bbox_detectors_base_src`）。
- 获取入口：`GetBBoxDetectorsInterface(seconds)`（`export_bbox_detectors_interface.cpp:24`）经 `DynamicLibraryManager::GetDynamicLibrary("libbdfr_base.z.so")` 加载并查符号。
- 延迟释放：`BBOX_LIB_DELAY_RELEASE_TIME = 5 * 60`（5 分钟，`export_bbox_detectors_interface.h:24`）。各调用点应按实际列出：
  - 传 `BBOX_LIB_DELAY_RELEASE_TIME`（延长库驻留 5min）：`OnEvent`（`bbox_detector_plugin.cpp:74`）、`InitPanicReporter`（`:139`）、`NotifyBootStable`（`:111`）、`AddBootScanEvent` 定时回调（`:157`，启动扫描）。
  - 使用默认值 0（不延长库驻留）：`AddDetectBootCompletedTask` 的开机完成轮询回调（`:87`，1s 轮询）、`ConfirmReportResult` 的 10s 回调（`:118`）。
  - 不能用"定时扫描等短时调用传 0"推导错误的驻留时长。
- deleter 捕获 `handle` 保证其生命周期与智能指针一致（`export_bbox_detectors_interface.cpp:39`）。

## 事件处理与去重

`BBoxDetectorsBase::HandleBBoxEvent`（`bbox_detectors_base.cpp:60`）双路去重：

1. `HisysEventUtil::IsEventProcessed(name, "LOG_PATH", dynamicPaths)` —— 查事件库是否已入库（`hisysevent_util.cpp:23`，`SysEventDao::BuildQuery("KERNEL_VENDOR", {name})`）。
2. `eventRecorder->IsExistEvent(name, happenTime, dynamicPaths)` —— 内存级去重（`bbox_event_recorder.cpp:27`），键为 `name + happentime`，进程重启后失效。

- `BboxEventRecorder`（`bbox_event_recorder.h:25`）持 `std::map` + `mutex`，仅做存在性判断，不持久化。
- `PanicReport::IsRecoveryPanicEvent` 仅判断 `LOG_PATH` 是否以 `PANIC_LOG_PATH` 开头（`panic_report_recovery.cpp:165-169`）；命中后 `HandleBBoxEvent` 直接 return，**不调 `OnFinish`**，`OnEvent` 随后仍返回 true（`bbox_detectors_base.cpp:64-66`）。因此该分支是跳过常规 BBox 解析、保留补报字段，并非丢弃重复补报。
- `CUSTOM` 事件先经 `PanicErrorInfoHandle::RKTransData` 把 pstore 数据转写 history.log（`bbox_detectors_base.cpp:68`）。
- `module=="AP" && event=="BFM_S_NATIVE_DATA_FAIL"` 调 `OnFinish()` 标记事件完成。
- `WaitForLogs` 等 `logDir/DONE` 文件最多 60s（`bbox_detectors_base.cpp:222`），超时仅告警不阻断。
- 栈帧解析：`SmartParser::Analysis(dynamicPaths, "/system/etc/hiview", name)`（`bbox_detectors_base.cpp:98`），配置目录同 `extract_rule.json` 中 `/data/log/bbox/` 规则。
- `CheckAndHiSysEventWrite`（`bbox_detectors_base.cpp:195`）只在 `OH_HiSysEvent_Write` 返回负值且 `name` 不含 `UNKNOWNS` 时，显式改名为 `UNKNOWNS` 递归重试一次（`:209-214`）。该回退只发生在 `CheckAndHiSysEventWrite` 内部，触发条件是写入失败而非事件名"未声明"，也不适用于任意 `OH_HiSysEvent_Write` 调用。递归重试结果未向外返回，外层仍返回首次 `res`，不能用该返回值判断回退是否成功。

## 启动扫描与历史日志

`StartBootScan`（`bbox_detectors_base.cpp:116`）：

- 历史日志两源：`/data/hisi_logs/history.log`（hisy 平台，键名带空格如 `category [`）与 `/data/log/bbox/history.log`（通用，键名 `category[`），见 `bbox_detectors_base.cpp:44-47`、`GetValueFromHistory`（`:163`）。
- 用 `std::ifstream::ate` + `FileUtil::GetLastLine` 从尾部倒读，每个文件最多 `readLineNum=5` 行。
- 跳过 24h 前事件（`oneDaySecond`，`:118`/`:141`），并执行与 `HandleBBoxEvent` 相同的双路去重。
- `DPACRASH`/`MODEMCRASH`/`MODEM_REBOOTSYS` 额外按 `SUB_LOG_PATH` 去重（`:148`）。
- 扫描结束 `eventRecorder.reset()`（`:160`），释放内存表。

## Panic 恢复补报

`PanicReport` 命名空间（`bdfr_base/panic_report_recovery.cpp`）：

- 短启动判定：`isLastStartUpShort` 来自配置文件；或 `last_bootup_keypoint < 250`（`/proc/cmdline`，`:157`/`:161`）。
- panic 日志压缩：`CompressAndCopyLogFiles`（`:215`）打包 `last_fastboot_log`/`hm_klog.txt`/`hm_snapshot.txt` 到 `PANIC_LOG_PATH/panic_log_<time>.zip`，`CheckTimeStr` 校验时间串只含数字与 `-`。
- 补报：`TryToReportRecoveryPanicEvent`（`:277`）在 `isPanicUploaded==false` 且恢复时间变化时，**还需对应备份 zip `PANIC_LOG_PATH/panic_log_<time>.zip` 存在**（`:282`）才调用 `ReportPanicEventAfterRecovery` 写 `PANIC` 事件（`REASON=HM_PANIC:HM_PANIC_SYSMGR`，`FIRST_FRAME=RECOVERY_PANIC`，`:263`）。`TryToReportRecoveryPanicEvent` 返回 true 只表示调用了上报函数；`ReportPanicEventAfterRecovery`（`:252`）未检查 `OH_HiSysEvent_Write` 返回值，不能据此称为"补报成功"。
- 确认：真正入库确认在 10s 后的 `ConfirmReportResult`（`:292`）查事件库（`HisysEventUtil::IsEventProcessed("PANIC", ...)`）确认入库后置 `isPanicUploaded=true`。
- 触发链：`NotifyBootCompleted`（开机完成移除检测任务）→ 10min 后 `NotifyBootStable`（`bbox_detector_plugin.cpp:131`）→ 若调用了上报函数再 10s 后 `ConfirmReportResult`（`:117`/`:131`）。

## 条件编译

- 特性宏 `hiview_feature_bbox_userspace`（`hiview.gni:102`，默认 false）控制 `panic_error_info_handle.cpp` 编译并注入 `-DBBOX_USERSPACE`（`bdfr_base/BUILD.gn:50`）。
- `BBOX_USERSPACE` 未定义时 `RKTransData` 为空实现（`panic_error_info_handle.h:30`），pstore→history.log 转写不生效。开启前确认目标设备为 userspace bbox 方案。
- 单元测试编译期注入 `-DUNITTEST`（`test/BUILD.gn:70`/`:96`、`bdfr_base/test/BUILD.gn:64`），`panic_report_recovery.cpp:36-42` 仅在 `#ifdef UNITTEST` 块内重定向 `BBOX_PARAM_PATH`/`PANIC_LOG_PATH` 到 `/data/test/bbox/`；`FACTORY_RECOVERY_TIME_PATH`、`CMD_LINE` 在该块外，**未被重定向**。
- UNITTEST 只对真正编译 `panic_report_recovery.cpp` 的 `BBoxDetectorBaseUnitTest`（`bdfr_base/test/BUILD.gn:34-39`、`:41`）生效；`BBoxDetectorUnitTest`/`BBoxDetectorModuleTest`（`test/BUILD.gn:34-42`）不编入 `panic_report_recovery.cpp`，且通过 `export_bbox_detectors_interface.cpp` 加载的是真实 `libbdfr_base.z.so`（无 `UNITTEST`），不要据此认定插件/模块测试全部在 `/data/test/bbox/` 内操作。
- 测试用 `-Dprivate=public`/`-Dprotected=public` 暴露私有成员。

## 路径与文件约束

| 常量 | 路径 | 用途 |
| --- | --- | --- |
| `HISIPATH` | `/data/hisi_logs/` | hisi 平台 bbox 日志根（`bbox_detectors_base.cpp:41`） |
| `BBOXPATH` | `/data/log/bbox/` | 通用 bbox 日志根（`:42`） |
| `HISTORY_LOG_PATH` | `/data/log/bbox/history.log` | 通用历史日志（`panic_error_info_handle.cpp:38`） |
| `SYS_FS_PSTORE_PATH` | `/sys/fs/pstore/blackbox-ramoops-0` | pstore panic 信息（`:39`） |
| `BBOX_PARAM_PATH` | `/log/reliability/bbox/bbox.save.log.flags` | panic 补报状态文件（`panic_report_recovery.cpp:40`） |
| `PANIC_LOG_PATH` | `/log/reliability/bbox/panic_log/` | panic 压缩包目录（`:41`） |
| `FACTORY_RECOVERY_TIME_PATH` | `/log/reliability/bbox/factory.recovery.time` | 恢复时间（`:43`） |
| `CMD_LINE` | `/proc/cmdline` | 读取 `last_bootup_keypoint`（`:44`） |
| `LOGPARSECONFIG` | `/system/etc/hiview` | SmartParser 配置目录（`bbox_detectors_base.cpp:43`） |

- `dynamicPaths` 由 `LOG_PATH` 与 `SUB_LOG_PATH` 拼接，末尾 `/` 归一化（`bbox_detectors_base.cpp:79`）。
- `SaveHistoryLog` 以追加方式写 history.log（`panic_error_info_handle.cpp:170`），`TryCreateDir` 创建 `/data/log/bbox/<时间戳>/` 目录（`:194`，权限 0770）。

## 配额与超时

| 常量 | 值 | 含义 |
| --- | --- | --- |
| `ZIP_FILE_SIZE_LIMIT` | 5MB | panic_log 目录上限（`panic_report_recovery.cpp:62`） |
| `COMPRESSION_RATION` | 9 | 压缩比/源文件超 `9 * 5MB` 时丢弃 `hm_snapshot`（`:61`/`:233`） |
| `oneDaySecond` | 86400 | 启动扫描跳过 24h 前事件（`bbox_detectors_base.cpp:118`） |
| `readLineNum` | 5 | 每个 history.log 倒读行数（`:119`） |
| `WaitForDoneFile` 超时 | 60s | 等待 DONE 文件（`:222`） |
| 启动扫描延迟 | 60s | `AddBootScanEvent` 定时（`bbox_detector_plugin.cpp:163`） |
| BootStable 延迟 | 10min | 开机完成后延迟补报（`:131`） |
| ConfirmReport 延迟 | 10s | 补报后确认入库（`:117`） |
| `BBOX_LIB_DELAY_RELEASE_TIME` | 5min | 动态库延迟释放 |

- `InitPanicReport` 启动时若 `PANIC_LOG_PATH` 体积超 5MB 先清空（`panic_report_recovery.cpp:121`）。

## 设备操作风险

- `SYS_FS_PSTORE_PATH`（`/sys/fs/pstore/blackbox-ramoops-0`）读取 panic 二进制结构体 `ErrorInfo`（`panic_error_info_handle.cpp:247`），`fin.read` 按固定长度读取，需确保结构体布局与内核一致。
- `RKTransData` 仅对 `const.product.devicetype == "default"` 生效（`:238`），其他设备类型直接返回。
- `/proc/cmdline` 读取 `last_bootup_keypoint` 后用 `atoi` 转 int（`:162`），需防非常规值。
- `OH_HiSysEvent_Write` 传入 `KERNEL_VENDOR` 域，事件名与字段需与系统侧 `hisysevent.yaml` 校验一致；启动扫描路径上 `CheckAndHiSysEventWrite` 在写入返回负值且事件名不含 `UNKNOWNS` 时会改名 `UNKNOWNS` 重试（详见"事件处理与去重"小节），递归结果不向外返回。
- `history.log`/`bbox.save.log.flags` 为可变状态文件，写入需保证原子性，进程异常退出可能留下不一致状态。当前 `LoadBboxSaveFlagFromFile` 读取失败时返回默认结构（`isPanicUploaded=true`），`isPanicUploaded` 字段缺失时空字符串 `!= "false"` 也会得到 true（`panic_report_recovery.cpp:88-102`、`panic_report_recovery.h:33`），因此状态文件为空但旧 panic zip 仍存在时 `!isPanicUploaded` 不成立，不会触发补报。`InitPanicConfigFile` 只更新启动标志、`TryToReportRecoveryPanicEvent` 清除短启动标志、`ConfirmReportResult` 仅在查到 PANIC 后设置已上传，这些逻辑没有从备份日志重建丢失状态，也不修复 history.log。异常退出后的状态一致性及待补报状态恢复仍需专项验证，不能仅靠现有重置逻辑判断收敛。

## ABI 与符号约束

- `libbdfr_base.z.so` 的导出符号由 `plugins/faultlogger/service/bdfr_base/libdfr_base.map` 控制，`extern "C"` 导出 `NewFaultloggerInterface*` 与 `NewBBoxDetectorsInterface*`（`:3-6`）。
- **禁止**修改/删除/重排已导出符号，`NewBBoxDetectorsInterface` 是 `bbox_detectors` 与 `bdfr_base` 间的稳定契约，删改将导致 `GetSymbol` 返回 nullptr。
- `plugins/plugin_build/libdfr.map` 另导出 `BboxEventRecorder::AddEventToMaps`/`IsExistEvent` 的 C++ mangled 符号（同时含 `unsigned long` 与 `unsigned long long` 两份），修改 `BboxEventRecorder` 公共签名会破坏 `libbdfr.z.so` ABI。
- `plugins/reliability/bbox_detectors/bdfr_base/libdfr_base.map` 为**未使用的遗留文件**（引用 `NewFaultloggerInterface`，且无 target 引用它），不要据此判断导出关系，修改时以 `plugins/faultlogger/service/bdfr_base/libdfr_base.map` 为准。

## 测试指引

- 插件层：使用 `BBoxDetectorUnitTest`（`test/BUILD.gn:44`，unittest），现有 3 个用例覆盖 `CanProcessEvent`（`bbox_detector_unit_test.cpp:73`）、定时/通知接口调用（`:97` `AddBootScanEvent`/`AddDetectBootCompletedTask`/`NotifyBootStable`/`NotifyBootCompleted`）和 `BboxEventRecorder`（`:115`），**未直接调用 `OnLoad`/`OnEvent`**。
- 业务核心：使用 `BBoxDetectorBaseUnitTest`（`bdfr_base/test/BUILD.gn:41`），覆盖 `HandleBBoxEvent`/`StartBootScan`/`PanicReport`，是启动扫描与 panic 补报的核心测试入口。
- 模块测试：使用 `BBoxDetectorModuleTest`（`test/BUILD.gn:85`，moduletest），实际 9 个用例（001–007、009、010，缺 008），覆盖 `OnLoad`+`OnEvent` 主链路（005 为 null 事件、006 域不匹配返回 false、010 验证 `BFM_S_NATIVE_DATA_FAIL` 后 `HasFinish`）。009 仅 `OnLoad` 后断言 `FIRST_FRAME` 等字段为空（`IsEventProcessed` mock 为 true），**未调用 `OnEvent`**；模块测试未直接调用 `StartBootScan` 或验证 history.log 扫描结果。
- mock：`test/mock/hisysevent_util_mock.cpp` 桩掉 `IsEventProcessed`，`bbox_detectors_mock.h` 桩 `HiviewContext`/`EventLoop`。
- 测试编译 `cflags_cc += ["-Dprivate=public", "-Dprotected=public"]` 暴露私有成员访问 `BBoxDetectorsBase` 内部方法。
- 测试目标聚合：`plugins/BUILD.gn:25`（unittest）/`:37`（moduletest）。
- 涉及真实 panic 采集、pstore 读取、history.log 启动扫描时，补充 `board-verification.md` 中的板侧证据，注意 pstore/`/proc/cmdline` 为设备相关路径。
