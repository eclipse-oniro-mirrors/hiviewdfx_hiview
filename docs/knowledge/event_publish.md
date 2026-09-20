# 应用事件发布知识

本文记录 `base/event_publish` 模块的权威模型。该模块负责把 OS 域 HiSysEvent（如 `APP_CRASH`、`APP_FREEZE`、`RESOURCE_OVERLIMIT`）投递到应用沙箱，供注册为观察者的应用接收。编译开关见 `hiview.gni` 的 `hiview_appevent_publish_enable`，编译产物随 `hiviewbase` 一起构建（见 `base/BUILD.gn` 的 `ohos_shared_library("hiviewbase")`）。

## 核心类与职责

| 类 | 头文件 | 职责 | 常见误用 |
|---|---|---|---|
| `EventPublish` | `event_publish.h` | 单例门面，提供 `PushEvent`/`IsAppListenedEvent`，使用 Pimpl(`Impl`) 隐藏线程逻辑 | 在非 hiview 进程或未校验 uid 时直接调用 |
| `AppEventHandler` | `app_event_handler.h` | 定义强类型事件结构体（`AppLaunchInfo`、`ScrollJankInfo`、`ResourceOverLimitInfo` 等）及 `PostEvent` 重载，序列化为 JSON 后委托 `EventPublish::PushEvent` | 绕过它直接拼 JSON 字符串调用 `PushEvent` |
| `AppEventPublisher` | `app_event_publisher.h` | 抽象插件接口，继承 `Plugin`，仅声明 `AddAppEventHandler` | 不配合 `REGISTER_PUBLISHER` 单独继承 |
| `AppEventPublisherFactory` | `app_event_publisher_factory.h` | 发布者注册表，`REGISTER_PUBLISHER(ClassName)` 宏在静态期完成登记 | 在运行时动态注册后忘记静态注册 |
| `UserDataSizeReporter` | `user_data_size_reporter.h` | 上报 `USER_DATA_SIZE`，按 `pathHolder_eventName` 做 24h 去重 | 每次推送事件都触发上报 |
| `LogFileNameConverter` | `log_file_name_converter.h` | 将内存泄漏类原始日志名规整为 `RESOURCE_OVERLIMIT_{ts}_{pid}_{suffix}` | 对非 `RESOURCE_OVERLIMIT` 事件调用 |
| `ElapsedTime` | `app_event_elapsed_time.h` | 耗时打点辅助，超阈值才打印 | 当作精确计时器依赖 |

## 事件分发路径

`EventPublish::Impl::PushEvent`（`event_publish.cpp:684`）按事件名分三种投递策略，不应混用：

- **立即上报**（默认）：先 `SaveLogToSandBox` 拷贝外部日志、再 `SaveEventToSandBox` 写事件 JSON 到 `cache/hiappevent/hiappevent_{ts}.txt`，最后 `ReportUserDataSize`。`APP_CRASH`、`APP_FREEZE`、`APP_LAUNCH`、`CPU_USAGE_HIGH`、`MAIN_THREAD_JANK`、`APP_HICOLLIE`、`APP_KILLED`、`AUDIO_JANK_FRAME`、`ADDRESS_SANITIZER`、`APPFREEZE_WARNING` 均走此路径。
- **超限上报**（`EVENT_RESOURCE_OVERLIMIT`）：由 `StartOverLimitThread` 起独立线程执行 `SendOverLimitEventToSandBox`，写入 `log/{pathHolder}/resourcelimit` 沙箱目录，避免阻塞主链路。同一时刻只允许一个 overlimit 线程。
- **延迟上报**（`EVENT_SCROLL_JANK`、`EVENT_BATTERY_USAGE`）：先 `SaveEventToTempFile` 写入临时文件 `/data/log/hiview/system_event_db/events/temp/hiappevent_{uid}.evt`，再由 `StartSendingThread` 休眠 `DELAY_TIME`(30s) 后批量拷贝到各应用沙箱。延迟线程单例，运行期间不复起。延迟上报期间需保活应用前台 30s，否则休眠期内应用被杀或退后台将导致 `GetPathPlaceHolder` 取不到沙箱、拷贝目标目录不存在，进而拷贝失败并清理临时文件丢失事件。

## 沙箱路径占位符

`GetPathPlaceHolder`（`event_publish.cpp:202`）按应用类型解析沙箱路径占位符，路径错误将导致事件丢失：

- 主应用：直接用 `bundleName`。
- 克隆应用（`appIndex > 0`）：`+clone-{appIndex}+{bundleName}`。
- 输入法扩展：`+extension-entry-InputMethodExtensionAbility+{bundleName}`，需先校验沙箱存在。
- 原子服务（`entryInstallationFree`）：由 `BundleMgrClient::GetDirByBundleNameAndAppIndex` 取目录。

`BundleUtil::GetSandBoxPath(uid, "base"\|"log", pathHolder, subPath)` 拼最终路径。base 沙箱不存在时 `PushEvent` 直接返回并清理临时文件。

## 监听校验

`CheckAppListenedEvents`（`event_publish.cpp:517`）通过沙箱目录的 xattr `user.appevent` 读取一个位图，按 `OS_EVENT_POS_INFOS` 中事件对应的 bit 位判断应用是否订阅该事件。未订阅的事件不应投递。`IsAppListenedEvent` 是对外查询入口，供 `faultlogger` 等插件预判是否需要采集附加日志。

## 外部日志与链接文件

`SaveLogToSandBox`（`event_publish.cpp:412`）处理事件参数中的 `external_log` 数组：

- 路径必须通过 `VerifyPathSecurity`，即 realpath 以 `/data/log/` 开头；沙箱内路径（`/data/storage/el2/log`）不拷贝只引用。
- 每类事件有独立的大小上限：通用 `MAX_FILE_SIZE`(5M)、minidump `DMP_MAX_FILE_SIZE`(35M)、`MAIN_THREAD_JANK` `WATCHDOG_MAX_FILE_SIZE`(10M)、`RESOURCE_OVERLIMIT` `RESOURCE_OVERLIMIT_MAX_FILE_SIZE`(2G)。超限置 `log_over_limit=true` 并中断。
- 拷贝后由 `CreateLinkFile` 按观察者数量 `observerNum`（来自 xattr `user.event_config.{eventName}`）创建硬链接 `_{i}` 后缀文件，并回填 `link_external_log`。`chown` 失败需回滚文件与链接。

## 发布者插件模式

插件不应直接 `new AppEventHandler`。正确流程：

1. 插件类继承 `AppEventPublisher`（同时仍是 `Plugin`），实现 `AddAppEventHandler` 存储句柄。
2. 在 `.cpp` 中 `REGISTER(ClassName); REGISTER_PUBLISHER(ClassName);`，后者在静态期把类名登记进 `AppEventPublisherFactory`。
3. 平台启动时 `core/hiview_platform.cpp` 对注册名匹配的插件构造默认 `AppEventHandler` 并 `AddAppEventHandler` 注入。现役发布者为 `XperfPlugin`（性能）与 `FaultDetectorManager`（内存泄漏）。
4. 仅当只需推送原始事件、不需要强类型结构体时，才直接调 `EventPublish::GetInstance().PushEvent`，如 `faultlog_cppcrash.cpp`、`unified_collector.cpp`。

## 编译开关与降级

`hiview.gni` 的 `hiview_appevent_publish_enable`（默认 `true`，见 `bundle.json`）开关：

- 开启：编译 `event_publish.cpp`、`app_event_handler.cpp`、`app_event_publisher_factory.cpp`、`log_file_name_converter.cpp`、`user_data_size_reporter.cpp`、`app_event_elapsed_time.cpp`，并依赖 `bundle_framework`、`samgr_proxy`、`storage_manager_acl` 等。
- 关闭：只编译 `app_event_publish_unable.cpp`，所有接口返回空实现（`PostEvent` 返回 -1、`PushEvent` 空函数体），保证下层不感知功能缺失。

## 模块编译与注意事项

- 模块代码编译目标为 `hiviewbase`（`base/BUILD.gn` 的 `ohos_shared_library("hiviewbase")`）。
- 模块UT用例的编译目标为 `EventPublishTest`（`ohos_unittest("EventPublishTest")`），输出路径 `hiview/event_publish`。
- 新增 OS 事件类型时，应同时在 `event_publish.h` 的 `HiAppEvent` 命名空间加常量、在 `OS_EVENT_POS_INFOS` 补 bit 位，并确认 `AppEventHandler` 是否需要配套 `PostEvent` 重载。
- 调整外部日志上限时，不应超过 `CopyFileFast` 的 `maxFileSizeBytes` 上层约束，且需同步 `UserDataSizeReporter` 的上报路径集合。

## 测试用例主要内容

- `EventPublishTest003` 覆盖立即上报事件集
- `EventPublishTest005` 覆盖 `RESOURCE_OVERLIMIT`
- `EventPublishTest008` 覆盖延迟上报（需保活应用在前台 30s）
- `EventPublishTest010` 覆盖外部日志拷贝
- `EventPublishTest011` 覆盖非对象 JSON 拒收
- `AppEventPublisherFactoryTest001` 覆盖发布者注册/反注册
- `LogFileNameConverterTest` 覆盖日志名规整