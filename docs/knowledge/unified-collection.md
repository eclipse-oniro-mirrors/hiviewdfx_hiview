# 统一采集知识

本文只记录统一采集器的接口分层、impl/empty_impl 二选一、decorator 装饰器的边界。trace 流控见 `trace-flow-control.md`，路径与权限见 `path-and-ipc-safety.md`。

## 接口分层

统一采集分两套接口，命名空间不同：

| 接口层 | 路径 | 命名空间 | 调用方式 |
| --- | --- | --- | --- |
| utility（进程内） | `interfaces/inner_api/unified_collection/utility/` | `UCollectUtil` | 直接调用 |
| client（跨进程 IPC） | `interfaces/inner_api/unified_collection/client/` | `UCollectClient` | SA 代理调用 |

两套接口的采集器范围不同：
- utility：CPU/Memory/IO/GPU/Trace/Thermal/Perf/Hilog/GraphicMemory（全部）
- client：仅 CPU 和 Trace（`cpu_collector_client.h`、`trace_collector_client.h`）

## impl/empty_impl 二选一

`framework/native/unified_collection/collector/` 下每个采集器有两套实现：

| 实现 | 路径 | 编译条件 |
| --- | --- | --- |
| 正式实现 | `collector/impl/<type>/` | 对应 `hiview_unified_collector_<type>_enable = true` |
| 空实现 | `collector/empty_impl/` | 对应开关为 false |

`BUILD.gn` 中模式（`framework/native/unified_collection/BUILD.gn`）：
```gn
if (hiview_unified_collector_cpu_enable) {
  sources += [ "collector/impl/cpu/cpu_collector_impl.cpp" ]
  defines += [ "UNIFIED_COLLECTOR_CPU_ENABLE" ]
} else {
  sources += [ "collector/empty_impl/cpu_collector_empty_impl.cpp" ]
}
```

**新增采集器时必须同时提供 impl 和 empty_impl 两套实现**，否则特性关闭时编译失败。

## 采集器清单

| 采集器 | utility 接口 | 关键方法 |
| --- | --- | --- |
| CPU | `cpu_collector.h` | `CollectSysCpuLoad`/`CollectSysCpuUsage`/`GetSysCpuUsage`/`CollectCpuFrequency`/`CollectProcessCpuStatInfo(s)`/`CreateThreadCollector` |
| Memory | `memory_collector.h` | `CollectProcessMemory`/`CollectSysMemory`/`CollectAllProcessMemory`/`CollectProcessVss`/`CollectMemoryLimit` |
| IO | `io_collector.h` | `CollectProcessIo`/`CollectDiskStats`/`CollectEMMCInfo`/`CollectAllProcIoStats`/`CollectSysIoStats` |
| GPU | `gpu_collector.h` | `CollectGpuFrequency`/`CollectSysGpuLoad` |
| Trace | `trace_collector.h` | `DumpTrace(caller)`/`DumpTraceWithDuration`/`DumpTraceWithFilter`/`DumpAppTrace`/`FilterTraceOn/Off` |
| Thermal | `thermal_collector.h` | 温度采集 |
| Perf | `perf_collector.h` | perf 采集 |
| Hilog | `hilog_collector.h` | hilog 采集 |
| GraphicMemory | `graphic_memory_collector.h` | 图形内存采集 |

## 修改前检查

- 新增采集器是否同时提供了 impl 和 empty_impl？
- 是否在 utility 和 client（如需跨进程）两套接口都声明？
- 是否提供了 decorator 装饰器？
- 特性开关是否在 `hiview.gni` 声明并在 `bundle.json` features 列出？
- `CollectResult` 的 `UcError` 是否需要新增错误码？
- 是否影响 `ProcessStatus` 的进程状态维护？

## 测试指引

- 客户端接口：使用 `UCollectionClientUnitTest`。
- utility 接口：使用 `UCollectionUtilityUnitTest`。
- Trace utility：使用 `UCollectionTraceUnitTest`，受 `hiview_unified_collector_trace_enable` 控制。
- 框架层：使用 `DecoratorUnitTest`、`CommonUtilTest`、`MemoryUtilsUnitTest`、`PerfCollectConfigUnitTest`（`framework/native/unified_collection/BUILD.gn`）。
- 统一采集器插件：使用 `UCStateObserverTest`（`plugins/unified_collector/test/unittest/common/BUILD.gn`）。
- 涉及真实设备 CPU/IO/Memory/trace 采集时，补充 `board-verification.md` 中的板侧证据。
