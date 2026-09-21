#include <iostream>
#include <Windows.h>
#include <string>
#include <codecvt>
#include <random>
#include <tuple>
#include <TlHelp32.h>
#include <vector>
#include <psapi.h>
#include <intrin.h>
#include <array>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cstdlib>
#include <cstring>
#include "Common.h"

namespace Common
{

	HANDLE hMutex;// 防多开
	bool isIntel = false;
	bool isAMD = false;
	std::mutex mutex; // 互斥锁

	namespace
	{
		struct ErrorDetails
		{
			const wchar_t* reason;
			const wchar_t* solution;
		};

		ErrorDetails ExplainWin32Error(const DWORD error)
		{
			switch (error)
			{
			case ERROR_INVALID_IMAGE_HASH:
				return { L"Windows 拒绝加载驱动映像：驱动签名无效、测试证书不受信任，或系统未启用测试签名。",
					L"开发测试版请以管理员身份把 Certificates 目录中的 .cer 导入本地计算机的“受信任的根证书颁发机构”和“受信任的发布者”，确认测试签名设置后重启；正式环境请使用受 Microsoft 信任的正式签名。" };
			case ERROR_GEN_FAILURE:
				return { L"系统返回通用失败码 31，真实根因通常在服务退出码、设备状态或驱动日志中。",
					L"不要只依据 31 判断原因；检查 VT_Driver 服务状态、Log\\log.ini 和 Log\\UnrealDbgDll.log，并以其中更具体的错误码为准。" };
			case ERROR_ACCESS_DENIED:
				return { L"访问被拒绝，当前进程权限不足或对象被安全策略保护。",
					L"以管理员身份运行，检查文件/服务权限及 Windows 安全软件拦截记录。" };
			case ERROR_FILE_NOT_FOUND:
				return { L"找不到指定文件，文件可能缺失、改名或部署目录不正确。",
					L"确认文件路径、文件名和位数匹配，并将依赖文件部署到程序目录后重试。" };
			case ERROR_PATH_NOT_FOUND:
				return { L"找不到指定目录或路径中的某一级目录。",
					L"检查程序、配置和日志目录，创建缺失目录后重试。" };
			case ERROR_MOD_NOT_FOUND:
			case ERROR_DLL_NOT_FOUND:
				return { L"找不到所需 DLL 或其依赖项，可能是文件缺失或 32/64 位不匹配。",
					L"确认 UnrealDbgDll.dll、D-encryption.dll 及运行库均已部署，且全部与当前进程使用相同位数。" };
			case ERROR_PROC_NOT_FOUND:
				return { L"DLL 中不存在调用方要求的导出函数，组件版本不匹配。",
					L"部署与主程序配套的 DLL，确认导出名称和架构一致。" };
			case ERROR_INVALID_PARAMETER:
				return { L"传入参数为空、越界或格式不符合 API 要求。",
					L"检查输入文件、密钥、缓冲区和长度；修正参数后重试。" };
			case ERROR_INVALID_DATA:
				return { L"输入数据、配置或密文格式无效，可能已损坏或版本不匹配。",
					L"确认文件未被截断，使用匹配的密钥和版本重新生成数据后重试。" };
			case ERROR_NOT_ENOUGH_MEMORY:
				return { L"系统或进程无法分配足够内存。",
					L"关闭无关程序、检查内存占用并重启本程序后重试。" };
			case ERROR_BUFFER_OVERFLOW:
			case ERROR_INSUFFICIENT_BUFFER:
				return { L"输出缓冲区不足，无法容纳完整结果。",
					L"增大调用方缓冲区，并按接口返回的所需长度重新调用。" };
			default:
				return { L"Windows 返回了未在加密模块诊断表中收录的错误。",
					L"保留错误码、系统原文和触发位置，检查文件、权限、密钥及组件版本，并查看前端日志。" };
			}
		}

		std::wstring GetSystemErrorText(const DWORD error)
		{
			LPWSTR buffer = nullptr;
			const DWORD length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
			if (length == 0 || buffer == nullptr)
			{
				return L"系统未提供错误文本";
			}
			std::wstring result(buffer, length);
			LocalFree(buffer);
			while (!result.empty() && (result.back() == L'\r' || result.back() == L'\n' || result.back() == L' '))
			{
				result.pop_back();
			}
			return result.empty() ? L"系统未提供错误文本" : result;
		}

		bool TryExtractErrorCode(const std::string& text, DWORD& result)
		{
			const char* markers[] = { "error:", "error=", "error code:", "error code=", "错误码：", "错误码:" };
			for (const char* marker : markers)
			{
				const std::string::size_type position = text.find(marker);
				if (position == std::string::npos)
				{
					continue;
				}
				std::string::size_type number = position + std::strlen(marker);
				while (number < text.size() && (text[number] == ' ' || text[number] == '\t'))
				{
					++number;
				}
				char* end = nullptr;
				const unsigned long parsed = std::strtoul(text.c_str() + number, &end, 10);
				if (end != text.c_str() + number && parsed <= (std::numeric_limits<DWORD>::max)())
				{
					result = static_cast<DWORD>(parsed);
					return true;
				}
			}
			return false;
		}
	}

	//string转wstring
	std::wstring stringToWideString(const std::string& narrowStr)
	{
		// 获取宽字符字符串的长度（包括空终止符）
		int wideStrLength = MultiByteToWideChar(CP_UTF8, 0, narrowStr.c_str(), -1, nullptr, 0);

		// 分配内存来存储宽字符字符串
		wchar_t* wideStr = new wchar_t[wideStrLength];

		// 将窄字符转换为宽字符
		MultiByteToWideChar(CP_UTF8, 0, narrowStr.c_str(), -1, wideStr, wideStrLength);

		// 创建 std::wstring 对象
		std::wstring result(wideStr);

		// 释放内存
		delete[] wideStr;

		return result;
	}

	//wstring转string
	//注意: 在Windows下将utf16转utf8的std::string是无法正常显示的
	std::string wideStringToString(const std::wstring& wideStr)
	{
		int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string str(bufferSize - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &str[0], bufferSize - 1, nullptr, nullptr);
		return str;
	}

	//wstring转本地string
	//注意: 本地ansi可以显示中文，但请不要再网络内容传输中使用它，因为不同计算机本地代码页不相同.
	std::string wideStringToString2(const std::wstring& wideStr)
	{
		int bufferSize = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string str(bufferSize - 1, 0);
		WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, &str[0], bufferSize - 1, nullptr, nullptr);
		return str;
	}

	//wchar_t*转string
	std::string wcharToString(const wchar_t* str)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(str);
	}

	//wchar_t*转wstring
	std::wstring wcharToWideString(const wchar_t* wcharStr)
	{
		// 使用构造函数将 wchar_t* 转换为 std::wstring
		std::wstring wideStr(wcharStr);

		return wideStr;
	}

	//char*转wchar_t*
	std::wstring ConvertCharToWchar(const char* charStr)
	{
		if (charStr == nullptr)
		{
			return L"";
		}
		const size_t sourceLength = strlen(charStr);
		if (sourceLength >= static_cast<size_t>(INT_MAX))
		{
			return L"";
		}
		const int charStrLength = static_cast<int>(sourceLength + 1); // char 字符串的长度（包括 null 终止符）

		// 计算 wchar_t 字符串所需的缓冲区大小
		const int wcharStrSize = MultiByteToWideChar(CP_UTF8, 0, charStr, charStrLength, nullptr, 0);
		if (wcharStrSize <= 0)
		{
			return L"";
		}

		std::wstring result(static_cast<size_t>(wcharStrSize), L'\0');
		if (MultiByteToWideChar(CP_UTF8, 0, charStr, charStrLength, &result[0], wcharStrSize) == 0)
		{
			return L"";
		}
		result.pop_back(); // 移除 MultiByteToWideChar 写入的 null 终止符
		return result;
	}

	//gbk转utf8
	std::string GbkToUTF8(const std::string& gbkString)
	{
		int bufferSize = MultiByteToWideChar(CP_ACP, 0, gbkString.c_str(), -1, nullptr, 0);
		std::wstring wideString(bufferSize - 1, L'\0');
		MultiByteToWideChar(CP_ACP, 0, gbkString.c_str(), -1, &wideString[0], bufferSize - 1);

		bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string utf8String(bufferSize - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, &utf8String[0], bufferSize - 1, nullptr, nullptr);

		return utf8String;
	}

	//gbk转utf8
	//std::string GbkToUTF8(const std::string& gbkString)
	//{
	//	int bufferSize = MultiByteToWideChar(CP_ACP, 0, gbkString.c_str(), -1, nullptr, 0);
	//	std::wstring wideString(bufferSize, L'\0');
	//	MultiByteToWideChar(CP_ACP, 0, gbkString.c_str(), -1, &wideString[0], bufferSize);

	//	bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
	//	std::string utf8String(bufferSize, '\0');
	//	WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, &utf8String[0], bufferSize, nullptr, nullptr);

	//	return utf8String;
	//}

	// 将 utf8 编码的字符串转换为 GBK 编码
	std::string utf8ToGbk(const std::string& utf8String)
	{
		int bufferSize = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, nullptr, 0);
		if (bufferSize == 0)
		{
			// 转换失败，可以根据实际情况进行错误处理
			return "";
		}

		std::wstring wideString(bufferSize, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, &wideString[0], bufferSize);

		bufferSize = WideCharToMultiByte(CP_ACP, 0, wideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (bufferSize == 0)
		{
			// 转换失败，可以根据实际情况进行错误处理
			return "";
		}

		std::string gbkString(bufferSize, '\0');
		WideCharToMultiByte(CP_ACP, 0, wideString.c_str(), -1, &gbkString[0], bufferSize, nullptr, nullptr);

		return gbkString;
	}

	// 将 utf8 编码的字符串转换为 Unicode 编码
	std::wstring utf8ToUnicode(const std::string& utf8String)
	{
		int bufferSize = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, nullptr, 0);
		std::wstring unicodeString(bufferSize, 0);
		MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, &unicodeString[0], bufferSize);
		return unicodeString;
	}

	//本地代码页转std::wstring
	std::wstring ConvertLocalCodePageToWideString(const std::string& str)
	{
		int wideStrLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
		if (wideStrLen == 0)
		{
			// 转换失败，可以根据实际情况处理错误
			return L"";
		}

		std::wstring wideStr(wideStrLen, L'\0');
		if (MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wideStr[0], wideStrLen) == 0)
		{
			// 转换失败，可以根据实际情况处理错误
			return L"";
		}

		// 去掉末尾的空字符
		wideStr.resize(wideStrLen - 1);

		return wideStr;
	}

	//本地代码页转std::string
	std::string LocalCodePageToUtf8(const std::string& localString)
	{
		int wideCharLength = MultiByteToWideChar(CP_ACP, 0, localString.c_str(), -1, nullptr, 0);
		if (wideCharLength == 0) {
			// 转换失败
			return "";
		}

		std::wstring wideString(wideCharLength, L'\0');
		if (MultiByteToWideChar(CP_ACP, 0, localString.c_str(), -1, &wideString[0], wideCharLength) == 0) {
			// 转换失败
			return "";
		}

		int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (utf8Length == 0) {
			// 转换失败
			return "";
		}

		std::string utf8String(utf8Length, '\0');
		if (WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, &utf8String[0], utf8Length, nullptr, nullptr) == 0) {
			// 转换失败
			return "";
		}

		return utf8String;
	}

	//Unicode转Utf8
	std::string UnicodeToUtf8(const std::wstring& unicodeString)
	{
		int utf8Length = WideCharToMultiByte(CP_UTF8, 0, unicodeString.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (utf8Length == 0) {
			// 转换失败
			return "";
		}

		std::string utf8String(utf8Length, '\0');
		if (WideCharToMultiByte(CP_UTF8, 0, unicodeString.c_str(), -1, &utf8String[0], utf8Length, nullptr, nullptr) == 0) {
			// 转换失败
			return "";
		}

		return utf8String;
	}

	//生成16位随机字符串
	std::string generateRandomString()
	{
		const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		const int length = 16;

		std::random_device rd;
		std::mt19937 generator(rd());
		std::uniform_int_distribution<int> distribution(0, static_cast<int>(characters.length()) - 1);

		std::string randomString;

		for (int i = 0; i < length; ++i) {
			randomString += characters[distribution(generator)];
		}

		return randomString;
	}

	//字符串截取
	std::string truncateString(const std::string& input, int length)
	{
		if (length >= input.length())
		{
			return input;
		}
		else
		{
			return input.substr(0, length);
		}
	}

	//截取字符串 和剩余字符串
	std::tuple<std::string, std::string> truncateString2(const std::string& input, int length)
	{
		if (length >= input.length())
		{
			return std::make_tuple(input, "");
		}
		else
		{
			return std::make_tuple(input.substr(0, length), input.substr(length));
		}
	}

	//将string转小写
	std::string ToLowerWindows(const std::string& str)
	{
		std::string lowerStr(str);
		CharLowerBuffA(&lowerStr[0], static_cast<DWORD>(lowerStr.size()));

		return lowerStr;
	}

	//将wstring转小写
	std::wstring ToLowerWindows(const std::wstring& str)
	{
		std::wstring lowerStr(str);
		CharLowerBuffW(&lowerStr[0], static_cast<DWORD>(lowerStr.size()));

		return lowerStr;
	}

	//枚举进程
	std::vector<ProcessInfo> EnumerateProcesses()
	{
		std::vector<ProcessInfo> processes;

		// 获取系统中所有进程的快照
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE)
		{
			// 返回空容器
			return processes;
		}

		PROCESSENTRY32W processEntry = { sizeof(PROCESSENTRY32W) };

		// 枚举进程快照中的进程信息
		if (Process32First(hSnapshot, &processEntry))
		{
			do
			{
				ProcessInfo process;
				process.processId = processEntry.th32ProcessID;
				process.processName = processEntry.szExeFile;

				// 打开进程
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processEntry.th32ProcessID);
				if (hProcess != nullptr)
				{
					TCHAR modulePath[MAX_PATH] = { 0 };
					if (GetModuleFileNameEx(hProcess, NULL, modulePath, MAX_PATH))
					{
						process.FullPath = modulePath;
					}
					CloseHandle(hProcess);					
				}
				processes.push_back(process);
			} while (Process32Next(hSnapshot, &processEntry));
		}

		// 关闭进程快照句柄
		CloseHandle(hSnapshot);

		return processes;
	}

	//检查目标进程是否正在运行
	BOOL IsProcessRunning(const std::wstring& processName)
	{
		BOOL boRet = FALSE;
		PROCESSENTRY32W entry;
		entry.dwSize = sizeof(PROCESSENTRY32W);

		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE)
		{
			if (Process32FirstW(hSnapshot, &entry))
			{
				do
				{
					std::wstring currentProcessName = Common::ToLowerWindows(entry.szExeFile);
					if (currentProcessName.find(Common::ToLowerWindows(processName)) != std::wstring::npos)  //查找子串
					{
						boRet = TRUE;
						break;
					}
				} while (Process32NextW(hSnapshot, &entry));
			}
			CloseHandle(hSnapshot);
		}
		return boRet;
	}

	//查找窗口信息
	BOOL FindWindowInfo(LPCWSTR lpClassName, LPCWSTR titleName)
	{
		if (FindWindow(lpClassName, titleName))
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	//终止进程
	bool TerminateWindowsProcess(DWORD processId)
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
		if (hProcess == NULL)
		{
			// 处理打开进程失败的情况
			return false;
		}

		// 终止进程
		bool result = TerminateProcess(hProcess, 0);

		// 关闭进程句柄
		CloseHandle(hProcess);

		return result;
	}


	//单例模式
	//防止程序多开
	BOOL SingletonPattern(const wchar_t* mutexName)
	{
		BOOL boRet = FALSE;

		// 创建互斥体
		hMutex = CreateMutexW(nullptr, TRUE, mutexName);

		// 检查互斥体是否已存在
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			// 关闭互斥体句柄并退出程序
			CloseHandle(hMutex);
		}
		else
		{
			boRet = TRUE;;
		}
		return boRet;
	}

	//退出单例
	void SingletonProgramEnd()
	{
		// 关闭互斥体句柄
		if (hMutex)
		{
			CloseHandle(hMutex);
		}		
	}

	//int转wstring
	std::wstring IntToWString(int value)
	{
		return std::to_wstring(value);
	}

	//wstring转int
	int WStringToInt(const std::wstring& str)
	{
		return std::stoi(str);
	}

	//确认CPU型号
	void ConfirmCPUVendor()
	{
		std::array<int, 4> cpui;

		// Calling __cpuid with 0x0 as the function_id argument
		// gets the number of the highest valid function ID.
		__cpuid(cpui.data(), 0);

		// Capture vendor string
		char vendor[0x20];
		memset(vendor, 0, sizeof(vendor));
		*reinterpret_cast<int*>(vendor) = cpui[ebx];
		*reinterpret_cast<int*>(vendor + 4) = cpui[edx];
		*reinterpret_cast<int*>(vendor + 8) = cpui[ecx];
		std::string vendor_ = vendor;
		if (vendor_ == "GenuineIntel")
		{
			isIntel = true;
		}
		else if (vendor_ == "AuthenticAMD")
		{
			isAMD = true;
		}
	}

	BOOL xxx_Process(DWORD dwProcessID, BOOL fSuspend)
	{
		BOOL bRet = FALSE;
		//Get the list of threads in the system.
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwProcessID);

		if (hSnapshot != INVALID_HANDLE_VALUE)
		{
			//Walk the list of threads.
			THREADENTRY32 te = { sizeof(te) };
			BOOL fOk = Thread32First(hSnapshot, &te);

			for (; fOk; fOk = Thread32Next(hSnapshot, &te))
			{
				//Is this thread in the desired process?
				if (te.th32OwnerProcessID == dwProcessID)
				{
					//Attempt to convert the thread ID into a handle.
					HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);

					if ((hThread != NULL) && (GetCurrentThreadId() != te.th32ThreadID))
					{
						//Suspend or resume the thread.
						if (fSuspend)
						{
							if (SuspendThread(hThread) != -1)
								bRet = TRUE;
						}
						else
						{
							if (ResumeThread(hThread) != -1)
								bRet = TRUE;
						}
					}
					CloseHandle(hThread);
				}
			}
			CloseHandle(hSnapshot);
		}
		return bRet;
	}

	//暂停进程
	BOOL SuspendProcess(DWORD dwProcessID)
	{
		return xxx_Process(dwProcessID, TRUE);
	}

	//恢复进程
	BOOL ResumeProcess(DWORD dwProcessID)
	{
		return xxx_Process(dwProcessID, FALSE);
	}

	void ReportSeriousError(const char* format, ...)
	{
		// 线程同步：使用互斥锁保护临界区
		std::lock_guard<std::mutex> lock(mutex);
		// 必须在格式化文本和弹窗之前捕获 LastError；后续 CRT/Win32
		// 调用可能改写线程错误值，导致真正的 577/31 被丢失。
		const DWORD capturedError = GetLastError();

		va_list args;
		va_start(args, format);
		char message[1024] = { 0 };
		vsnprintf(message, sizeof(message), format, args);
		va_end(args);

		std::ostringstream oss;
		oss << message;

		std::string logMessage = oss.str();
		if (!logMessage.empty())
		{
		DWORD diagnosticError = capturedError;
		if (diagnosticError == ERROR_SUCCESS)
		{
			DWORD parsedError = ERROR_SUCCESS;
			if (TryExtractErrorCode(logMessage, parsedError))
			{
				diagnosticError = parsedError;
			}
		}
		if (diagnosticError == ERROR_SUCCESS)
		{
			diagnosticError = ERROR_UNHANDLED_EXCEPTION;
		}
		const ErrorDetails details = ExplainWin32Error(diagnosticError);
		const std::wstring systemMessage = GetSystemErrorText(diagnosticError);

			UINT codePage = CP_UTF8;
			int required = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, logMessage.c_str(), -1, nullptr, 0);
			if (required == 0)
			{
				codePage = CP_ACP;
				required = MultiByteToWideChar(codePage, 0, logMessage.c_str(), -1, nullptr, 0);
			}
			std::wstring wideMessage;
			if (required > 0)
			{
				wideMessage.resize(static_cast<size_t>(required));
				MultiByteToWideChar(codePage, codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0,
					logMessage.c_str(), -1, &wideMessage[0], required);
			}

			if (wideMessage.empty())
			{
				wideMessage = L"发生未提供文本的严重错误。";
			}
			else if (!wideMessage.empty() && wideMessage.back() == L'\0')
			{
				wideMessage.pop_back();
			}

			std::wstring completeMessage = wideMessage + L"\r\n\r\n错误码：" +
				std::to_wstring(diagnosticError) + L" (0x";
			wchar_t hexadecimal[16]{};
			wsprintfW(hexadecimal, L"%08lX", static_cast<unsigned long>(diagnosticError));
			completeMessage += hexadecimal;
			completeMessage += L")\r\n系统原文：" + systemMessage +
				L"\r\n原因：" + details.reason +
				L"\r\n解决方案：" + details.solution +
				L"\r\n详细日志：前端 Log\\log.ini、Log\\UnrealDbgDll.log；同时保留本弹窗中的原始模块消息。";

			// 保留严重错误弹窗，同时把同一份 Unicode 文本送回宿主的
			// PrintLog。这样错误不会只停留在一个乱码/阻塞弹窗里，前端
			// 日志仍能按错误级别着色并继续显示后续诊断。
			using PrintLogFn = void(__stdcall*)(wchar_t*);
			HMODULE host = GetModuleHandleW(nullptr);
			FARPROC printLog = host == nullptr ? nullptr : GetProcAddress(host, "PrintLog");
			if (printLog != nullptr)
			{
				std::wstring callbackMessage = L"[错误] " + completeMessage;
				auto callback = reinterpret_cast<PrintLogFn>(printLog);
				callback(const_cast<wchar_t*>(callbackMessage.c_str()));
			}

			MessageBoxW(NULL, completeMessage.c_str(), L"严重错误", MB_ICONERROR | MB_SYSTEMMODAL);
			SetLastError(diagnosticError);
		}
	}

	bool fileExists(const std::wstring& path)
	{
		HANDLE hFile = CreateFile(
			path.c_str(),
			GENERIC_READ,
			0, // 不共享
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (hFile != INVALID_HANDLE_VALUE) {
			CloseHandle(hFile);
			return true; // 文件存在
		}
		else {
			return false; // 文件不存在
		}
	}
}
