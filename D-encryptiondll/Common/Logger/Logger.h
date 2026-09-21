#pragma once

#ifndef _LOGGER_H
#define _LOGGER_H

typedef void(__stdcall* PFN_PRINTLOG)(TCHAR* sText);

extern PFN_PRINTLOG pfnPrintLog;


class Logger {
public:
    Logger(const std::string& filename) : filename(filename)
    {
        logFile.open(filename, std::ios::out | std::ios::app);
        if (!logFile.is_open())
        {
            // DLL 可能在宿主尚未创建窗口时初始化；不要弹出阻塞式窗口，
            // 统一写入调试输出，避免旧版 ANSI 弹窗造成乱码。
            ::OutputDebugStringW(L"[D-encryption] 无法打开日志文件，请确认程序目录可写。\n");
        }
    }

    Logger()
    {

    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    //void _outDebug(TCHAR* sText)
    //{
    //    // 线程同步：使用互斥锁保护临界区
    //    std::lock_guard<std::mutex> lock(mutex);
    //    TCHAR szBuf[1024] = { 0 };

    //    if (m_modName.empty())
    //    {
    //        m_modName = Common::stringToWideString(FileSystem::GetSelfModuleName());
    //    }

    //    wcscat(szBuf, _T("["));
    //    wcscat(szBuf, m_modName.c_str());
    //    wcscat(szBuf, _T("] "));
    //    wcscat(szBuf, sText);
    //    //logger.Log(Common::wideStringToString(sText));
    //    OutputDebugString(szBuf);
    //    OutputDebugString(_T("\n"));
    //}

    void _outDebug(TCHAR* sText)
    {
        // 线程同步：使用互斥锁保护临界区
        std::lock_guard<std::mutex> lock(mutex);
        TCHAR szBuf[1024] = { 0 };

        if (m_modName.empty())
        {
            m_modName = Common::stringToWideString(FileSystem::GetSelfModuleName());
        }

        wcscat(szBuf, _T("["));
        wcscat(szBuf, m_modName.c_str());
        wcscat(szBuf, _T("] "));
        wcscat(szBuf, sText);

        try
        {
            if (!pfnPrintLog)
            {
                pfnPrintLog = (PFN_PRINTLOG)GetProcAddress(GetModuleHandle(NULL), "PrintLog");
            }
            if (pfnPrintLog == nullptr)
            {
                // 新版 C++ 前端必须导出 PrintLog。缺少该入口是宿主 ABI
                // 错误，不能静默吞掉：记录到 DLL 日志、调试输出并保留
                // 错误码；同时不再调用旧版 ANSI MessageBox。
                SetLastError(ERROR_PROC_NOT_FOUND);
                const char* diagnostic = "[错误] 宿主未导出 PrintLog；日志回调失败。原因：宿主与 DLL 接口版本不匹配。解决方案：使用配套的 UnrealDbgNative.exe，确认导出函数 PrintLog 存在。";
                if (logFile.is_open())
                {
                    logFile << diagnostic << std::endl;
                    logFile.flush();
                }
                ::OutputDebugStringW(L"[D-encryption] 宿主未导出 PrintLog；请使用配套前端并检查导出表。\n");
                ::OutputDebugString(szBuf);
                ::OutputDebugString(_T("\n"));
                return;
            }
            pfnPrintLog(szBuf);
        }
        catch (...)
        {
            SetLastError(ERROR_UNHANDLED_EXCEPTION);
            const char* diagnostic = "[错误] 调用 PrintLog 时发生未处理异常；原因：宿主回调 ABI 或运行状态异常。解决方案：确认前端和 DLL 位数、版本一致，并查看日志后重启。";
            if (logFile.is_open())
            {
                logFile << diagnostic << std::endl;
                logFile.flush();
            }
            ::OutputDebugStringW(L"[D-encryption] 调用 PrintLog 时发生未处理异常；请检查前端/DLL ABI。\n");
            ::OutputDebugString(szBuf);
            ::OutputDebugString(_T("\n"));
        }
    }

    int outDebug(const TCHAR* _Format, ...)
    {
        __try
        {
            int iRet;
            va_list list;
            TCHAR szBuf[1024] = { 0 };
            va_start(list, _Format);
            iRet = _vsnwprintf_s(szBuf, _countof(szBuf), _TRUNCATE, _Format, list);
            _outDebug(szBuf);
            va_end(list);
            return iRet;
        }
        __except (1)
        {
            ::OutputDebugStringW(L"[D-encryption] 日志缓冲区溢出，已停止本次日志调用。\n");
        }
        return 0;
    }

    ////英文日期
    //void Log(const std::string& message) {
    //    if (logFile.is_open()) {
    //        std::time_t now = std::time(nullptr);
    //        std::string timestamp = std::ctime(&now);
    //        timestamp.resize(timestamp.length() - 1);  // Remove trailing newline

    //        logFile << "[" << timestamp << "] " << message << std::endl;
    //        logFile.flush();
    //    }
    //}

    void Log(const char* format, ...) {
        // 线程同步：使用互斥锁保护临界区
        std::lock_guard<std::mutex> lock(mutex);

        if (logFile.is_open()) {
            std::time_t now = std::time(nullptr);
            std::tm* localTime = std::localtime(&now);

            char buffer[100];
            std::strftime(buffer, sizeof(buffer), "%Y年%m月%d日 %H:%M:%S", localTime);

            //std::string logMessage = "[" + std::string(buffer) + "] " + format;

            std::ostringstream oss;
            oss << "[" << buffer << "] ";

            va_list args;
            va_start(args, format);
            char message[1024] = { 0 };
            vsnprintf(message, sizeof(message), format, args);
            va_end(args);

            oss << message;

            std::string logMessage = oss.str();
            logFile << logMessage << std::endl;
            logFile.flush();  //将缓冲区中的数据立即刷新到磁盘，确保数据写入文件。
            //OutputDebugStringA(logMessage.c_str());
        }
    }

private:
    std::ofstream logFile;
    std::string filename;
    std::wstring m_modName;
    std::mutex mutex; // 互斥锁
};

#endif // !_LOGGER_H
