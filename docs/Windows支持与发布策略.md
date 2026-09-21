# 精确 Windows 版本支持表与发布策略

程序使用 `RtlGetVersion` 获取真实内核版本，并读取 `HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\UBR`。只有下表中明确列出的 Build + UBR 范围才允许进入驱动事务；未知版本会在加载任何驱动前返回 `ERROR_NOT_SUPPORTED`。

| Build | 系统版本 | 桥接驱动 | 当前状态 |
|---:|---|---|---|
| 19045 | Windows 10 22H2 | DbgkSysWin10.sys | 开发基线，待快照虚拟机验证 |
| 22000 | Windows 11 21H2 | DbgkSysWin11.sys | 开发基线，待快照虚拟机验证 |
| 22621 | Windows 11 22H2 | DbgkSysWin11.sys | 开发基线，待快照虚拟机验证 |
| 22631 | Windows 11 23H2 | DbgkSysWin11.sys | 开发基线，待快照虚拟机验证 |
| 26100 | Windows 11 24H2 | DbgkSysWin11.sys | 开发基线，待快照虚拟机验证 |
| 26200 | Windows 11 25H2 | DbgkSysWin11.sys | 当前开发基线，必须完成隔离 VM 回归 |

“开发基线”不是正式兼容承诺。只有在矩阵中的 CPU、Hyper-V/VBS/HVCI、签名和重启用例全部通过后，才可把条目改为“已验证”。Windows 更新后如果 Build 改变，必须重新执行矩阵。符号缓存清单同时记录系统模块文件版本与 CodeView GUID/Age；任一项变化都会使缓存失效并重新验证，不会沿用旧符号。

## 驱动事务状态机

```text
精确版本检查 → 服务身份/二进制路径检查 → VT_Driver 核心就绪
→ Win10/Win11 桥接驱动就绪 → 打开 \\.\UnrealDbg → IOCTL 符号握手 → Ready
```

任何阶段失败都会记录阶段、Win32 错误码、系统原文、原因和解决方案。桥接阶段失败时仅补偿本次创建的 `UnrealDevice`；VT 核心可能已经进入 VMX 状态，程序不会未经验证强制卸载，而是明确要求重启恢复。

## 签名发布路线

- 开发测试：使用隔离快照 VM 和测试证书；证书必须导入“本地计算机\受信任的根证书颁发机构”和“受信任的发布者”。
- 正式发布：使用 Microsoft 认可的内核驱动签名流程（硬件开发者中心/Attestation 或 WHQL），并对 SYS、CAT、INF 和发布包做哈希记录。
- 不通过关闭 HVCI、关闭安全启动或永久启用 TESTSIGNING 来规避 577；这些设置只可在明确隔离的开发 VM 中使用。

## 日志协议

驱动调试输出使用 `UDBG-UTF8/1` 行协议，文本按 UTF-8 解释并包含组件、时间、级别和状态码。内核不再创建或写入 `C:\Logs\driver.xml`；用户态日志统一写入发布目录下的 `Log\UnrealDbgDll.log`，后续可由 ETW/WPP 收集器接入同一协议。

## 开发诊断日志与复现包

开发测试版本会把每一项本地日志关联到“会话号 + 序号 + PID + TID”。这让一次失败的前端、后端、服务状态和部署文件可以按时间顺序还原，而不是只剩一个孤立错误码。

- `Log\log.ini` 是前端滚动汇总日志，达到 16 MB 时尽力轮转为 `log.previous.ini`；轮转被占用时继续追加，绝不主动丢失记录。
- `Log\Sessions\UnrealDbg-*.log` 是前端单进程完整会话，单文件上限为 64 MB；达到上限会写明原因，汇总日志继续记录。
- `Log\UnrealDbgDll.log` 是后端滚动汇总日志；`Log\Sessions\UnrealDbgDll-*.log` 保存后端 DLL 的独立会话。后端初始化密钥始终脱敏，不会写入任何日志或诊断包。
- 前端会在启动完成、用户请求进入 VT 前、符号准备失败后、VT 初始化成功或失败后记录只读环境快照：Windows Build/UBR、管理员状态、VT/Hypervisor/VBS/HVCI/安全启动状态、运行时 DLL/SYS 的路径/大小/修改时间/SHA-256，以及 `VT_Driver`、`UnrealDevice` 的服务状态、二进制路径和退出码。

复现问题时运行下列命令即可生成只读诊断包；它不会加载驱动或更改任何系统安全设置：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\windows\Collect-DriverDiagnostics.ps1
```

诊断包包含应用日志（默认最近 12 个会话）、运行时文件 SHA-256 和签名状态、服务配置/状态、最近七天的服务控制管理器和 Code Integrity 事件、操作系统/CPU/DeviceGuard 状态，以及一份采集清单。单个异常日志超过 64 MB 时仅复制末尾 25000 行，并在包内明确标示；原日志不会被修改。
