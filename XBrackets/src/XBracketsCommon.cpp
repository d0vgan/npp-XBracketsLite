#include "XBracketsCommon.h"

namespace XBrackets
{
    bool isExistingFile(const TCHAR* filePath) noexcept
    {
        DWORD dwAttr = ::GetFileAttributes(filePath);
        return ((dwAttr != INVALID_FILE_ATTRIBUTES) && ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0));
    }

    bool isExistingFile(const tstr& filePath) noexcept
    {
        return isExistingFile(filePath.c_str());
    }

    std::vector<char> readFile(const TCHAR* filePath)
    {
        HANDLE hFile = ::CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            // ERROR: Could not open the file
            return {};
        }

        DWORD nBytesRead = 0;
        const DWORD nFileSize = ::GetFileSize(hFile, NULL);
        std::vector<char> buf(nFileSize + 1);
        const BOOL isRead = ::ReadFile(hFile, buf.data(), nFileSize, &nBytesRead, NULL);

        ::CloseHandle(hFile);

        if ( !isRead || nFileSize != nBytesRead )
        {
            // ERROR: Could not read the file
            return {};
        }

        buf[nFileSize] = 0; // the trailing '\0'

        return buf;
    }

    tstr string_to_tstr(const std::string& str)
    {
    #ifdef _UNICODE
        if ( str.empty() )
            return tstr();

        std::vector<TCHAR> buf(str.length() + 1);
        buf[0] = 0;
        ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.length()), buf.data(), static_cast<int>(str.length() + 1));
        return tstr(buf.data(), str.length());
    #else
        return str;
    #endif
    }

    tstr string_to_tstr_changecase(const std::string& str, eStringCase strCase)
    {
#ifdef _UNICODE
        if ( str.empty() )
            return tstr();

        std::vector<TCHAR> buf(str.length() + 1);
        buf[0] = 0;
        ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.length()), buf.data(), static_cast<int>(str.length() + 1));
        if ( strCase == scUpper )
            ::CharUpper(buf.data());
        else
            ::CharLower(buf.data());
        return tstr(buf.data(), str.length());
#else
        if ( str.empty() )
            return str;

        std::vector<char> buf(str.length() + 1);
        lstrcpyA(buf.data(), str.c_str());
        if ( strCase == scUpper )
            ::CharUpperA(buf.data());
        else
            ::CharLowerA(buf.data());
        return tstr(buf.data(), str.length());
#endif
    }
}
