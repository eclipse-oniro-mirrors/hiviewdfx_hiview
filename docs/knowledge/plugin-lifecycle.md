# 插件生命周期知识

本文只记录插件注册、加载、卸载、线程模型和插件包 bundle 的边界。事件流转见 `event-pipeline.md`，故障日志处理见 `faultlogger-management.md`。

## 注册方式三选一（互斥）

`base/include/plugin_factory.h` 三个宏，底层都展开为 `REGISTER__`，区别在 `needCreateProxy`/`needStartupLoading`：

| 宏 | needCreateProxy | needStartupLoading | 行为 |
| --- | --- | --- | --- |
| `REGISTER(ClassName)` | false | true | 静态插件，开机加载，**永不卸载** |
| `REGISTER_PROXY(ClassName)` | true | false | 代理插件，开机**不**加载，运行时按需加载 |
| `REGISTER_PROXY_WITH_LOADED(ClassName)` | true | true | 代理插件，开机加载，可动态卸载/重载 |

注册通过静态对象 `g_staticPluginRegister` 在 main 之前触发 `PluginFactory::RegisterPlugin`，把 `PluginRegistInfo` 存入全局 map。

## PROXY_ASSERT 编译期断言

`PROXY_ASSERT` 宏（`plugin_factory.h`）对 `REGISTER_PROXY` 和 `REGISTER_PROXY_WITH_LOADED` 都会展开，做两个 `static_assert`：

- `EventSource` 子类不能用代理——事件源需在平台初始化时立即 `StartEventSource` 主动产生事件，懒加载会导致流水线无从流转。
- `EventListener` 子类不能用代理——独立监听者模式不支持动态加载。

## 线程模型

`plugin_build.json` 的 `threads` 字段配置三种模式：

| 模式 | JSON 结构 | 生成结果 | 含义 |
| --- | --- | --- | --- |
| `singledthread` | `"插件名": "线程名"` | `插件名[thread:线程名]` | 插件独享 EventLoop |
| `sharedthread` | `"线程名": ["插件A","插件B"]` | 多个插件都写 `thread:线程名` | 多插件共享 EventLoop |
| `threadpool` | `{"number":N,"poolinfo":{...}}` | `插件名[pool:池名:N]` | 线程池 |

平台三个 EventLoop（`hiview_platform.cpp`）：
- `mainWorkLoop_`（"hiview"，高优先级 nice -20）—— 主线程循环，`StartLoop(false)` 不创建新线程。
- `sharedWorkLoop_`（"plat_shared"）—— 共享工作线程。
- `privateWorkLoopMap_` —— 各插件独享线程。

**高优先级特例**（`hiview_platform.cpp`）：线程名为 `sysevent_store` 或 `sysevent_source` 时创建高优先级 EventLoop。每个 EventLoop 线程启动时调用 `MemoryUtil::DisableThreadCache()`（`event_loop.cpp`）。

插件获取线程：`Plugin::GetWorkLoop()`（配置了线程才有）、`HiviewContext::GetSharedWorkLoop()`（共享线程）、`GetMainWorkLoop()`（主线程）。

## 命名约束

- **插件名必须等于类名**：`REGISTER` 宏用 `#ClassName` 作为注册名（`plugin_factory.h`），`plugin_config` 中插件名也必须与类名一致。
- **流水线名不能重复**：`CreatePipeline`（`hiview_platform.cpp`）检查重名并跳过。
- **插件名不能重复**：`CreatePlugin` 检查重名并上报 `LOAD_DUPLICATE_NAME`。
- **so 命名**：`lib{bundlename}.z.so`（全小写，`GetDynamicLibName` `hiview_platform.cpp`）。
- **配置文件命名**：`{bundleName}_plugin_config`（`SplitBundleNameFromPath` 取 `_plugin_config` 前的部分）。

## 修改前检查

- 该插件用哪种注册方式？是否触发了 `PROXY_ASSERT`（EventSource/EventListener 不能用代理）？
- 该插件是否配置了线程？线程名是否为 `sysevent_store`/`sysevent_source`（高优先级）？
- 卸载逻辑是否会触发 `UnregisterPlugin`（不可重载）？
- `use_count` 检查是否会因事件未处理完而拒绝卸载？
- 新增插件是否在 `plugin_build.json` 声明（path、name、loadtime、pipeline、threads）？
- 插件名/流水线名是否与已有重名？

## 测试指引

- 插件注册与工厂：使用 `PluginFactoryTest`（`base/test/BUILD.gn`）。
- 插件平台集成：使用 `PluginPlatformTest`、`HiviewPlatformModuleTest`（`core/test/BUILD.gn`、`test/BUILD.gn`）。
- 插件包 bundle：使用 `PluginBundleTest`（`core/test/BUILD.gn`），依赖 `examples_bundle:bundleplugintest`。
- 三种注册模式示例：`event_processor_example1`（静态）、`event_processor_example3`（代理）、`event_processor_example4`（代理且加载）。
- 动态加载插件：`dynamic_load_plugin_example`（`libdynamicloadpluginexample.z.so`）。
- 配置解析：使用 `ParseConfigTest`（`base/test/BUILD.gn`）。
- 参数更新：使用 `ParamUpdateTest`（`core/test/BUILD.gn`）。
