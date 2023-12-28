//
// Created by aojoie on 5/7/2023.
//

#include "HAL/File.hpp"
#include "Core/private/win32/App.hpp"
#include "Utility/Path.hpp"
#include "Utility/win32/Path.hpp"
#include "Utility/win32/Unicode.hpp"

#include <Wincrypt.h>
#include <Windows.h>

namespace AN
{


static constexpr int kDefaultPathBufferSize = MAX_PATH * 4;

bool RemoveReadOnlyW(LPCWSTR path)
{
    DWORD attributes = GetFileAttributesW(path);

    if (INVALID_FILE_ATTRIBUTES != attributes)
    {
        attributes &= ~FILE_ATTRIBUTE_READONLY;
        return SetFileAttributesW(path, attributes);
    }

    return false;
}

static HANDLE OpenFileWithPath(const char *path, FilePermission permission)
{
    std::wstring wPath = Utf8ToWide(path);
    DWORD        accessMode, shareMode, createMode;
    switch (permission)
    {
        case kFilePermissionRead:
            accessMode = FILE_GENERIC_READ;
            shareMode  = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            createMode = OPEN_EXISTING;
            break;
        case kFilePermissionWrite:
            accessMode = FILE_GENERIC_WRITE;
            shareMode  = 0;
            createMode = CREATE_ALWAYS;
            break;
        case kFilePermissionAppend:
            accessMode = FILE_GENERIC_WRITE;
            shareMode  = 0;
            createMode = OPEN_ALWAYS;
            break;
        case kFilePermissionReadWrite:
            accessMode = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
            shareMode  = 0;
            createMode = OPEN_ALWAYS;
            break;
        default:
            AN_LOG(Error, "Unknown file permission mode");
            return INVALID_HANDLE_VALUE;
    }
    HANDLE fileHandle = CreateFileW(wPath.c_str(), accessMode, shareMode, NULL, createMode, NULL, NULL);
    if (INVALID_HANDLE_VALUE == fileHandle)
    {
        if (FILE_GENERIC_WRITE == (accessMode & FILE_GENERIC_WRITE))
        {
            DWORD lastError = GetLastError();
            if (ERROR_ACCESS_DENIED == lastError)
            {
                if (RemoveReadOnlyW(wPath.c_str()))
                {
                    fileHandle = CreateFileW(wPath.c_str(), accessMode, shareMode, NULL, createMode, NULL, NULL);
                }
                else
                    SetLastError(lastError);
            }
            else
                SetLastError(lastError);
        }
    }
    if (permission == kFilePermissionAppend && fileHandle != INVALID_HANDLE_VALUE)
        SetFilePointer(fileHandle, 0, NULL, FILE_END);
    return fileHandle;
}

File::File()
    : m_IsEOF(),
      _fileHandle(INVALID_HANDLE_VALUE),
      _position() {}

File::~File()
{
    Close();
}

bool File::Open(const char *path, FilePermission permission)
{
    Close();
    _path       = path;
    _fileHandle = OpenFileWithPath(path, permission);
    _position   = 0;
    if (_fileHandle != INVALID_HANDLE_VALUE)
    {
        if (permission == kFilePermissionAppend &&
            (_position = SetFilePointer(_fileHandle, 0, NULL, FILE_CURRENT)) == INVALID_SET_FILE_POINTER)
        {
            Close();
            return false;
        }
        return true;
    }

    return false;
}

bool File::Close()
{
    if (_fileHandle != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(_fileHandle))
        {
            DWORD lastError = GetLastError();
            AN_LOG(Error, "Closing file %s: %s", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
        }
        _fileHandle = INVALID_HANDLE_VALUE;
    }
    _position = 0;
    _path.clear();
    return true;
}

int File::Read(void *buffer, int size)
{
    if (_fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesRead = 0;
        BOOL  ok        = ReadFile(_fileHandle, buffer, size, &bytesRead, NULL);
        if (ok || bytesRead == size)
        {
            _position += bytesRead;
            return bytesRead;
        }
        else
        {
            //_position = -1;
            /// TODO retry read
            DWORD lastError = GetLastError();
            AN_LOG(Error, "Reading file %s fail: %s ", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
            return 0;
        }
    }
    else
    {
        AN_LOG(Error, "Reading failed because the file was not opened");
        return 0;
    }
}

std::string File::ReadLine()
{
    std::string buffer(128, 0);

    DWORD lastBytesRead = 0;

    while (true)
    {
        if (_fileHandle != INVALID_HANDLE_VALUE)
        {
            DWORD bytesRead = 0;
            BOOL  ok        = ReadFile(_fileHandle, buffer.data(), buffer.size(), &bytesRead, NULL);
            if (ok && bytesRead > lastBytesRead)
            {
                auto pos = buffer.find_first_of('\n');
                if (pos == std::string::npos)
                {
                    if (bytesRead < buffer.size())
                    {
                        m_IsEOF = true;
                        return buffer.substr(0, bytesRead);
                    }

                    lastBytesRead = bytesRead;
                    buffer.resize(buffer.size() * 2);
                    SetFilePointer(_fileHandle, _position, NULL, FILE_BEGIN);
                    continue;
                }
                if (pos == 0)
                {
                    _position += 1;
                    SetFilePointer(_fileHandle, _position, NULL, FILE_BEGIN);
                    return {};
                }

                _position += pos + 1;
                SetFilePointer(_fileHandle, _position, NULL, FILE_BEGIN);

                if (buffer[pos - 1] == '\r')
                {

                    return buffer.substr(0, pos - 1);
                }

                return buffer.substr(0, pos);

            }
            else
            {
                //_position = -1;
                /// TODO retry read
//                DWORD lastError = GetLastError();
//                AN_LOG(Error, "Reading file %s fail: %s ", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
                m_IsEOF = true;
                return {};
            }
        }
        else
        {
            AN_LOG(Error, "Reading failed because the file was not opened");
            return {};
        }
    }

}

int File::Read(int position, void *buffer, int size)
{
    if (_fileHandle != INVALID_HANDLE_VALUE)
    {
        if (position != _position)
        {
            DWORD newPosition = 0;
            newPosition       = SetFilePointer(_fileHandle, position, NULL, FILE_BEGIN);
            DWORD lastError   = GetLastError();
            bool  failed      = (newPosition == INVALID_SET_FILE_POINTER && lastError != NO_ERROR);
            if (newPosition == position && !failed)
            {
                _position = position;
            }
            else
            {
                _position = -1;
                AN_LOG(Error, "Reading file %s fail: %s ", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
                return 0;
            }
        }

        DWORD bytesRead = 0;
        BOOL  ok        = ReadFile(_fileHandle, buffer, size, &bytesRead, NULL);
        if (ok)
        {
            _position += bytesRead;
            return bytesRead;
        }
        else
        {
            DWORD lastError = GetLastError();
            _position       = -1;
            AN_LOG(Error, "Reading file %s fail: %s ", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
            return 0;
        }
    }
    else
    {
        AN_LOG(Error, "Reading failed because the file was not opened");
        return 0;
    }
}

bool File::Write(const void *buffer, int size)
{
    if (_fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten = 0;
        BOOL  ok           = WriteFile(_fileHandle, buffer, size, &bytesWritten, NULL);
        if (ok && size == bytesWritten)
        {
            _position += bytesWritten;
            return true;
        }
        else
        {
            DWORD lastError = GetLastError();
            AN_LOG(Error, "Writing file %s fail: %s ", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
            return false;
        }
    }
    else
    {
        AN_LOG(Error, "Writing failed because the file was not opened");
        return false;
    }
}

bool File::WriteLine(const char *line)
{
    if (_fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten = 0;
        int len = strlen(line);
        std::vector<char> buffer(len + 2);
        if (len > 0)
        {
            memcpy(buffer.data(), line, len * sizeof(char));
        }
        buffer[buffer.size() - 2] = '\r';
        buffer[buffer.size() - 1] = '\n';
        BOOL  ok           = WriteFile(_fileHandle, buffer.data(), buffer.size(), &bytesWritten, NULL);
        if (ok && buffer.size() == bytesWritten)
        {
            _position += bytesWritten;
            return true;
        }
        else
        {
            DWORD lastError = GetLastError();
            AN_LOG(Error, "Writing file %s fail: %s ", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
            return false;
        }
    }
    else
    {
        AN_LOG(Error, "Writing failed because the file was not opened");
        return false;
    }
}

bool File::Write(int position, const void *buffer, int size)
{
    if (_fileHandle != INVALID_HANDLE_VALUE)
    {
        // Seek if necessary
        if (position != _position)
        {
            DWORD newPosition = 0;
            newPosition       = SetFilePointer(_fileHandle, position, NULL, FILE_BEGIN);
            DWORD lastError   = GetLastError();
            bool  failed      = (newPosition == INVALID_SET_FILE_POINTER && lastError != NO_ERROR);
            if (newPosition == position && !failed)
            {
                _position = position;
            }
            else
            {
                _position = -1;
                AN_LOG(Error, "Writing file %s fail: %s ", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
                return false;
            }
        }

        DWORD bytesWritten = 0;
        BOOL  ok           = WriteFile(_fileHandle, buffer, size, &bytesWritten, NULL);
        DWORD lastError    = GetLastError();
        if (ok && size == bytesWritten)
        {
            _position += bytesWritten;
            return true;
        }
        else
        {
            _position = -1;
            AN_LOG(Error, "Writing file %s fail: %s ", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
            return false;
        }
    }
    else
    {
        AN_LOG(Error, "Writing failed because the file was not opened");
        return false;
    }
}

int File::GetFileLength() const
{
    if (_fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD size;
        ::GetFileSize(_fileHandle, &size);
        return size;
    }
    return 0;
}

bool File::SetPosition(int position)
{
    _position         = position;
    DWORD newPosition = 0;
    newPosition       = SetFilePointer(_fileHandle, position, NULL, FILE_BEGIN);
    DWORD lastError   = GetLastError();
    bool  failed      = (newPosition == INVALID_SET_FILE_POINTER && lastError != NO_ERROR);
    if (newPosition == position && !failed)
    {
        m_IsEOF = false;
        _position = position;
        return true;
    }
    else
    {
        _position = -1;
        AN_LOG(Error, "setting file position %s fail: %s ", _path.c_str(), WIN::TranslateErrorCode(lastError).c_str());
    }
    return false;
}

bool File::IsEOF() const
{
    return m_IsEOF;
}

static HCRYPTPROV gProv = 0;

struct CryptRAII
{
    ~CryptRAII()
    {
        if (gProv)
        {
            CryptReleaseContext(gProv, 0);
            gProv = 0;
        }
    }
} gCryptRAII;

static void InitializeCryptContext()
{
    static bool inited = false;
    if (!inited)
    {
        inited         = true;
        DWORD dwStatus = 0;

        // Get handle to the crypto provider
        if (!CryptAcquireContext(&gProv,
                                 NULL,
                                 NULL,
                                 PROV_RSA_FULL,
                                 CRYPT_VERIFYCONTEXT))
        {
            dwStatus = GetLastError();
            AN_LOG(Error, "CryptAcquireContext failed: %s", WIN::TranslateErrorCode(dwStatus).c_str());
            return;
        }
    }
}

bool File::GetMD5Hash(MD5Hash hashOut)
{
    if (_fileHandle == INVALID_HANDLE_VALUE) return false;

    InitializeCryptContext();

    if (!gProv) return false;

    static constexpr int BUFSIZE = 1024;
    static constexpr int MD5LEN  = 16;

    BYTE  rgbFile[BUFSIZE];
    DWORD cbRead   = 0;
    DWORD dwStatus = 0;

    HCRYPTHASH hHash = 0;
    struct HashRAII
    {
        HCRYPTHASH *hHashPtr;
        ~HashRAII()
        {
            if (*hHashPtr)
            {
                ANAssert(CryptDestroyHash(*hHashPtr));
            }
        }
    } hashRaii{ &hHash };

    if (!CryptCreateHash(gProv, CALG_MD5, 0, 0, &hHash))
    {
        dwStatus = GetLastError();
        AN_LOG(Error, "CryptAcquireContext failed: %s", WIN::TranslateErrorCode(dwStatus).c_str());
        return false;
    }

    int oldPosition = _position;
    SetPosition(0);
    while ((cbRead = Read(rgbFile, BUFSIZE)) > 0)
    {
        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            AN_LOG(Error, "CryptHashData failed: %s", WIN::TranslateErrorCode(dwStatus).c_str());
            /// restore position
            SetPosition(oldPosition);
            return false;
        }
    }

    DWORD cbHash = 0;
    BYTE  rgbHash[MD5LEN];
    cbHash = MD5LEN;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        memcpy(hashOut, rgbHash, MD5LEN);

        /// restore position
        SetPosition(oldPosition);
        return true;
    }
    else
    {
        dwStatus = GetLastError();
        AN_LOG(Error, "CryptGetHashParam failed: %s", WIN::TranslateErrorCode(dwStatus).c_str());
        /// restore position
        SetPosition(oldPosition);
        return false;
    }
}

#undef GetCurrentDirectory
#undef SetCurrentDirectory

std::string GetCurrentDirectory()
{
    DWORD        dwSize = ::GetCurrentDirectoryW(0, nullptr);
    std::wstring wPath;
    wPath.resize(dwSize);
    ::GetCurrentDirectoryW(wPath.size(), wPath.data());
    wPath.pop_back();// remove last null character
    return WideToUtf8(wPath);
}

void SetCurrentDirectory(const char *dir)
{
    ANAssert(::SetCurrentDirectoryW(Utf8ToWide(dir).c_str()) == TRUE);
}

std::string GetApplicationFolder()
{
    return Path(GetApplicationPath()).RemoveLastComponent().ToString();
}

std::string GetApplicationPath()
{
    wchar_t buffer[kDefaultPathBufferSize];
    GetModuleFileNameW(nullptr, buffer, kDefaultPathBufferSize);
    return ConvertWindowsPathName(buffer);
}

}// namespace AN