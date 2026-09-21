# UnrealDbg - 原生 VT 调试器

UnrealDbg 是开源的 VT 调试工具。当前提供原生 WinUI 3 C++ 前端（无 WebView UI），后端继续使用原有 UnrealDbgDll 接口和驱动协议。

## Features 🛠️

UnrealDbg comes packed with a variety of features to assist developers in their debugging tasks. Some of the key features include:

- **VT Technology**: Utilizes Virtual Table (VT) technology for enhanced debugging capabilities.
- **Native WinUI 3 Interface**: 暗黑主题、单页操作和底部实时分级日志，运行在原生 XAML 控件上。
- **Open Source**: Codebase is open for collaboration and customization.
- **Windows 适配**: 按精确 Windows Build/UBR 支持表选择 Win10/Win11 调试子系统驱动；未知版本在加载前拒绝。
- **Extensive Debugging Tools**: Offers a range of tools for thorough debugging processes.

![Debugger Image](https://github.com/Code4sanz/UnrealDbg/releases)

## Installation Guide 📦

To install UnrealDbg, follow these steps:

1. 使用 Visual Studio 2019/WDK 打开 `UnrealDbg.sln`。
2. 构建 `UnrealDbgNativeWinUI.vcxproj` 的 x64 Release 配置。
3. 先确保机器安装 Windows App Runtime 1.8 x64，再运行 `x64\WinUI\Release\UnrealDbgNativeWinUI.exe`。

For more detailed installation instructions, refer to the [official documentation](https://github.com/Code4sanz/UnrealDbg/releases).

## Usage 🚩

UnrealDbg can be used for various debugging purposes, including:

- Analyzing code execution flow.
- Inspecting variables and memory.
- Setting breakpoints for efficient debugging.
- Tracking function calls and returns.

Get started with UnrealDbg today and take your debugging process to the next level!

## Download App 📥

Click the button below to download the latest version of UnrealDbg:

[![Download UnrealDbg](https://github.com/Code4sanz/UnrealDbg/releases)](https://github.com/Code4sanz/UnrealDbg/releases)

*Note: Make sure to launch the downloaded file to start using the application.*

## Contributing 🤝

Contributions to UnrealDbg are welcome! If you have any ideas for improvements or new features, feel free to open an issue or submit a pull request. Together, we can make UnrealDbg even better for the developer community.

## License ℹ️

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## 目录结构

- `UnrealDbg.sln`：Visual Studio/WDK 解决方案；根目录仅保留源码、文档和解决方案文件。
- `x64\Debug`、`x64\Release`：编译后的运行目录，根目录只保留启动 EXE 和功能子目录。
- `x64\*\bin`：运行时 DLL、注入组件和 SYS 驱动；程序会优先从这里加载，旧版根目录布局仍可回退。
- `x64\*\Config`：`copyright.db`、`DebuggerList.ini`、`Config.ini` 等配置文件；新配置始终写入这里。
- `x64\*\Symbols`：PDB、EXP、LIB 等调试/链接文件，不参与运行。
- `x64\*\Certificates`：驱动测试证书，只有安装或签名时需要。
- `x64\Release\Legacy`：旧版 Delphi 前端及旧版兼容文件，仅作归档保留。
- `x64\Release\Legacy\旧版驱动布局`：历史发布包中的重复驱动副本，仅作归档，不参与加载。
- `_artifacts\logs`：历史构建日志和自测记录。
- `x64\*\Log`：程序运行日志。

## WinUI 前端

`UnrealDbgNative\UnrealDbgNativeWinUI.vcxproj` 是新的原生 WinUI 3 前端，代码在 `WinUiMain.cpp`，后端编排在 `WinUiController.cpp`。布局采用单页状态概览、调试器列表、TL 选项和底部全局日志，默认暗黑主题。它不使用 WebView，不改变原有调试功能；驱动加载使用事务状态机，设备通过受控 ACL 和 V2 IOCTL 访问。

精确版本表、事务恢复规则、签名发布路线和 Win11 测试矩阵见 `docs\Windows支持与发布策略.md` 与 `tests\windows\WindowsCompatibilityMatrix.md`。

构建脚本使用 `_artifacts\winui_packages` 中的 Windows App SDK/CppWinRT 包生成投影头；重新部署到其他机器时，需要安装匹配的 Windows App Runtime 1.8 x64。构建产物放在 `x64\WinUI\Debug` 或 `x64\WinUI\Release`，运行 DLL、驱动仍集中在 `bin` 子目录。

首次构建前如果 `_artifacts\winui_packages` 不存在，请在仓库根目录执行 `powershell -ExecutionPolicy Bypass -File UnrealDbgNative\RestoreWinUiPackages.ps1`，脚本会从 NuGet 恢复编译依赖；UI 本身不使用 WebView。

这样整理不会改变程序查找依赖的协议；`UnrealDbgNative.exe` 会自动解析 `bin`、`Config`，并兼容旧版根目录文件。直接运行对应配置目录中的 `UnrealDbgNative.exe` 即可。

---

Let's enhance your debugging experience with UnrealDbg! Happy debugging! 🚀

![Debugging Image](https://github.com/Code4sanz/UnrealDbg/releases)
