//
// Created by aojoie on 5/7/2023.
//

#ifndef OJOIE_FILE_HPP
#define OJOIE_FILE_HPP

#include <ojoie/Configuration/platform.h>
#include <ojoie/Configuration/typedef.h>
#include <string>

#ifdef AN_WIN
typedef void *HANDLE;
#endif

namespace AN {

enum FilePermission {
    kFilePermissionRead      = 0,
    kFilePermissionWrite     = 1,
    kFilePermissionReadWrite = 2,
    kFilePermissionAppend    = 3
};


typedef UInt8 MD5Hash[16];

class AN_API File {

#ifdef AN_WIN
    HANDLE _fileHandle;
#endif
    int         _position;
    std::string _path;

public:
    File();
    ~File();

    bool open(const char *path, FilePermission permission);
    bool close();

    int read(void *buffer, int size);
    int read(int position, void *buffer, int size);

    bool write(const void *buffer, int size);
    bool write(int pos, const void *buffer, int size);

    int getFileLength();
    int getPosition() const { return _position; }
    bool setPosition(int position);

    bool getMD5Hash(MD5Hash hashOut);
};

AN_API std::string GetCurrentDirectory();
AN_API void SetCurrentDirectory(const char *dir);

AN_API std::string GetApplicationPath();
AN_API std::string GetApplicationFolder();

}// namespace AN

#endif//OJOIE_FILE_HPP
