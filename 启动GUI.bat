@echo off
rem ============================================================
rem  VT 调试器本地版：启动 GUI（原生 WinUI 3 前端）
rem  目标产物：x64\WinUI\Release\UnrealDbgNativeWinUI.exe
rem  逻辑：exe 缺失或源码比 exe 新 -> 自动用 MSBuild 重建，
rem        构建失败则提示并暂停，构建成功后再启动；
rem        始终以 exe 所在目录为工作目录（bin\、Config\ 可解析）。
rem  说明：时间戳比较按本机区域设置的日期字符串序进行，
rem        若系统使用 月/日/年 格式，判断可能失准（仅多构建一次，无副作用）。
rem ============================================================
setlocal enabledelayedexpansion

set "ROOT=%~dp0"
set "SRCDIR=%ROOT%UnrealDbgNative"
set "PROJ=%SRCDIR%\UnrealDbgNativeWinUI.vcxproj"
set "OUTDIR=%ROOT%x64\WinUI\Release"
set "GUI=%OUTDIR%\UnrealDbgNativeWinUI.exe"

rem ---------- 1. 判断是否需要重建 ----------
set "NEED_BUILD=0"

if not exist "%GUI%" (
    set "NEED_BUILD=1"
    echo [信息] 未找到 GUI 产物，将先执行构建。
) else (
    for %%A in ("%GUI%") do set "EXETS=%%~tA"
    for %%F in ("%SRCDIR%\*.cpp" "%SRCDIR%\*.h" "%SRCDIR%\*.vcxproj" "%SRCDIR%\*.manifest") do (
        if "%%~tF" GTR "!EXETS!" set "NEED_BUILD=1"
    )
    if "!NEED_BUILD!"=="1" (
        echo [信息] 检测到源码比 exe 新 ^(exe: !EXETS!^)，将先执行构建。
    ) else (
        echo [信息] 源码未变更，直接启动现有产物。
    )
)

rem ---------- 2. 需要时定位 MSBuild 并重建 ----------
if "!NEED_BUILD!"=="1" (
    set "MSBUILD="

    rem 先查 PATH
    where msbuild >nul 2>nul
    if not errorlevel 1 (
        for /f "delims=" %%i in ('where msbuild 2^>nul') do (
            if not defined MSBUILD set "MSBUILD=%%i"
        )
    )

    rem 兜底：用 vswhere 定位 Visual Studio 自带的 MSBuild
    if not defined MSBUILD (
        set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
        if exist "!VSWHERE!" (
            for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
                if not defined MSBUILD set "MSBUILD=%%i"
            )
        )
    )

    if not defined MSBUILD (
        echo.
        echo [错误] 未找到 MSBuild，无法自动构建。
        echo        请确认已安装 Visual Studio 2019/2022 及“使用 C++ 的桌面开发”工作负载。
        echo        若 WinUI 依赖包尚未还原，请先运行 RestoreWinUiPackages.ps1 后重试。
        echo.
        pause
        exit /b 1
    )

    echo [1/2] 正在重建 Release x64 ...
    pushd "%ROOT%"
    "%MSBUILD%" "%PROJ%" /p:Configuration=Release /p:Platform=x64 /m /v:m
    set "RC=!ERRORLEVEL!"
    popd

    if not "!RC!"=="0" (
        echo.
        echo [失败] 构建报错 ^(退出码 !RC!^)，未启动 GUI。
        echo        请将上方编译错误信息反馈，修复后重新双击本脚本。
        echo.
        pause
        exit /b !RC!
    )

    if not exist "%GUI%" (
        echo.
        echo [失败] 构建已返回成功，但仍未生成目标文件：
        echo        %GUI%
        echo.
        pause
        exit /b 1
    )

    echo [2/2] 构建完成。
)

rem ---------- 3. 以 exe 所在目录为工作目录启动 ----------
cd /d "%OUTDIR%"
start "" "%GUI%"
exit /b 0
