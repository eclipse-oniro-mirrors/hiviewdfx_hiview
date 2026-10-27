# 板侧验证知识

本文记录 hiview 变更需要板侧证据时的稳定做法。

## 构建位置

OpenHarmony 构建命令从源码根目录执行，不在本子目录执行。

```sh
./build.sh --product-name rk3568 --build-target hiview
```

涉及故障日志、trace 采集、IPC 服务、冻屏检测、事件源或公开行为时，构建 hiview 或相关测试目标。

## 何时需要板侧证据

| 变更类型 | 最低证据 |
| --- | --- |
| 事件源、流水线分发、事件订阅 | 构建并运行最近的平台/分发测试；涉及真实 socket 或 samgr 时增加板侧运行 |
| 故障日志写入/查询、GWP-ASan | 构建并运行 faultlogger 测试；涉及真实崩溃采集时增加板侧运行 |
| trace 采集、流控、状态机 | 构建并运行 trace 测试；涉及真实 trace 节点或低内存阈值时在板侧运行 |
| eventlogger catcher | 构建并运行 eventlogger 测试；涉及进程栈/binder/dmesg/sysrq 时在板侧运行 |
| IPC 服务、路径校验、权限 | 构建并运行 SA/服务测试；涉及真实 IPC 或设备节点时在板侧运行 |
| 冻屏检测规则 | 构建并运行 freeze_detector 测试；涉及真实冻屏场景时在板侧运行 |
| 统一采集器 | 构建并运行 unified_collection 测试；涉及真实 CPU/IO/Memory 采集时在板侧运行 |
| 公共 API 兼容性 | 构建并运行 API 测试；API 触达服务状态时增加板侧证据 |
| 仅配置解析 | 除非运行时行为变化，否则运行解析器或最近单元测试即可 |

## 场景化验证

单元测试无法覆盖真实崩溃采集、trace 节点、设备节点或冻屏场景时，使用服务集成场景：

- 故障日志场景通过触发真实进程崩溃（如 `kill -6 <pid>`）或 appfreeze 事件验证 cppcrash/appfreeze 处理。
- trace 采集通过 `hitrace` 命令或 `DumpTrace` 接口验证采集和流控。
- 设备节点（`/dev/bbox`、`/dev/ucollection`、`/proc/sysrq-trigger`）通过真实节点操作验证权限和采集。
- hiview 运行状态通过 `hidumper -s 1202`（faultlogger SA）或 `hiviewdfx.hiview.ready` 参数检查。

## 关键日志与输出位置

| 路径 | 用途 |
| --- | --- |
| `/data/log/hiview/` | hiview 工作目录（运行时日志、临时文件、`hiview.pid`） |
| `/data/log/faultlog/faultlogger/` | 故障日志主目录 |
| `/data/log/faultlog/temp/` | 故障日志临时目录 |
| `/data/log/faultlog/freeze/` | 冻屏日志 |
| `/log/hiview/` | 持久目录（跨重启保留） |
| `/system/etc/hiview/` | 配置目录（plugin_config、hiview_platform_config） |
