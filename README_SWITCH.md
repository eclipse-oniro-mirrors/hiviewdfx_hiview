# Hiview Trace 开关说明<a name="ZH-CN_TOPIC_TRACE_SWITCH"></a>

-   [简介](#section_intro)
-   [persist.hiview.freeze_detector](#section_freeze_detector)
    -   [开关说明](#section_define)
    -   [使用场景](#section_scenario)
    -   [Trace 保存位置](#section_path)

## 简介<a name="section_intro"></a>

Hiview 提供若干能力，根据其使用业务场景不同，可提供包括trace在内的各种能力。

## persist.hiview.freeze_detector<a name="section_freeze_detector"></a>

### 开关说明<a name="section_define"></a>

-   **参数名**：`persist.hiview.freeze_detector`
-   **取值**：`"true"` / `"false"`，默认关闭
-   **修改方式**：

    ```
    param set persist.hiview.freeze_detector true
    ```

    修改后实时生效，无需重启。

### 使用场景<a name="section_scenario"></a>

开关打开后，Hiview 在以下卡死/冻屏类故障发生时会自动抓取一份 trace，用于事后定位卡顿来源（主线程阻塞、UI 卡顿、Binder 阻塞、调度异常等）：

| 故障类型              | 触发来源                                                                |
| --------------------- | ----------------------------------------------------------------------- |
| `APP_FREEZE`          | HiCollie 检测到应用主线程阻塞、生命周期超时等应用冻屏事件             |
| `SYS_FREEZE`          | HiCollie/Watchdog 检测到系统服务长时间无响应                          |
| `SYS_WARNING`         | 系统级响应异常预警                                                    |
| `APPFREEZE_WARNING`   | 应用冻屏的早期预警（如线程阻塞 3s、生命周期超时一半等）              |

典型流程：应用卡死 → HiCollie 上报冻屏事件 → Hiview 的 freeze_detector 接收到事件 → 从持续运行的 trace 缓存中 Dump 出故障前后的 trace 文件 → 与该次冻屏故障日志关联。


### Trace 保存位置<a name="section_path"></a>

Hiview 抓取的 trace 文件统一保存在设备的以下目录：

```
/data/log/hiview/unified_collection/trace/share/
```

故障发生时的临时文件位于：

```
/data/log/hiview/unified_collection/trace/share/temp/
```

文件名可在对应的 freeze 故障日志中找到映射，复制设备上述目录下的 trace 文件即可用 SmartPerf/Trace Streamer 等工具进行分析。
