// -*- Ang -*-
//===--------------------------- `target name` ---------------------------------===//
//
// SourceFile.cpp
// lib/Basic
// Created by Molybdenum on 3/7/23.
//===----------------------------------------------------------------------===//

#include "Utility//SourceFile.hpp"
#include <iostream>

#ifdef _WIN32

#include <Windows.h>

#else

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#endif

namespace AN {

struct SourceFile::Impl {
#ifdef _WIN32
    HANDLE hFile;
    HANDLE hMapFile;
#else
    int fd;
    size_t len;
#endif
};


SourceFile::SourceFile() : impl(new Impl()) {}


bool SourceFile::init(std::string_view path, Error *error) {
    return open(path, error);
}

bool SourceFile::open(std::string_view path, Error *error) {
    // map file into memory
#ifdef _WIN32

    // convert to utf16 to support non-ASCII path
    std::wstring wFilePath;
    wFilePath.resize(MultiByteToWideChar(CP_UTF8, 0, path.data(), (int)path.length() + 1, nullptr, 0));
    MultiByteToWideChar(CP_UTF8, 0, path.data(), (int)path.length() + 1, wFilePath.data(), (int)wFilePath.size());

    HANDLE hFile = CreateFileW(
            wFilePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        if (error) {
            *error = Error(1, "CreateFile return invalid handle");
        }
        return false;
    }

    impl->hFile = hFile;

    DWORD dwFileSize = GetFileSize(hFile, nullptr);

    HANDLE hMapFile = CreateFileMapping(
            hFile,
            nullptr,
            PAGE_READONLY,
            0,
            dwFileSize,
            nullptr);

    if (hMapFile == NULL) {
        CloseHandle(hFile);

        if (error) {
            *error = Error(2, "CreateFileMapping fail");
        }

        return false;
    }

    impl->hMapFile = hMapFile;

    LPVOID lpFile = MapViewOfFile(
            hMapFile,
            FILE_MAP_READ,
            0,
            0,
            dwFileSize);

    if (lpFile == nullptr) {
        CloseHandle(hMapFile);
        CloseHandle(hFile);

        if (error) {
            *error = Error(3, "MapViewOfFile fail");
        }

        return false;
    }

    // Use the memory mapped file here
    _buffer = (const char *) lpFile;

#else
    int fd = open(path.data(), O_RDONLY);
    if (fd == -1) {
        if (error) {
            *error = Error(1, "open failed");
        }
        return false;
    }

    impl->fd = fd;

    // mmap
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        if (error) {
            *error = Error(2, "call fstat failed");
        }
        return false;
    }

    impl->len = sb.st_size;

    _buffer = (char *) mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (_buffer == MAP_FAILED) {
        if (error) {
            *error = Error(3, "map file failed");
        }
    }

#endif

    return true;
}

void SourceFile::close() {
    if (impl) {

#ifdef _WIN32
        if (_buffer) {
            UnmapViewOfFile((LPVOID) _buffer);
            CloseHandle(impl->hMapFile);
            CloseHandle(impl->hFile);

            _buffer = nullptr;
        }

#else
        if (_buffer) {
            if (munmap((void *) _buffer, impl->len) == -1) {
                std::cerr << "Fail to unmap file at buffer" << (void *) _buffer << '\n';
            }

            close(impl->fd);

            _buffer = nullptr;
        }

#endif
    }
}

SourceFile::~SourceFile() {
    if (impl) {

#ifdef _WIN32
        if (_buffer) {
            UnmapViewOfFile((LPVOID) _buffer);
            CloseHandle(impl->hMapFile);
            CloseHandle(impl->hFile);

            _buffer = nullptr;
        }

#else
        if (_buffer) {
            if (munmap((void *) _buffer, impl->len) == -1) {
                std::cerr << "Fail to unmap file at buffer" << (void *) _buffer << '\n';
            }

            close(impl->fd);

            _buffer = nullptr;
        }

#endif

        delete impl;
        impl = nullptr;
    }
}

}// namespace AN