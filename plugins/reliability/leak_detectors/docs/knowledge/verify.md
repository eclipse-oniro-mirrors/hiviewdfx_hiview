# Verify — leak_detectors 编译和测试方法

> 本文档描述 leak_detectors 的编译命令、测试命令、Done 定义、验证循环、关键观测点、排障命令。
> 代码位置：`base/hiviewdfx/hiview/plugins/reliability/leak_detectors`
>
> 注意：leak_detectors 正在从 vendor 目录下的 hiview_xpower_plugin 搬迁到 base/hiviewdfx/hiview 的 reliability 插件目录。base 侧的 BUILD.gn 构建接入尚未完成，编译/测试命令暂沿用 vendor 侧（hiview_xpower_plugin）的命令，待 base 侧构建接入后更新为 hiview 的编译命令。

---

## 1. 最小编译命令

leak_detectors 在 vendor 侧是 `hiview_xpower_plugin` 的一部分，编译产物为 `xpowerservice.so`（非独立 so）。编译入口在 `services/BUILD.gn`，build target 为 `xpowerservice`。

系统组件（手机）编译命令（vendor 侧，参考用）：

```bash
# 在 OpenHarmony 源码根目录
# 编译 xpowerservice（包含 leak_detectors）
./build_system.sh --abi-type generic_generic_arm_64only --device-type general_all_phone_standard --ccache --build-variant root --build-target xpowerservice

# 关键 GN 参数（通过 --gn-args 传递）
# hiview_xpower_plugin_dfr_leakdetector_enable=true   # 启用 leak detector 编译
# hiview_xpower_plugin_dfr_leakdetector_telemetry_enable=true  # 启用 Telemetry 编译（定义 DFR_LEAKDETECTOR_CONFIG_TELEMETRY）
# DUBAI_CONFIG_FEATURE_MEM=true   # 启用内存特性（UT 依赖）

# 启用 Telemetry 编译示例
./build_system.sh --abi-type generic_generic_arm_64only --device-type general_all_phone_standard --ccache --build-variant root --build-target xpowerservice --gn-args "hiview_xpower_plugin_dfr_leakdetector_telemetry_enable=true"
```

base 侧（hiview）编译命令（待 BUILD.gn 接入后启用）：

```sh
# 在 OpenHarmony 源码根目录执行（见仓库 AGENTS.md）
./build.sh --product-name rk3568 --build-target hiview
```

**注意**：
- `hiview_xpower_plugin_dfr_leakdetector_enable` 控制是否编译 leak detector 相关代码及外部依赖（selinux、hiprofiler）。
- `hiview_xpower_plugin_dfr_leakdetector_telemetry_enable` 控制 Telemetry 模块的编译（`telemetry_manager_test.cpp` / `telemetry_reporter_test.cpp`）。
- leak_detectors 源码通过 `services/BUILD.gn` 的 `xpower_source_set` template 统一编译进 `xpowerservice.so`。
- base 侧当前无 BUILD.gn，构建接入后需同步更新本节命令。

---

## 2. 最小测试命令

测试入口在 `test/BUILD.gn`，UT target 为 `XPowerUnitTest_Dfr`，MT 在 `test/moduletest/operators/handlers/dfr/leak_detectors/`。

```bash
# 单元测试（UT）—— target: XPowerUnitTest_Dfr
./test/xts/hats/run.sh -t XPowerUnitTest_Dfr
# 或直接在设备上执行
hdc shell /data/test/xpower_unit_test XPowerUnitTest_Dfr

# 模块测试（MT）—— fd_leak / thread_leak
hdc shell /data/test/xpower_module_test fd_leak_module_test
hdc shell /data/test/xpower_module_test thread_leak_module_test

# UT module_output_path: hiview_xpower_plugin/LeakDetectorDfx
```

**UT 覆盖范围**（`test/unittest/operators/handlers/dfr/leak_detectors/`）：
- `base/`：`fault_dump_state_base_test`
- `config/`：`fault_config_manager_test`
- `detector_utils/detail_info_parser/`：`detail_info_parser_test`
- `event_handler/`：cred/third_party_framework/process_kill/ashmem_kill/dma_kill/rss_kill/stability_check_kill/leak_event_handler_factory/oomdump
- `fd_leak/`：detector/judge_state/dump_state/dump_client/dump_on_demand_load/report_state/app_report_state/removal_state/state_context/info
- `kernel_leak/`：detector/judge_state/dump_state/sample_state/report_state/removal_state/state_context/info/appreport_state
- `native_leak/`：detector/judge_state/dump_state/report_state/removal_state/appreport_state/state_context/info/growth_analyser/xperf_detector
- `notify_leak/`：detector/state_context/judge_state/dump_state/report_state/app_report_state/removal_state/info/deal_fork_process/compress_common/compress_manager
- `rss_leak/`：detector
- `thread_leak/`：detector/judge_state/state/state_context/info
- `fault_detector_manager_test`
- `utils/`：dfr_zip_util/waiting_kill_list/dfr_util
- `resource_overload/sysload_manager`
- `old_log_clean/old_log_cleaner`
- Telemetry（条件编译）：`telemetry_manager_test` / `telemetry_reporter_test`（`hiview_xpower_plugin_dfr_leakdetector_telemetry_enable`）

**MT 覆盖范围**（`test/moduletest/operators/handlers/dfr/leak_detectors/`）：
- `fd_leak_module_test`
- `thread_leak_module_test`
- `leak_inject/resource_leak_inject`（注入工具）

---

## 3. Done 定义

任务完成的判定标准：

- [ ] **编译通过**：vendor 侧 `./build_system.sh ... --build-target xpowerservice` 或 base 侧 `./build.sh --product-name rk3568 --build-target hiview` 无错误
- [ ] **UT 全绿**：`XPowerUnitTest_Dfr` 全部用例通过
- [ ] **MT 全绿**：`fd_leak_module_test` / `thread_leak_module_test` 全部用例通过
- [ ] **Telemetry 条件编译验证**：`hiview_xpower_plugin_dfr_leakdetector_telemetry_enable=true/false` 均能编译通过
- [ ] **设备验证**：plugin 加载、采样、判定、上报链路完整（见 §5 观测点）
- [ ] **无回归**：原有功能未受影响

---

## 4. 验证循环

修改后验证步骤：

1. **本地编译**：`./build_system.sh --abi-type generic_generic_arm_64only --device-type general_all_phone_standard --ccache --build-variant root --build-target xpowerservice`
2. **运行 UT**：`./test/xts/hats/run.sh -t XPowerUnitTest_Dfr`
3. **运行 MT**：`hdc shell /data/test/xpower_module_test fd_leak_module_test`
4. **设备验证**：
   - 推送 so：`hdc file send xpowerservice.so /system/lib64/`
   - 重启 hiview：`hdc shell kill -9 <hiview_pid>` 或 `hdc shell reboot`
   - 检查 plugin 加载：`hdc shell hilog | grep FaultDetectorManager`
   - 检查采样：`hdc shell ls /data/log/reliability/resource_leak/`
   - 检查上报：`hdc shell hilog | grep EVENT_ID`
5. **Telemetry 验证**（如修改 Telemetry）：`hiview_xpower_plugin_dfr_leakdetector_telemetry_enable=true` 编译 + `telemetry_manager_test`

---

## 5. 关键观测点

### 5.1 Plugin 加载

```bash
# 检查 FaultDetectorManager 是否加载
hdc shell hilog | grep -i "FaultDetectorManager"
# 预期：OnLoad 日志、PrepareFaultDetectorEnv 日志

# 检查 ReadyToLoad
hdc shell hilog | grep -i "ReadyToLoad"
# 预期：三个开关都 disable 时返回 false，否则 true
```

### 5.2 Detector 注册

```bash
# 检查 detectorList_ 填充
hdc shell hilog | grep -i "PrepareFaultDetectorEnv"
# 预期：根据开关状态注册对应 detector

# 检查 RSS_LEAK 是否启用
hdc shell param get const.logsystem.versiontype
# beta → RSS_LEAK 不启用；非 beta → RSS_LEAK 启用
```

### 5.3 采样与判定

```bash
# 检查监控循环
hdc shell hilog | grep -i "MonitorProcess"
# 预期：周期性触发各 detector 的 HandleDetector

# 检查 FD 泄漏采样
hdc shell ls /proc/*/fd 2>/dev/null | head
# 检查线程泄漏采样
hdc shell ls /proc/*/task 2>/dev/null | head

# 检查内存泄漏采样（ioctl /dev/bbox）
hdc shell hilog | grep -i "MEMCHECK\|bbox"
# 预期：ioctl 返回 res=0；若 res!=0 则 memcheck 驱动未加载
```

### 5.4 落盘

```bash
# FD 泄漏落盘
hdc shell ls /data/log/reliability/resource_leak/fd_leak/
# 线程泄漏落盘
hdc shell ls /data/log/reliability/resource_leak/thread_leak/
# 内存泄漏落盘
hdc shell ls /data/log/reliability/resource_leak/memory_leak/
```

### 5.5 上报

```bash
# 检查事件上报
hdc shell hilog | grep -i "EVENT_ID_FD_LEAK\|EVENT_ID_THREAD_LEAK\|EVENT_ID_MEMORY_LEAK"
# 预期：检测到泄漏时构造对应事件并 PostUnorderedEvent
```

### 5.6 配置解析

```bash
# 检查配置文件
hdc shell cat /system/etc/hiview/leak_detector_config.json
hdc shell cat /system/etc/hiview/kernel_leak_config.json
hdc shell cat /system/etc/hiview/native_leak_config.json

# 检查 Commercial/Beta 解析
hdc shell param get const.logsystem.versiontype
# beta → Commercial base + Beta override
# 非 beta → 仅 Commercial base

# 检查 Develop 模式
hdc shell param get persist.hiview.leak_detector
# enable → Develop 模式（Beta override）
```

### 5.7 Telemetry

```bash
# 检查 Telemetry 配额文件
hdc shell ls /data/log/reliability/resource_leak/telemetry_quota
# 预期：Telemetry 启用时存在配额文件

# 检查 Telemetry 条件编译
# DFR_LEAKDETECTOR_CONFIG_TELEMETRY 定义时：TelemetryManager 方法有实现
# 未定义时：全部方法为 stub（返回 false/空）
```

### 5.8 WaitingKillList（DTS2026050926030）

```bash
# 检查 WaitingKillList 状态
hdc shell hilog | grep -i "WaitingKillList"
# 预期：进程被判定后台查杀后加入 WaitingKillList，延迟校验后清理

# 检查 OverLimitEvent 状态转移是否正确
hdc shell hilog | grep -i "OverLimitEvent"
# 预期：WaitingKillList 中的进程应进入 PROC_FINISHED_STATE，不应重复 dump
```

### 5.9 AppEvent（external 维测）uid 校验（DTS2026050926030）

```bash
# 检查 AppEvent 是否发送成功
hdc shell hilog | grep -i "AppEvent\|hiappevent"
# 预期：uid 非 0 时校验通过，事件成功发送；uid=0 则校验失败

# 检查 OverBaseProcesses 字段填充
hdc shell hilog | grep -i "OverBaseProcesses"
# 预期：uid 字段已通过 GetProcessUid(pid) 设置，非默认 0
```

---

## 6. 排障命令速查

| 问题 | 排障命令 | 预期结果 |
|------|---------|---------|
| Plugin 不加载 | `hdc shell hilog \| grep FaultDetectorManager` | OnLoad 日志存在；若无则检查 `leakdetectors_plugin_config` 格式 |
| Plugin 加载但 detector 不工作 | `hdc shell param get hiview.fdleakcheck` / `hiview.threadleakcheck` / `hiview.memleakcheck` | 对应开关值为 enable（非 disable） |
| 用户态采样失败 | `hdc shell hilog \| grep -i "MEMCHECK\|bbox"` | ioctl 返回 res=0；res!=0 则检查 memcheck 驱动 |
| RSS_LEAK 不启用 | `hdc shell param get const.logsystem.versiontype` | beta 版本下 RSS_LEAK 设计上不启用 |
| Commercial 改动不生效 | `hdc shell param get const.logsystem.versiontype` | 非 beta 仅读 Commercial；beta/develop 模式 Commercial+Beta 合并 |
| 配置文件未加载 | `hdc shell ls /system/etc/hiview/leak_detector_config.json` | 文件存在；若不存在检查 `leak_config:leak_config` 构建依赖 |
| 事件未上报 | `hdc shell hilog \| grep EVENT_ID` | 检查 report_state 是否触发；检查 `readyUpload_/needReport_` 标志 |
| Telemetry 不工作 | 检查 `DFR_LEAKDETECTOR_CONFIG_TELEMETRY` 宏 | 宏未定义时全部方法为 stub |
| UT 不编译 | 检查 `DUBAI_CONFIG_FEATURE_MEM` 和 `hiview_xpower_plugin_dfr_leakdetector_enable` | UT 依赖这两个 GN 参数为 true |
| Telemetry UT 不编译 | 检查 `hiview_xpower_plugin_dfr_leakdetector_telemetry_enable` | Telemetry UT 依赖此参数为 true |
| kill 回调 AppEvent 发送失败 | `hdc shell hilog \| grep -i "OverBaseProcesses"` | 检查 uid 字段是否非 0（uid=0 则 hiappevent 校验失败） |
| kill 后 PreloadApplication 不恢复 | `hdc shell hilog \| grep -i "SetProcessDied"` | 检查 HandleKillProcess 后是否调用 SetProcessDied |
| WaitingKillList 中进程被重复 dump | `hdc shell hilog \| grep -i "OverLimitEvent"` | 检查状态转移是否调用 IsInWaitingKillList |
| ArkWeb JS engine OOM 旧 rawheap 残留 | `hdc shell hilog \| grep -i "arkweb_jsheap replaced"` | 检查 TryReplaceArkwebRawheap 是否按时间戳替换 |
