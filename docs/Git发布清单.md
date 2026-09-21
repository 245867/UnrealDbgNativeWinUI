# Git 发布清单 — 什么可以发布，什么不能

> 审查日期：2026-09-21
> 审查范围：`D:\Backup\Documents\VT调试器本地版` 全部内容
> Git 仓库当前跟踪文件数：1008

> **📌 阅读说明：本文档包含两个阶段的内容**
> - **第一 ~ 七章**：发布**前**的审查与计划记录（保留原貌，其中"当前状态"指审查当时的状态，部分已执行完毕）
> - **第八章**：发布**后**的实际执行结果与内容泄露审计
>
> 若前后表述冲突，**以第八章为准**。

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
| 受限发布资产（私有发布包） | ❌ 绝对不可以 | 未跟踪（`.gitignore` 的 `**/x64/` 已整体排除） | — |
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
| 许可证 | `LICENSE` | GPLv3 主许可证 + Phnt/ia32-doc 第三方许可证 |
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

### 3.1 ★ 受限发布资产 — 绝对禁止

构建输出目录（`x64/Release/`）下存放着若干**私有发布包与受限资产**，属于本项目最敏感的部分。
本节**不列举其具体名称、路径、体积与内部文件清单** —— 避免文档本身成为信息泄露渠道。

| 项目 | 说明 |
|------|------|
| 位置 | 统一位于构建输出目录 `x64/Release/` 之下 |
| 内容性质 | 完整调试器发布包，含编译产物（DLL/SYS/EXE）、加密数据（`.aes`）、版权保护数据库（`.db`）、运行时配置、内部测试文档等 |
| 禁止原因 | 含编译产物、加密版权数据与专有发布包，公开即等于泄露完整可执行程序 |
| 当前状态 | ✅ 已被 `.gitignore` 的 `**/x64/` 规则整体排除，Git 不跟踪、不推送 |
| 保护约定 | **禁止删除、禁止修改、禁止上传、禁止外传** |

> **关于这些资产的具体名称与清单：本文档不记录，也不应记录在任何会被发布的文件中。**
> 需要时请以本地项目目录（未纳入版本控制）中的实际目录结构为准。

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
| 加密文件 | 受限资产内的 `.aes`（位于 `x64/Release/` 下） | ✅ `**/x64/` 覆盖 |
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

## 六、受限发布资产专项分析

> 为避免本文档自身成为泄露渠道，本节**不记录资产的名称、路径、体积与文件清单**，只说明其性质与保护要求。

### 6.1 为什么受限资产绝对不能发布

1. **编译产物**：资产内的 `.sys` / `.dll` / `.exe` 是项目的编译输出，发布到公开仓库等于泄露完整可执行程序
2. **加密版权数据**：含版权保护数据库（`.db`）与 AES 加密文件（`.aes`），均含敏感信息
3. **内部文档**：含内部测试与修复文档
4. **配置泄露**：含调试器路径等运行时配置
5. **第三方许可约束**：内含商业保护方案的 SDK 运行时，受其许可条款约束

### 6.2 当前安全状态

| 检查项 | 状态 |
|--------|------|
| Git 跟踪 | ✅ 未跟踪（0 个文件被跟踪） |
| `.gitignore` 排除 | ✅ `**/x64/` 规则覆盖 |
| 物理完整性 | ✅ 清理操作未触碰 |
| 上传风险 | ✅ 不会随 `git push` 推送 |
| 文档泄露 | ✅ 本文档已脱敏，不含其名称/路径/体积/文件清单 |

### 6.3 保护规则

- **禁止删除** — 不论何种理由
- **禁止修改** — 不改动任何文件
- **禁止上传** — 不推送到 Git 或任何远程仓库
- **禁止外传** — 不复制到项目外、不分享链接、不通过任何渠道传出
- **禁止在文档中记录** — 名称、路径、体积、文件清单均不得写入任何会被发布的文件

---

## 七、待定项（需用户决策）

> 下表为发布前的待定判断。**处置结果见末列**（已于 2026-09-21 执行完毕，详见第八章）。

| 文件 | 审查时状态 | 问题描述 | 原建议 | 实际处置 |
|------|-----------|---------|--------|---------|
| `D-encryptiondll/D-encryption/copyright.db` | 已跟踪 | 加密的版权数据库二进制文件，非源码 | 移除跟踪，加入 .gitignore | ✅ 已移除跟踪并加入 `.gitignore` |
| `UnrealDbg/UnrealDbg.res` 等 `.res` | 已跟踪 | Delphi 编译的资源文件（非源码） | 移除跟踪（可从 .pas/.dfm 重新生成） | ⏸️ **保留**：这些工具的 `.dpr` 依赖它，保留可让旧工具直接编译 |
| `VT调试器 GUI.lnk` | 未跟踪 | 用户桌面快捷方式 | 加入 .gitignore | ✅ 已加入 `.gitignore` |
| `UnrealDbg/Network/KeyVerification.pas` | 已跟踪 | 密钥验证源码（当前函数体为空桩，无敏感数据） | 可发布（已审查，无硬编码密钥） | ✅ 已发布 |

---

## 八、发布执行结果与发布后审计

发布已完成，远程仓库：**https://github.com/245867/UnrealDbgNativeWinUI**（分支 `main`）。

### 8.1 执行结果

| 项目 | 结果 |
|------|------|
| 远程文件总数 | 770 |
| 敏感扩展名（`.dll/.sys/.exe/.pdb/.lib/.exp/.obj/.aes/.db`） | **0** |
| 受限资产相关文件（文件名层面） | **0** |
| 构建产物 `/x64/` | **0** |
| 从索引移除的文件 | 271（245 `__history` + IDE 缓存 + VS 用户配置 + `copyright.db` + 游离 PDB/日志） |
| 新增跟踪的文件 | 33（`UnrealDbgNative/` 全部 WinUI 3 前端源码 + `docs/` + `tests/` + 批处理） |
| 许可证 | 统一为 GNU GPL v3.0 |

### 8.2 内容泄露审计（对远程全部 770 个文件逐项比对）

| 审计项 | 结果 |
|--------|------|
| 受限资产的名称 / 路径 / 体积（文件名与正文两个层面） | ✅ **0 命中**（首次审计发现 `docs/` 两份文档正文曾记录其名称、路径、体积与内部文件清单，已全部脱敏后重新推送） |
| 本机绝对路径（`C:\Users\...`、`D:\Backup\...`） | 0 命中 |
| 本机计算机名 | 0 命中 |
| 项目原目录名 | 0 命中 |
| 上游原仓库名 | 0 命中 |
| 凭据特征（`ghp_*`、`github_pat_*`、私钥头、`password=`、`api_key=`、`Bearer`） | 0 命中 |

> **重要教训**：文件「不推送」不等于信息「不外传」。
> 第一时间只检查了**文件名**（0 命中），但文档**正文**里仍完整记录了受限资产的名称、路径与体积，
> 这同样属于泄露。发布前必须对**文件内容**一并审计，而不只是文件名。

### 8.3 关于 `Common/Ring0` 重复目录（已确认，非问题）

`D-encryptiondll/Common/Ring0/` 与根目录 `Common/Ring0/` 存在高度重复：

- `Common/Ring0`：191 个文件，5.4 MB
- `D-encryptiondll/Common/Ring0`：174 个文件，5.2 MB
- 其中 **162 个文件字节完全相同**

**结论：这是有意保留的构建隔离副本，不应删除。**
原因：`D-encryptiondll/D-encryption/D-encryption.vcxproj` 通过相对路径 `..\Common` 引用该副本，
若删除将导致 D-encryptiondll 子解决方案无法独立编译。根目录其余工程（`DbgkSysWin10/11`、
`Hook`、`Loader`、`UnrealDbgDll`、`VT_Driver`）则统一引用根目录的 `Common/`。

> 如需在本地保留磁盘文件的同时减小仓库体积，只能改为「子解决方案依赖根目录 Common」的重构，
> 属于工程结构调整，需另行评估，**不建议为省体积而直接删除**。

### 8.4 发布后仍存在的已知项

| 文件 | 状态 | 说明 |
|------|------|------|
| 6 个 Delphi `.res`（`CardRegistration`、`D-encryption`、`SymbolTool`、`UnrealDbg`） | 仍跟踪 | Delphi 编译资源，二进制但无敏感内容；这些工具的 `.dpr` 依赖它，保留可让旧工具直接编译 |
| `Common/Ring0/ia32-doc/**/*.disabled`（8 个） | 仍跟踪 | 上游 ia32-doc 自带的禁用清单，属第三方内容，保留原样 |

### 8.5 最终 `.gitignore` 规则集（发布后生效）

以下规则已在实际发布的 `.gitignore` 中生效，是防止后续误提交的最后一道防线：

| 规则 | 覆盖对象 |
|------|---------|
| `**/x64/` | ★ **构建输出与受限发布资产整体**（最关键的一条） |
| `*.dll` `*.exe` `*.sys` `*.cat` `*.pdb` `*.lib` `*.exp` `*.obj` | 编译产物、驱动、符号 |
| `*.cer` `*.aes` `copyright.db` | 证书、加密数据、版权数据库 |
| `*.log` `/_artifacts/` | 运行日志、临时工件 |
| `**/__history/` `*.~*` `*.identcache` `*.dproj.local` `*.groupproj.local` | Delphi 备份与 IDE 缓存 |
| `*.vcxproj.user` `*.aps` `*.suo` `.vs/` `ipch/` | Visual Studio 用户配置与 IDE 缓存 |
| `*.rar` `*.zip` `*.7z` | **归档文件** —— 防止私有发布包被复制到 `x64/` 之外后误提交 |
| `*.tlog` `*.lastbuildstate` `*.recipe` `*.iobj` `*.ipdb` | MSVC 构建中间文件（`x64/` 之外的情况） |
| `*.lnk` | 桌面快捷方式（含机器相关内容） |

**验证方式**（应输出为空）：

```bash
# 已跟踪但会被 ignore 规则匹配的文件（正常应为空）
git ls-files -i -c --exclude-standard
```

> ⚠️ 维护提醒：**不要**为 `x64/` 下的任何文件添加 `!` 例外规则。
> 该目录整体属于受限范围，任何"放行个别文件"的写法都会重新打开泄露通道。
