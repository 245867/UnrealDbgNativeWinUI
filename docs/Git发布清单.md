# Git 发布清单 — 什么可以发布，什么不能

> 审查日期：2026-09-21
> 审查范围：`D:\Backup\Documents\VT调试器本地版` 全部内容
> Git 仓库当前跟踪文件数：1008

---

## 一、总览

| 分类 | 可否发布 | 当前状态 | 文件数 |
|------|:--------:|---------|------:|
| 源代码（.cpp/.h/.pas/.asm） | ✅ 可以 | 已跟踪 | ~467 |
| 工程文件（.vcxproj/.dpr/.dproj/.sln） | ✅ 可以 | 已跟踪 | ~30 |
| ia32-doc 文档（.yml） | ✅ 可以 | 已跟踪 | ~168 |
| 脚本（.py） | ✅ 可以 | 已跟踪 | ~16 |
| 文档（.md） | ✅ 可以 | **未跟踪**，需添加 | 3 |
| 测试脚本（.ps1/.md） | ✅ 可以 | **未跟踪**，需添加 | 3 |
| WinUI 前端源码（UnrealDbgNative/） | ✅ 可以 | **未跟踪**，需添加 | ~20 |
| 根目录批处理（.bat） | ✅ 可以 | **未跟踪**，需添加 | 2 |
| Delphi __history 备份（~N~） | ❌ 不可以 | **已跟踪，需移除** | 245 |
| IDE 缓存（.identcache/.dproj.local 等） | ❌ 不可以 | **已跟踪，需移除** | 11 |
| VS 用户配置（.vcxproj.user/.aps） | ❌ 不可以 | **已跟踪，需移除** | 8 |
| Delphi 编译资源（.res） | ⚠️ 视情况 | 已跟踪 | 4 |
| copyright.db（加密版权数据） | ⚠️ 视情况 | 已跟踪 | 1 |
| F3 调试器发布包 | ❌ 绝对不可以 | 未跟踪（.gitignore 已排除） | ~170 |
| 外部工具（DRCE/x64dbg_plus 等） | ❌ 不可以 | 未跟踪 | ~大量 |
| 编译产物（.dll/.sys/.exe/.pdb） | ❌ 不可以 | 未跟踪（.gitignore 已排除） | ~大量 |
| 证书文件（.cer/.pfx） | ❌ 绝对不可以 | 未跟踪（.gitignore 已排除） | — |
| 运行日志（.log） | ❌ 不可以 | 未跟踪（.gitignore 已排除） | — |

---

## 二、可以发布的文件

### 2.1 已被 Git 跟踪，直接发布

| 类别 | 扩展名 | 说明 |
|------|--------|------|
| C/C++ 源码 | `.cpp` `.h` | VT_Driver, Hook, Loader, Common, UnrealDbgDll, D-encryptiondll 等模块 |
| 汇编源码 | `.asm` | VMX 启动/退出桩，ia32 指令 |
| Delphi 源码 | `.pas` `.dpr` `.dfm` | UnrealDbg 旧版前端, CardRegistration, D-encryption, SymbolTool |
| VS/WDK 工程文件 | `.vcxproj` `.vcxproj.filters` | 各模块的编译配置（不含 .user） |
| Delphi 工程文件 | `.dproj` | Delphi 项目配置（不含 .local） |
| 解决方案 | `.sln` | UnrealDbg.sln |
| ia32-doc | `.yml` `.conf` | Intel 指令集文档（CPUID/VMX/MSR 定义），开源 |
| Python 脚本 | `.py` | 辅助工具脚本 |
| 图标资源 | `.ico` | 程序图标 |
| 文本文件 | `.txt` | 配置说明、注意事项 |
| 许可证 | `LICENSE` | MIT 许可证 + Phnt 许可证 |
| Git 配置 | `.gitignore` `.gitattributes` | 版本控制配置 |

### 2.2 应该发布但当前未被 Git 跟踪（需 `git add`）

| 路径 | 说明 | 操作 |
|------|------|------|
| `docs/` | 项目文档（3个 .md 文件） | `git add docs/` |
| `tests/` | 测试脚本（.ps1 + 兼容性矩阵 .md） | `git add tests/` |
| `UnrealDbgNative/` | **WinUI 3 前端全部源码** — 当前完全未跟踪！包含 WinUiMain.cpp, Core.cpp, WinUiController.cpp, LogPanel.cpp 等 | `git add UnrealDbgNative/` |
| `启动GUI.bat` | 启动批处理 | `git add 启动GUI.bat` |
| `构建并验证GUI.bat` | 构建批处理 | `git add 构建并验证GUI.bat` |
| `Common/Ring0/SymbolicAccess/Utils/LegacyLog.h` | 新增头文件 | `git add` |
| `Common/Ring0/SymbolicAccessKM.NOTICE.txt` | 通知文件 | `git add` |
| `Common/Shared/WindowsBuildSupport.h` | 新增头文件 | `git add` |
| `Common/VMProtect/VMProtectSDK_stub.cpp` | VMProtect stub | `git add` |
| `VT_Driver/Log.cpp` | 新增源文件 | `git add` |

> ⚠️ **UnrealDbgNative/ 是整个 WinUI 前端的源码，当前完全没有被 Git 跟踪。** 这是最重要的遗漏——如果不 `git add`，推送到远程仓库后该模块将完全缺失。

---

## 三、不能发布的文件

### 3.1 ★ F3 调试器发布包 — 绝对禁止

| 路径 | 大小 | 内容 | 禁止原因 |
|------|------|------|---------|
| `x64/Release/F3/` | 89MB | 完整调试器发布包：编译产物(DLL/SYS/EXE)、加密文件(UnrealDbg.aes)、版权数据库(F3copyright.db)、配置(DebuggerList.ini)、日志、符号工具 | 含编译产物、加密版权数据、专有发布包 |
| `x64/Release/F3_VT9_15/` | 331MB | VT 9.15 版本完整发布包，同上 + tool 目录 | 同上 |
| `x64/Release/F3_VT_9_16/` | 93MB | VT 9.16 版本完整发布包，同上 | 同上 |
| `x64/Release/F3(新).rar` | — | F3 压缩包归档 | 同上 |

**F3 目录内含的敏感文件类型：**
- `F3copyright.db` — 版权保护数据库（加密二进制）
- `UnrealDbg.aes` — AES 加密文件（可能为加密的前端或配置）
- `*.sys` — 编译后的内核驱动（VT_Driver.sys, DbgkSysWin10.sys, DbgkSysWin11.sys）
- `*.dll` — 编译后的 DLL（UnrealDbgDll.dll, Hook64.dll, D-encryption.dll, AIHelper.dll, VMProtectSDK64.dll）
- `*.exe` — 可执行文件（LeoMoon CPU-V.exe）
- `DebuggerList.ini` — 调试器路径配置
- `修复or使用or各系统测试文档/` — 内部测试文档

**当前状态：** ✅ 已被 `.gitignore` 的 `**/x64/` 规则排除，Git 不跟踪，不会推送。

**安全约束：** 禁止删除、修改、上传到 Git、外传给第三方。

### 3.2 外部工具 — 禁止发布（第三方版权）

| 路径 | 大小 | 内容 | 禁止原因 |
|------|------|------|---------|
| `x64/Release/DRCE(26.8.6)/` | 42MB | DrCE.exe（CE 修改版） | 第三方工具，非本项目授权 |
| `x64/Release/x64dbg_plus(1.2)/` | 177MB | x64dbg 增强版 | 第三方工具，GPLv3 许可证 |
| `x64/Release/DSigntool签名/` | 5.8MB | 亚洲诚信签名工具修改版 | 第三方工具 + 签名相关 |
| `x64/Release/TimestampClient/` | 5.0MB | 时间戳客户端 | 第三方工具 |
| `x64/Release/帖子/` | 1.4MB | 52pojie 论坛保存网页 | 转载内容 |

**当前状态：** ✅ 已被 `.gitignore` 的 `**/x64/` 规则排除。

### 3.3 编译产物 — 禁止发布

| 类型 | 扩展名 | 当前 .gitignore 规则 |
|------|--------|-------------------|
| 动态链接库 | `.dll` | ✅ `*.dll` |
| 可执行文件 | `.exe` | ✅ `*.exe` |
| 驱动文件 | `.sys` | ❌ **缺失！** 需添加 `*.sys` |
| 调试符号 | `.pdb` | ✅ `*.pdb` |
| 导出文件 | `.exp` | ✅ `*.exp` |
| 静态库 | `.lib` | ✅ `*.lib` |
| 对象文件 | `.obj` | ✅ `*.obj` |
| 编译中间 | `.tlog` `.recipe` `.lastbuildstate` | ✅ `**/x64/` 覆盖 |
| 编译日志 | `.log` | ✅ `*.log` |

### 3.4 证书与密钥 — 绝对禁止

| 类型 | 路径 | 当前 .gitignore 规则 |
|------|------|-------------------|
| 测试证书 | `x64/*/Certificates/*.cer` | ✅ `*.cer` + `**/x64/` |
| 证书数据库 | `copyright.db` | ❌ **未排除，且已跟踪！** 需处理 |
| 加密文件 | `UnrealDbg.aes` (F3目录内) | ✅ `**/x64/` 覆盖 |
| 密钥文件 | `License.key`（运行时生成） | ✅ 不在仓库中 |

### 3.5 IDE 缓存与备份 — 禁止发布（当前已被跟踪，需移除）

| 类型 | 扩展名/模式 | 当前跟踪数 | 当前 .gitignore 规则 | 需要操作 |
|------|------------|:---------:|-------------------|---------|
| Delphi 历史备份 | `__history/` 内 `~N~` 文件 | 245 | ❌ **缺失** | `git rm -r --cached **/__history/` + 加 .gitignore |
| Delphi 标识缓存 | `*.identcache` | 6 | ❌ **缺失** | `git rm --cached *.identcache` + 加 .gitignore |
| Delphi 本地配置 | `*.dproj.local` | 4 | ❌ **缺失** | `git rm --cached *.dproj.local` + 加 .gitignore |
| Delphi 项目组本地 | `*.groupproj.local` | 1 | ❌ **缺失** | `git rm --cached *.groupproj.local` + 加 .gitignore |
| VS 用户配置 | `*.vcxproj.user` | 7 | ❌ **缺失** | 已从磁盘删除，需 `git rm --cached` |
| VS 资源缓存 | `*.aps` | 1 | ❌ **缺失** | 已从磁盘删除，需 `git rm --cached` |

### 3.6 其他不应发布的文件

| 路径 | 说明 | 当前状态 |
|------|------|---------|
| `x64/Release/成品签名/` | 空目录 | ✅ 已排除 |
| `x64/Release/Symbols/` | PDB/EXP/LIB 调试符号 | ✅ 已排除 |
| `x64/Release/bin/` | 运行时 DLL/SYS 编译产物 | ✅ 已排除 |
| `Log/` | 运行日志目录 | ✅ `*.log` 排除 |
| `.git/` | Git 仓库元数据 | ✅ Git 自动管理 |

---

## 四、.gitignore 改进建议

当前 `.gitignore` 缺少以下规则，建议补充：

```gitignore
# === 需要新增的规则 ===

# Delphi IDE 自动备份
**/__history/
*.~*~

# Delphi IDE 缓存
*.identcache
*.dproj.local
*.groupproj.local
*.tgsproj

# Visual Studio 用户配置
*.vcxproj.user
*.aps
*.suo
*.VC.db
*.VC.opendb
*.sdf
*.opensdf

# 驱动文件（当前缺失）
*.sys
*.cat

# 加密版权数据库
copyright.db
*.aes

# WinUI 构建临时
*.winmd
*.pri
*.pkg

# 诊断输出
_artifacts/

# 桌面快捷方式（用户特定）
*.lnk
```

> 注意：部分规则（如 `*.sys`, `*.cat`）虽在 `.gitignore` 中添加，但如果文件已被跟踪则不会自动停止跟踪。需要配合 `git rm --cached` 操作。

---

## 五、需要执行的 Git 操作清单

### 5.1 从 Git 跟踪中移除不应发布的文件

```bash
# 移除 Delphi __history（245个文件）
git rm -r --cached "**/__history/"

# 移除 IDE 缓存文件
git rm --cached "*.identcache"
git rm --cached "*.dproj.local"
git rm --cached "*.groupproj.local"
git rm --cached "*.vcxproj.user"
git rm --cached "*.aps"

# 移除 copyright.db（如果决定不发布）
git rm --cached "D-encryptiondll/D-encryption/copyright.db"

# 可选：移除 Delphi 编译资源（.res）
git rm --cached "*.res"
```

### 5.2 添加应发布但未跟踪的文件

```bash
# WinUI 前端源码（最重要！）
git add UnrealDbgNative/

# 文档和测试
git add docs/
git add tests/

# 根目录批处理
git add "启动GUI.bat" "构建并验证GUI.bat"

# 新增源文件
git add Common/Ring0/SymbolicAccess/Utils/LegacyLog.h
git add Common/Ring0/SymbolicAccessKM.NOTICE.txt
git add Common/Shared/WindowsBuildSupport.h
git add Common/VMProtect/VMProtectSDK_stub.cpp
git add VT_Driver/Log.cpp
```

### 5.3 更新 .gitignore

按第四节建议补充缺失规则后：
```bash
git add .gitignore
```

### 5.4 提交

```bash
git commit -m "清理IDE缓存，添加WinUI源码和文档，完善.gitignore"
```

---

## 六、F3 目录专项分析

### 6.1 为什么 F3 绝对不能发布

1. **编译产物**：F3 目录内的 `.sys`/`.dll`/`.exe` 是项目的编译输出，发布到公开仓库等于泄露完整可执行程序
2. **加密版权数据**：`F3copyright.db` 是版权保护数据库，`UnrealDbg.aes` 是 AES 加密文件，两者均含敏感信息
3. **内部文档**：`修复or使用or各系统测试文档/` 包含内部测试和修复文档
4. **配置泄露**：`DebuggerList.ini` 暴露调试器路径配置
5. **VMProtect 保护**：`VMProtectSDK64.dll` 是 VMProtect 的 SDK 运行时，受商业许可约束

### 6.2 F3 当前安全状态

| 检查项 | 状态 |
|--------|------|
| Git 跟踪 | ✅ 未跟踪（0 个文件被跟踪） |
| .gitignore 排除 | ✅ `**/x64/` 规则覆盖 |
| 物理完整性 | ✅ 清理操作未触碰 |
| 上传风险 | ✅ 不会随 git push 推送 |

### 6.3 F3 目录保护规则

- **禁止删除** — 不论何种理由
- **禁止修改** — 不改动任何文件
- **禁止上传** — 不推送到 Git 或任何远程仓库
- **禁止外传** — 不复制到项目外、不分享链接、不通过任何渠道传出

---

## 七、待定项（需用户决策）

| 文件 | 当前状态 | 问题描述 | 建议 |
|------|---------|---------|------|
| `D-encryptiondll/D-encryption/copyright.db` | 已跟踪 | 加密的版权数据库二进制文件，非源码 | 移除跟踪，加入 .gitignore |
| `UnrealDbg/UnrealDbg.res` 等 4个 `.res` | 已跟踪 | Delphi 编译的资源文件（非源码） | 移除跟踪（可从 .pas/.dfm 重新生成） |
| `VT调试器 GUI.lnk` | 未跟踪 | 用户桌面快捷方式 | 加入 .gitignore |
| `UnrealDbg/Network/KeyVerification.pas` | 已跟踪 | 密钥验证源码（当前函数体为空桩，无敏感数据） | 可发布（已审查，无硬编码密钥） |
