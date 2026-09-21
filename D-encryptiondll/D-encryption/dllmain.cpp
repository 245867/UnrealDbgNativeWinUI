#include "dllmain.h"

#include <limits>


#pragma comment(lib, "advapi32.lib")

namespace
{
    void AppendEncryptionDiagnosis(std::wstringstream& message, const DWORD error)
    {
        switch (error)
        {
        case ERROR_FILE_NOT_FOUND:
            message << L"原因：加密文件不存在或路径不正确。"
                << L"\r\n解决方案：确认文件名、程序工作目录和访问权限，重新部署缺失文件。";
            break;
        case ERROR_ACCESS_DENIED:
            message << L"原因：当前进程无权读取/写入文件，或文件被安全策略拦截。"
                << L"\r\n解决方案：以管理员身份运行，检查文件 ACL、杀毒软件拦截记录和文件占用情况。";
            break;
        case ERROR_INVALID_PARAMETER:
            message << L"原因：传入的文件名、密钥、数据指针或长度为空/越界。"
                << L"\r\n解决方案：检查调用参数和输出缓冲区，修正后重新执行。";
            break;
        case ERROR_INVALID_DATA:
        case NTE_BAD_DATA:
            message << L"原因：密文格式无效、数据已损坏，或解密密钥与加密时不一致。"
                << L"\r\n解决方案：确认密钥、文件完整性及加解密版本完全匹配；必要时从原始数据重新生成密文。";
            break;
        case NTE_BAD_KEYSET:
            message << L"原因：Windows 加密服务找不到或无法打开密钥容器。"
                << L"\r\n解决方案：确认 Cryptographic Services 服务正在运行，并检查当前用户密钥容器权限。";
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            message << L"原因：系统无法为加密/解密缓冲区分配足够内存。"
                << L"\r\n解决方案：关闭无关程序、检查内存占用后重试。";
            break;
        default:
            message << L"原因：Windows 加密 API 或其依赖组件拒绝了请求。"
                << L"\r\n解决方案：检查 Cryptographic Services 服务、文件权限、密钥和组件位数；"
                << L"同时查看 Log\\log.ini 与 Log\\UnrealDbgDll.log。";
            break;
        }
    }

    void ReportEncryptionFailure(const wchar_t* operation, DWORD error)
    {
        if (error == ERROR_SUCCESS)
        {
            error = ERROR_GEN_FAILURE;
        }

        // CryptoAPI 的错误码必须回传给调用者；否则上层只能看到“返回
        // 长度无效”，会再次退化成旧版笼统的错误 31。
        SetLastError(error);

        std::wstringstream message;
        message << L"加密模块操作失败：" << (operation == nullptr ? L"未知操作" : operation)
            << L"；错误码=" << error << L" (0x";
        wchar_t hexadecimal[16]{};
        wsprintfW(hexadecimal, L"%08lX", static_cast<unsigned long>(error));
        message << hexadecimal << L")\r\n";
        AppendEncryptionDiagnosis(message, error);
        LPWSTR systemText = nullptr;
        const DWORD systemLength = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0, reinterpret_cast<LPWSTR>(&systemText), 0, nullptr);
        if (systemLength != 0 && systemText != nullptr)
        {
            std::wstring systemMessage(systemText, systemLength);
            LocalFree(systemText);
            while (!systemMessage.empty() && (systemMessage.back() == L'\r' || systemMessage.back() == L'\n' ||
                systemMessage.back() == L' '))
            {
                systemMessage.pop_back();
            }
            if (!systemMessage.empty())
            {
                message << L"\r\n系统原文：" << systemMessage;
            }
            else
            {
                message << L"\r\n系统原文：系统未提供错误文本。";
            }
        }
        const std::wstring text = message.str();
        ::OutputDebugStringW((L"[D-encryption] " + text + L"\n").c_str());
        SetLastError(error);
    }
}

void EncryptDataToFile(TCHAR* data, TCHAR* filename, TCHAR* userKey)
{
    try
    {
        if (!data || !filename || !userKey)
        {
            ReportEncryptionFailure(L"EncryptDataToFile 参数校验", ERROR_INVALID_PARAMETER);
            return;
        }

        if (data && filename && userKey)
        {
            std::string _data = Common::wideStringToString(data);
            std::string _filename = Common::wideStringToString(filename);
            std::string _userKey = Common::wideStringToString(userKey);
            EncryptDataToFile_internal(_data, _filename, _userKey);
        }
    }
    catch (const std::exception& e)
    {
        SetLastError(ERROR_UNHANDLED_EXCEPTION);
        Common::ReportSeriousError("%s[%d] 加密模块发生异常：%s", __func__, __LINE__, e.what());
    }
}

int DecryptDataFromFile(TCHAR* filename, TCHAR* userKey, TCHAR* decryptedData)
{
    int decryptedDataLen = 0;
    try
    {
        if (!filename || !userKey)
        {
            ReportEncryptionFailure(L"DecryptDataFromFile 参数校验", ERROR_INVALID_PARAMETER);
            return 0;
        }

        // 让内部函数用 ERROR_SUCCESS 表示“确实完成”，避免把调用者
        // 之前遗留的 GetLastError 误当成当前解密失败。
        SetLastError(ERROR_SUCCESS);
        if (filename && userKey)
        {
            std::string _filename = Common::wideStringToString2(filename);
            std::string _userKey = Common::wideStringToString(userKey);
            std::string _decryptedData;
            DecryptDataFromFile_internal(_filename, _userKey, _decryptedData);

            const DWORD decryptError = GetLastError();
            if (decryptError != ERROR_SUCCESS)
            {
                return 0;
            }

            std::wstring ws_decryptedData = Common::stringToWideString(_decryptedData);
            const size_t resultBytes = (ws_decryptedData.length() + 1) * sizeof(WCHAR);
            if (resultBytes > static_cast<size_t>((std::numeric_limits<int>::max)()))
            {
                ReportEncryptionFailure(L"DecryptDataFromFile 输出缓冲区长度溢出", ERROR_BUFFER_OVERFLOW);
                return 0;
            }
            decryptedDataLen = static_cast<int>(resultBytes);

            if (decryptedData)
            {
                // 复制数据
                wcscpy(decryptedData, ws_decryptedData.c_str());
            }
        }
    }
    catch (const std::exception& e)
    {
        SetLastError(ERROR_UNHANDLED_EXCEPTION);
        Common::ReportSeriousError("%s[%d] 解密模块发生异常：%s", __func__, __LINE__, e.what());
        //Common::ReportSeriousError("%s[%d] 解密数据失败! (error: %d)", __func__, __LINE__, GetLastError());
    }
    return decryptedDataLen;
}



void EncryptDataToFile_internal(const std::string& data, const std::string& filename, const std::string& userKey)
{
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hKey = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE* pbData = (BYTE*)data.c_str();
    if (data.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
    {
        ReportEncryptionFailure(L"EncryptDataToFile 明文长度溢出", ERROR_ARITHMETIC_OVERFLOW);
        return;
    }
    if (userKey.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
    {
        ReportEncryptionFailure(L"EncryptDataToFile 密钥长度溢出", ERROR_ARITHMETIC_OVERFLOW);
        return;
    }
    DWORD dwPlainTextLen = static_cast<DWORD>(data.size());
    DWORD dwBufLen = dwPlainTextLen;

    // 1. Acquire a cryptographic provider context
    // 获取加密提供程序上下文
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        ReportEncryptionFailure(L"CryptAcquireContext 获取加密上下文", GetLastError());
        return;
    }

    // 2. Create a hash object
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        ReportEncryptionFailure(L"CryptCreateHash 创建哈希对象", GetLastError());
        CryptReleaseContext(hProv, 0);
        return;
    }

    // 3. Hash the data (you can also use a password)
    // 对数据进行哈希处理（您也可以使用密码）
    if (!CryptHashData(hHash, (BYTE*)userKey.c_str(), static_cast<DWORD>(userKey.size()), 0)) {
        ReportEncryptionFailure(L"CryptHashData 写入密钥哈希", GetLastError());
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    // 4. Derive a key from the hash
    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey)) {
        ReportEncryptionFailure(L"CryptDeriveKey 派生加密密钥", GetLastError());
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    //获得密文长度
    if (!CryptEncrypt(hKey, 0, TRUE, 0, NULL, &dwBufLen, 0)) {
        ReportEncryptionFailure(L"CryptEncrypt 计算密文长度", GetLastError());
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }


    // 5. Encrypt the data
    BYTE* pbCipherText = new BYTE[dwBufLen + 1];  //分配密文缓冲区
    memset(pbCipherText, 0, dwBufLen + 1);
    memcpy(pbCipherText, pbData, dwPlainTextLen);  //拷贝明文内容

    DWORD tmp = dwPlainTextLen;
    if (!CryptEncrypt(hKey, 0, TRUE, 0, pbCipherText, &tmp, dwBufLen + 1)) {
        ReportEncryptionFailure(L"CryptEncrypt 加密数据", GetLastError());
        delete[] pbCipherText;
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    // 6. Write the encrypted data to a file
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open())
    {
        ReportEncryptionFailure(L"打开加密输出文件", ERROR_OPEN_FAILED);
        delete[] pbCipherText;
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }
    outFile.write((char*)pbCipherText, dwBufLen);
    if (!outFile.good())
    {
        ReportEncryptionFailure(L"写入加密输出文件", ERROR_WRITE_FAULT);
    }
    outFile.close();

    // Clean up
    delete[] pbCipherText;
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}


void DecryptDataFromFile_internal(const std::string& filename, const std::string& userKey, std::string& decryptedData)
{
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hKey = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE* pbCipherText = nullptr;
    DWORD dwCipherTextLen = 0;

    // 1. Acquire a cryptographic provider context
    // 获取加密提供程序上下文
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        ReportEncryptionFailure(L"CryptAcquireContext 获取解密上下文", GetLastError());
        return;
    }

    // 2. Create a hash object
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        ReportEncryptionFailure(L"CryptCreateHash 创建解密哈希对象", GetLastError());
        CryptReleaseContext(hProv, 0);
        return;
    }

    // 3. Hash the user key
    if (userKey.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)()))
    {
        ReportEncryptionFailure(L"DecryptDataFromFile 密钥长度溢出", ERROR_ARITHMETIC_OVERFLOW);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }
    if (!CryptHashData(hHash, (BYTE*)userKey.c_str(), static_cast<DWORD>(userKey.size()), 0)) {
        ReportEncryptionFailure(L"CryptHashData 写入解密密钥哈希", GetLastError());
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    // 4. Derive a key from the hash
    // 从哈希中导出密钥
    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey)) {
        ReportEncryptionFailure(L"CryptDeriveKey 派生解密密钥", GetLastError());
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    // 5. Read the encrypted data from the file
    // 从文件中读取加密数据
    std::ifstream inFile(filename, std::ios::binary | std::ios::ate);
    if (!inFile.is_open()) {
        ReportEncryptionFailure(L"打开加密文件", ERROR_FILE_NOT_FOUND);
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    const std::streampos fileSize = inFile.tellg();
    if (fileSize < 0 || fileSize > static_cast<std::streampos>((std::numeric_limits<DWORD>::max)()))
    {
        ReportEncryptionFailure(L"读取加密文件大小", ERROR_INVALID_DATA);
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }
    dwCipherTextLen = static_cast<DWORD>(fileSize);
    if (dwCipherTextLen == 0)
    {
        ReportEncryptionFailure(L"读取加密文件内容", ERROR_INVALID_DATA);
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }
    inFile.seekg(0, std::ios::beg);
    pbCipherText = new BYTE[dwCipherTextLen + 1];
    memset(pbCipherText, 0, dwCipherTextLen + 1);
    inFile.read((char*)pbCipherText, dwCipherTextLen);
    if (inFile.gcount() != static_cast<std::streamsize>(dwCipherTextLen))
    {
        ReportEncryptionFailure(L"读取加密文件内容", ERROR_READ_FAULT);
        delete[] pbCipherText;
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }
    inFile.close();

    // 6. Decrypt the data
    // 解密数据
    DWORD dwPlainTextLen = dwCipherTextLen;
    if (!CryptDecrypt(hKey, 0, TRUE, 0, pbCipherText, &dwPlainTextLen)) {
        ReportEncryptionFailure(L"CryptDecrypt 解密数据", GetLastError());
        delete[] pbCipherText;
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    // 7. Convert decrypted data to string
    // 将解密数据转换为字符串
    decryptedData.assign((char*)pbCipherText, dwPlainTextLen);

    // Clean up
    delete[] pbCipherText;
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}
