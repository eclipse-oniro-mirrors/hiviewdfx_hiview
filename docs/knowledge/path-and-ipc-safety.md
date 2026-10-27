# 路径与 IPC 安全知识

本文只记录路径校验、IPC 权限、SA ID、接口码保护、隐私脱敏的边界。故障日志 IPC 见 `faultlogger-management.md`，统一采集见 `unified-collection.md`。

## IsSafePath 路径校验

`adapter/service/server/src/hiview_service_ability.cpp`：

```cpp
bool IsSafePath(const std::string& basePath, const std::string& fullPath)
{
    std::string realBasePath;
    if (!FileUtil::PathToRealPath(basePath, realBasePath)) { return false; }
    std::string realFullPath;
    if (!FileUtil::PathToRealPath(fullPath, realFullPath)) { return false; }
    realBasePath = FileUtil::IncludeTrailingPathDelimiter(realBasePath);
    return realFullPath.find(realBasePath) == 0;
}
```

校验规则：
1. 对 `basePath` 和 `fullPath` 都做 `realpath()` 解析（消除 `..`、符号链接）。
2. 给 `realBasePath` 补尾部路径分隔符（防止 `/log/LogService` 前缀匹配 `/log/LogServiceEvil`）。
3. 检查 `realFullPath` 是否以 `realBasePath` 开头（`find(...) == 0`）。

调用点（`hiview_service_ability.cpp`）：Copy/Move 文件操作时校验目标路径不逃逸出允许的目录范围。

**必要性**：hiview 处理不可信事件输入与 IPC 路径参数。若不校验，攻击者可通过 `../../../etc/passwd` 路径穿越读写任意文件。

## CheckIdentity 身份校验

`HiviewServiceAbility`（`hiview_service_ability.h`，SystemAbility）在 IPC 接口入口校验调用方身份。`HasAccessPermission`检查调用方是否具备所需权限。

## IPC 接口码保护

### faultlogger IPC 接口码

`plugins/faultlogger/service/idl/include/hiviewfaultlogger_ipc_interface_code.h`：

- `FaultLoggerServiceInterfaceCode`（SAID: 1202）：ADD_FAULTLOG、QUERY_SELF_FAULTLOG、ENABLE/DISABLE/GET_GWP_ASAN_GRAYSALE、DESTROY、ENABLE_GWP_ASAN_INNER。
- `FaultLogQueryResultInterfaceCode`：HASNEXT、GETNEXT。

**约束**：
- 枚举值**只能追加**，不能修改/删除/重排已分配值（否则客户端与服务端枚举错位）。
- 新增接口码必须配套权限校验。
- 需安全评审。

### HiviewServiceAbility 接口

涉及文件操作（Copy/Move/Remove/ListFiles）的接口必须经 `IsSafePath` 校验。

## 隐私脱敏

### privacy_controller

- `OnEvent` 中 `IsBundleNameAllow`/`IsValidParam` 按 allowListFile 校验 bundle 名和参数是否允许上报。
- **禁止修改 allowList 逻辑，除非经过安全评审**。

## bounds_checking_function 约束

`bounds_checking_function`（`libsec_shared`）是必选依赖（`base/BUILD.gn`，`bundle.json`）。代码中大量使用 `memcpy_s`/`memset_s`/`strcpy_s`/`strncpy_s`：

```cpp
if (memcpy_s(des, desLen, source, sourceHeaderLen) != EOK) { return; }
```

**约束**：禁止裸 `strcpy/memcpy`，必须用 `_s` 版本并检查返回值为 `EOK`。路径操作必须用 `FileUtil::PathToRealPath` 解析后校验，禁止直接拼接不可信路径。

## 修改前检查

- 文件操作（Copy/Move/Remove/ListFiles）是否经 `IsSafePath` 校验？
- 新增 IPC 接口是否配套权限校验（`HasAccessPermission`/`CheckIdentity`）？
- IPC 接口码是否只追加不修改？
- 是否修改了 `privacy_controller` 的 allowList 逻辑（需安全评审）？
- 是否修改了参数 DAC 配置（可能影响其他进程访问）？

## 测试指引

- HiviewServiceAbility：使用 `HiviewSATest`、`AdapterLoglibraryAbilityTest`（`adapter/service/test/BUILD.gn`）。
- HiviewServiceAbility Stub fuzz：使用 `HiviewServiceAbilityStubFuzzTest`（`adapter/service/BUILD.gn`）。
- HiviewLogConfigManager fuzz：使用 `HiviewLogConfigManagerFuzzTest`（同上）。
- HiviewService fuzz：使用 `HiviewServiceFuzzTest`（`service/test/fuzztest/common/hiviewservice_fuzzer/BUILD.gn`）。
- faultlogger fuzz：约 46 个 fuzz target 覆盖 IPC 接口（`plugins/faultlogger/BUILD.gn`）。
- 路径校验、权限边界和设备节点操作的改动需补充 `board-verification.md` 中的板侧证据。
