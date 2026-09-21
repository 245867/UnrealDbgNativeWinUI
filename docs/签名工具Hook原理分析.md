# 《亚洲诚信数字签名工具修改版》技术原理深入分析

> 来源：吾爱破解论坛帖子（结帖自 https://www.52pojie.cn/thread-1027420-1-1.html）
> 作者：JemmyloveJenny（本项目 Hook 机制参考资料）
> 更新日期：2019-09-22

---

## 1. 一句话概括

本项目编译产物 `HookSigntool.dll`，利用微软 **Detours** 库 **Hook 签名工具的 6 个函数调用**，实现：
**不需要修改系统时间，就能用已过期的（或伪造的）代码签名证书完成可信的数字签名，甚至生成能在任意 Windows 版本加载的驱动签名。**

它兼容的签名工具有：亚洲诚信数字签名工具（`DSignTool.exe`）、天威诚信代码签名证书助手、沃通代码签名工具、环玺信息数字签名工具等。

---

## 2. 底层原理

### 2.1 为什么能"伪造时间"

Windows 数字签名验证链由两部分组成：

1. **代码签名证书本身** —— 验证签发者链（Chain）与有效期（`CertVerifyTimeValidity`）。
2. **时间戳签名** —— 用一个受信时间戳服务器（TSA, RFC3161 / Authenticode）确认"签名发生在证书有效期内"。

关键漏洞：**证书是否过期，最终取决于"签名时刻"是否落在证书有效期窗口内。**
如果：
- 让工具误以为证书有效（Hook `CertVerifyTimeValidity`）；同时
- 用一个**自建、可伪造任意时间**的 TSA 给签名打时间戳，

那么整条链对 Windows 来说"自洽"——签名时间提示"当时证书有效"，于是验证通过。

### 2.2 Hook 的 6 个函数

| # | 函数 | 所在 DLL | 篡改行为 |
|---|------|---------|---------|
| 1 | `CertVerifyTimeValidity` | `crypt32.dll` | **返回值强制改为 0**，让签名工具误以为所有证书都在有效期内（不用改系统时间） |
| 2 | `SignerSign` | `mssign32.dll` | 传入的时间戳地址 `pwszHttpTimeStamp` **改为自建时间戳服务器地址** |
| 3 | `SignerTimeStamp` | `mssign32.dll` | 同上 |
| 4 | `SignerTimeStampEx2` | `mssign32.dll` | 同上 |
| 5 | `SignerTimeStampEx3` | `mssign32.dll` | 同上（**注意：此函数 Windows 7 上不存在**，Hook 时需处理） |
| 6 | `GetLocalTime` | `kernel32.dll` | 返回值按配置修改（仅影响证书管理界面颜色/到期天数，对签名本身无影响） |

**技术要点**：
- Detours 通过改写目标函数入口（前几条指令被替换成跳转到 trampoline / detour 函数）实现运行时 API 拦截，属于 **user-mode inline hook**。
- 被 Hook 的 4 个 `Signer*` 函数覆盖了新旧 API，保证工具无论调用哪个时间戳入口都能被拦截。

### 2.3 自建时间戳服务器

- 域名：`timestamp.pki.jemmylovejenny.tk`（`timestamp` 可简写为 `tsa`）。
- 根证书由作者自己签发（`JemmyLoveJenny EV Root CA`），时间戳含 **SHA1 与 SHA256 两条证书链**。
- **伪造任意时间**：服务器地址上拼接 UTC 时间路径即可，例如：

```
http://timer.tk ... /SHA1/2011-04-01T00:00:00
http://tsa.pki.jemmylovejenny.tk/SHA256/2019-03-10T02:25:34
```

- **协议无关**：Authenticode 与 RFC3161 地址相同，服务器根据请求自动识别处理。
- **信任前提**：客户端（要验证的人）必须信任自建根证书 `JemmyloveJenny EV Root CA`（提供 `EVRootCA.reg` 注册表导入）。

---

## 3. 使用方式

### 3.1 时间表示法（SimpleDateFormat / UTC）

格式 `yyyy-MM-dd'T'HH:mm:ss`，**UTC 时间**。北京时间 = UTC+8，需减 8 小时：

| 北京时间 | 配置/参数中的 UTC |
|---------|------------------|
| 2011-04-01 08:00:00 | `2011-04-01T00:00:00` |
| 2019-03-10 10:25:34 | `2019-03-10T02:25:34` |

### 3.2 配置方式（二选一）

**A. `hook.ini` 文件**（默认同目录；可用 `-config` 指定其他 ini）：

```ini
[Timestamp]
Timestamp=2011-04-01T00:00:00          ; 时间戳签名伪造的默认时间

[Time]
;Year=2011
;Month=4
;Day=1
;Hour=0
;Minute=0
;Second=0                              ; GetLocalTime 返回值（可选，仅影响 UI 颜色）
```

**B. 命令行参数**（向 `DSigntool.exe` 传参）：

```powershell
DSigntool.exe -config hook.ini                  # 指定 ini
DSigntool.exe -config ../another.ini
DSigntool.exe -ts 2011-04-01T00:00:00           # 直接指定时间戳时间
DSigntool.exe -ts 2011-04-01T00:00:00 -config hook.ini   # -ts 优先级更高
```

**优先级**：`-ts` 参数 > `-config` 指定的 ini 中配置的时间。

**C. 快捷方式（lnk）**：在目标路径末尾加 `-ts` 参数，可做多个不同时间戳的快捷方式，免改系统时间。

### 3.3 让签名工具加载 DLL

用 **LordPE** 等 PE 工具修改 `DSignTool.exe` 的**导入表**，新增 `HookSigntool.dll!attach` 即可实现启动时自动注入。

### 3.4 替换内置时间戳地址（自定义时间戳时必做）

用十六进制编辑器把工具内置的**两处**时间戳地址**整串替换**（包括 `http://` 部分）为特殊标记，剩余长度用 `0x00` 填充：

| 用途 | 替换为 |
|------|--------|
| SHA1 时间戳地址 | `{CustomTimestampMarker-SHA1}` |
| SHA256 时间戳地址 | `{CustomTimestampMarker-SHA256}` |

- 标记**与时间戳协议无关**（Authenticode / RFC3161 相同）。
- DLL 会对该标记做**特异性识别**，自动改写成当前设定的时间戳服务器地址。

---

## 4. 绕过证书有效期

只要工具加载了 DLL 即可生效，无需额外操作（`CertVerifyTimeValidity` 已被改返回 0）。

---

## 5. 驱动签名（重点）

Quoting 微软内核驱动签名策略文档：

> 任何带有 **`Microsoft Code Verification Root`** 交叉签名、且**颁发日期在 2015-07-29 之前**的代码签名证书，
> 配合**伪造的时间戳签名**，可生成一个在**任意 Windows 版本下都有效**的驱动签名。

因此：**采用（泄露的）旧版交叉签名证书 + 信任自建时间戳根证书**，就可以在 **WinXP ~ Win10（含 SecureBoot 启用）** 任意版本成功加载无签名驱动。

**一条重要的现实线索**：帖子 2# 楼"哎，好不容易找到证书"、及后续"万事具备，只差证书"——**瓶颈往往在"找到一张过期但带微软交叉签名的旧证书"**，工具只是辅助。

### 5.1 配合微软 signtool

先签好名（不带时间戳），假设有 N 个签名：

```powershell
# 第一个签名打时间戳
signtool timestamp /t "<URL>" <filename>

# 后续任意个签名打时间戳（index 从 1 递增）
signtool timestamp /tp <index> /tr "<URL>" <filename>
```

示例：

```powershell
signtool timestamp /t "http://tsa.pki.jemmylovejenny.tk/SHA1/2011-04-01T00:00:00" test.exe
signtool timestamp /tp 1 /tr "http://tsa.pki.jemmylovejenny.tk/SHA256/2011-04-01T00:00:00" test.exe
```

---

## 6. 编译环境

- **IDE**：Visual Studio 2019（生成工具 v142）
- **关键依赖**：微软 Detours 库（https://www.microsoft.com/en-us/download/details.aspx?id=52586）
- **编译步骤**：
  1. 解压 Detours。
  2. 打开 `x86 Native Tools Command Prompt for VS 2019`，进入 Detours 目录执行 `nmake`，编出 x86 的 lib（x64 无需编译）。
  3. 把 Detours 的 `include`、`lib.X86` 下文件复制到 VS 的 `include` / `lib\x86` 子目录，完成安装。
  4. 打开 `HookSigntool.sln`，Release 配置直接编译。

---

## 7. 更新历史与版本差异（2019-09-22）

相比旧版（thread-877849）：
1. 可根据配置文件或命令行参数修改**伪造的签名时间**（旧版时间戳地址写死）。
2. 修改 Hook 函数，**新增 2 个函数**（覆盖 `SignerTimeStampEx3` 等）。
3. 修复旧版**内存溢出漏洞**。
4. 重构软件架构。

---

## 8. 附件/资源清单

| 资源 | 说明 |
|------|------|
| `DSigntool.7z`（修改版，1.1MB） | 替换原版目录下文件即可 |
| `DSigntool` GUI 时间戳工具.zip | 第三方 GUI 封装 |
| 原版下载 | https://www.trustasia.com/sign-tools |
| 修改版源码 | https://github.com/JemmyLoveJenny/HookSigntool |
| `EVRootCA.reg` | 导入信任自建时间戳根证书（"时间验证失效"时的解决办法） |
| 自建 PKI | https://pki.jemmylovejenny.tk/ |

---

## 9. 常见问题（帖子回复中）

- **"时间验证失效了~"** → 需要**信任时间戳根证书**，导入 `EVRootCA.reg`。
- **504 错误** → 2019 年曾因服务器 SQL 句柄耗尽 504，修复后已恢复。
- **功能建议**：可自动判断证书真实有效期，把时间自动改到有效范围内（等价于改系统时间）。

---

## 10. 技术拆解总结（本项目可借鉴要点）

1. **Detours user-mode inline hook 的典型用法**：Hook 系统/签名 API，改返回值/参数来篡改验证逻辑。
2. **证书有效性验证的两个独立环节**（证书链 + 时间戳）可被分离绕过——只要 TSA 受信即可。
3. **自建根 CA + 自建 TSA + 自签名证书**，构造一条完全可控、可伪造时间的签名链。
4. **PE 导入表注入**（LordPE 加 `attach`）是实现"进程启动即注入 DLL"的朴素手段，不依赖驱动，无需提权。
5. **逐字串替换 + 标记占位**（`{CustomTimestampMarker-*}` + `0x00` 填充）是纯手工 patch 二进制的一种实用技巧。