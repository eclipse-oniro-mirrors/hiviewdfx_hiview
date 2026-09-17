# Hiview 维测组件指引

## 项目定位

本仓库对应 OpenHarmony `base/hiviewdfx/hiview`。优先按这些目录定位问题：

- `base/`：核心框架（插件、事件、事件循环、流水线、插件工厂、事件存储）。
- `core/`：插件管理平台（HiviewPlatform、插件配置、插件包、参数更新）。
- `plugins/`：独立业务插件（faultlogger、eventlogger、sysevent_source、unified_collector、performance、freeze_detector 等）。
- `adapter/`：服务适配层（IPC 服务、SysEvent 服务适配）。
- `framework/native/unified_collection/`：统一数据采集（CPU/GPU/IO/Memory/Trace 采集器与装饰器）。
- `interfaces/`：公共 API（inner_api、js/napi、ets/ani）。
- `utility/`：工具类（common_utils、smart_parser、analysis_faultlog）。
- `service/`、`core/platform_config/`、`service/config/`：运行配置。
- `test/`、`plugins/*/test/`、`*/test/`：单元测试、模块测试和 fuzz 测试。

### 按任务类型定位代码

| 任务类型 | 首选目录 |
| --- | --- |
| 新增静态/代理插件 | `base/include/` + 各插件目录 |
| 修改插件生命周期 | `core/` |
| 新增事件源 | `plugins/sysevent_source/` 或自建 |
| 修改流水线分发 | `core/` + `plugins/*/config/` |
| 新增故障类型处理 | `plugins/faultlogger/service/bdfr_base/event/` |
| 新增 catcher | `plugins/eventlogger/log_catcher/` |
| 修改 trace 采集/流控 | `framework/native/unified_collection/trace_manager/` |
| 新增统一采集器 | `interfaces/inner_api/unified_collection/utility/` + `framework/native/unified_collection/collector/` |
| 修改冻屏规则 | `plugins/freeze_detector/` |
| 修改 IPC 接口 | `plugins/faultlogger/service/idl/` 或 `adapter/service/` |
| 修改 NAPI/ANI 绑定 | `interfaces/js/napi/`, `interfaces/ets/ani/` |
| 修改路径/权限校验 | `adapter/service/server/src/` |
| 修改日志宏/通用定义 | `base/include/` |
| 修改构建配置/特性开关 | 根目录 + `build/` + `hiview.gni` |
| 修改事件定义 | `hisysevent/`, `hisysevent.yaml` |

### 嵌套指引

本仓库无目录级别的嵌套指引。所有任务级指导均通过 `docs/knowledge/` 中的场景文档提供。

## 构建和验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行，本仓不能独立编译。

```sh
# 编译验证
./build.sh --product-name rk3568 --build-target hiview

# 全量编译
./build.sh --product-name rk3568 --build-target base/hiviewdfx/hiview:hiview_package

# 编译单元测试
./build.sh --product-name rk3568 --build-target base/hiviewdfx/hiview:hiview_test_package
```

插件配置由 `build/gen_plugin_build.py` 从 `build/plugin_build.json` 生成 `plugin_config` 预置 etc，安装到 `/system/etc/hiview/`。

### 完成标准

任务被认为完成，当且仅当：

1. **本地构建通过** - 执行上述构建命令
2. **相关测试通过** - 对应 unittest/moduletest/fuzztest 通过
3. **板侧验证（如适用）** - 涉及故障日志、trace 采集、IPC 服务、冻屏检测的改动需提供验证证据
4. **文档更新（如适用）** - 公共 API 修改需更新注释和 `bundle.json` inner_kits；新增事件需更新 `hisysevent.yaml`

### 如果无法运行验证

明确说明无法运行的原因，列出推荐的验证步骤供人工执行，标记需要人工验证的部分。

### 完成报告格式

报告应包含：改动摘要（文件列表、改动点）、验证结果（构建/测试输出）、风险评估（API 兼容性、性能风险）、未完成事项。

## 知识索引

稳定背景知识放在 `docs/knowledge/`。改动前按场景读取对应文件：

### 场景与路径路由

| 场景 | 修改目录 | 先读文档 |
| --- | --- | --- |
| 事件流转、流水线分发、事件源、订阅、无序事件、dispatch rule 匹配 | `core/`, `plugins/sysevent_source/`, `base/` | `docs/knowledge/event-pipeline.md` |
| 插件注册、生命周期、代理加载/卸载、线程模型、插件包 bundle、命名约束 | `base/include/`, `core/`, 各插件 | `docs/knowledge/plugin-lifecycle.md` |
| 故障日志写入/查询、cppcrash/jserror/appfreeze/rustpanic、文件配额、GWP-ASan、FaultLogType | `plugins/faultlogger/` | `docs/knowledge/faultlog-management.md` |
| eventlogger catcher 编排、各 catcher 条件编译、采集大小/超时、event_logger_config | `plugins/eventlogger/` | `docs/knowledge/eventlogger-catcher.md` |
| trace 采集、状态机切换、配额、zip/io/app overflow、telemetry quota | `framework/native/unified_collection/trace_manager/`, `plugins/unified_collector/` | `docs/knowledge/trace-flow-control.md` |
| 统一采集器、utility/client 接口、impl/empty_impl 二选一、decorator | `interfaces/inner_api/unified_collection/`, `framework/native/unified_collection/` | `docs/knowledge/unified-collection.md` |
| 路径校验、IPC 权限、SA ID、接口码、隐私脱敏、allowList | `adapter/service/`, `plugins/faultlogger/service/idl/`, `plugins/privacy_controller/` | `docs/knowledge/path-and-ipc-safety.md` |
| 构建、板侧测试、PR 证据、共享库重构建、配置行为验证 | 任何构建/测试相关改动 | `docs/knowledge/board-verification.md` |

### 开始编辑前

在修改代码前，按以下顺序确认：
1. 确认任务类别
2. 根据上表确定需要阅读的文档
3. 根据"项目约束"确认不违反任何约束
4. 声明："我将修改 X，已阅读 Y 文档，遵循 Z 约束"

## 项目约束

### 性能约束

- 事件分发和故障日志写入是高频/关键路径，不要在 `Pipeline::ProcessEvent`、`PublishPipelineEvent`、`OnEvent` 中增加全表扫描、字符串格式化或 INFO 日志。
- 主循环与 `sysevent_source`/`sysevent_store` 线程为高优先级（nice -20），不要在这些线程做阻塞 IO 或长耗时操作。
- 每个 EventLoop 线程已禁用 thread cache 和 delay free（jemalloc），新增线程应遵循同样的内存策略。

### 架构约束

- 插件注册方式三选一（`REGISTER`/`REGISTER_PROXY`/`REGISTER_PROXY_WITH_LOADED`），互斥，不要在同一类上混用。
- `EventSource` 和 `EventListener` 子类不能用代理模式（`PROXY_ASSERT` 编译期断言）。
- 事件订阅两套接口（`EventListener::AddListenerInfo` 独立订阅 vs `Plugin::AddDispatchInfo` 动态订阅）不能混合调用，订阅规则互相不可见。
- 插件名必须等于类名（`REGISTER` 宏用 `#ClassName` 作为注册名）。
- 插件卸载后**不能**重新加载（`PluginFactory::UnregisterPlugin` 删除注册信息）。
- 流水线名/插件名不能重复（`CreatePipeline`/`CreatePlugin` 检查重名并上报 `LOAD_DUPLICATE_NAME`）。
- 配置文件 `hiview_platform_config` 的字段必须**严格按顺序**出现（逐行 pop_front 正则解析）。

### 编码约定

- C++ 改动优先复用项目宏：`HIVIEW_LOGI/LOGW/LOGE/LOGD`、`DEFINE_LOG_TAG`、`__UNUSED`、`DllExport`。
- 每个源文件顶部必须 `DEFINE_LOG_TAG("Tag")` 一次。禁止裸 `printf`/`cout`。
- C 字符串操作必须用 `bounds_checking_function`（`libsec_shared`）的 `memcpy_s`/`strcpy_s` 等并检查返回值为 `EOK`，禁止裸 `strcpy/memcpy`。
- 路径操作必须用 `FileUtil::PathToRealPath` 解析后校验，禁止直接拼接不可信路径。
- 新增文件路径必须确认，写文件要做老化处理。
- 解析从节点中读取的事件都不可信，需要对字符串长度、数组长度和拼接的字符串路径做合法校验。
- 在进行正则匹配时确保正则模式前后一致，对正则字符串的长度做限制。
- 调用hisysevent传入的数组类型数据需要保证原子性。
- 使用open等方法打开外部路径文件时，必须添加NOFOLLOW标记。
- 使用hisysevent参数进行文件读写、命令执行、身份检查等关键操作时必须要做校验。
- 流水线事件分发中，插件处理事件禁止有耗时操作。
- 日志输出内容精简，禁止过程的日志输出，禁止在同一个函数中多次输出日志，禁止在循环中输出日志。
- 使用外部数据时必须做校验。
- 使用外部数据做数组边界、内存大小分配等操作时，必须对数值范围做校验。

### 公共 API 约束

**Do not（禁止）：**
- 修改已发布的 inner_api/NAPI/ANI/C API 的签名、参数类型、返回值类型
- 修改已有 IPC 接口码枚举的已分配值（`hiviewfaultlogger_ipc_interface_code.h` 只能追加）
- 删除或重命名已有公共 API
- 修改 `bundle.json` 中已声明 inner_kits 的头文件列表（除非评估跨模块兼容性）
- 修改 `.map` 版本脚本中已导出的符号（破坏 ABI 稳定性）

**Ask before（修改前必须确认）：**
- 新增公共 API：确认是否需要 DFX 日志、HiSysEvent 上报、权限检查
- 新增 IPC 接口码：必须配套权限校验，且需安全评审
- 修改错误处理逻辑：确认是否影响应用层错误码兼容性
- 修改 `hisysevent.yaml`：确认是否影响事件校验和文档生成

### 安全与权限边界

**Do not（禁止）：**
- 绕过 `HiviewServiceAbility::IsSafePath` 和 `CheckIdentity` 的路径/身份校验
- 在未校验的情况下直接使用跨进程传递的文件路径、fd、共享内存
- 修改 `privacy_controller` 的 allowList 逻辑，除非经过安全评审
- 使用popen执行命令

**Ask before（修改前必须确认）：**
- 涉及 `plugins/faultlogger/service/idl/include/hiviewfaultlogger_ipc_interface_code.h` 的改动
- 涉及 GWP-ASan 开关接口（`ENABLE_GWP_ASAN_INNER` 等可能影响被检测进程稳定性）
- 涉及故障日志查询权限边界（`QuerySelfFaultLog` 只能查自身故障日志）
- 涉及设备节点操作（`/dev/bbox`、`/dev/ucollection`、`/proc/sysrq-trigger`）

### 协议与数据格式兼容性

**Do not（禁止）：**
- 修改 IPC 接口的 Parcel 序列化顺序和数据结构布局
- 修改 `plugin_config`/`xxx_plugin_config` 文件格式（`plugins:N`/`pipelines:N`/`pipelinegroups:N` 行格式由正则解析，格式错乱会导致平台启动失败）
- 修改事件原始数据 `EventRaw::RawData` 的二进制布局
- 修改 `hisysevent.yaml` 中已发布事件的 `__BASE` type/level/tag

**Ask before（修改前必须确认）：**
- 新增 IPC 接口：确认是否需要跨版本兼容性处理
- 修改事件字段：确认是否影响事件接收方和 `EventJsonParser` 校验

### 生成代码边界

**Do not（禁止）：**
- 直接修改由 `build/gen_plugin_build.py` 生成的 `plugin_config` 和 `plugin_build.gni`
- 手动编辑 `core/bundle_config/` 生成的 bundle 配置

**正确做法：**
- 修改源 `build/plugin_build.json`
- 重新运行 `gen_plugin_config` action
- bundle 配置修改 `core/bundle_config/config/plugin_bundle.json`

### 设备操作约束

**涉及真实设备时的注意事项：**
- 不执行可能影响设备正常运行的破坏性操作
- 需要在真实设备上验证的改动，必须提供板侧证据（故障日志、hilog、hidumper 输出）
