//
// Created by Aleudillonam on 8/2/2023.
//

#pragma once

#include <ojoie/Configuration/platform.h>
#include <ojoie/Template/delegate.hpp>
#include <mutex>

#ifdef AN_WIN
#include <Windows.h>
#endif

namespace AN {

enum ChangeActions {
    kFileAdded          = 1,
    kFileRemoved        = 2,
    kFileModified       = 3,
    kFileRenamedOldName = 4,
    kFileRenamedNewName = 5
};

enum NotifyFlags {
    kNotifyChangeFileName = 0x1,
    kNotifyChangeDirName = 0x2,
    kNotifyChangeSize = 0x8,
    kNotifyChangeTouched = 0x10,
    kNotifyChangeCreated = 0x40,

    kDefaultNotifyFlags = kNotifyChangeFileName | kNotifyChangeDirName | kNotifyChangeSize | kNotifyChangeTouched | kNotifyChangeCreated,
};

struct ChangeRecord {
    std::string   path;
    ChangeActions action;
};


constexpr int kMaxNotifyBuffer = 1024 * 1024;

class AN_API FileWatcher {

    struct WatchedDirectory {
        HANDLE       dirHandle;
        std::wstring name;
        UInt8        buffer[kMaxNotifyBuffer];
        DWORD        bufferLength;
        OVERLAPPED   overlapped;
    };

    std::vector<WatchedDirectory *> m_Dirs;

#ifdef AN_WIN
    mutable std::mutex m_PendingMutex;
    DWORD              m_NotifyFlags;
    HANDLE             m_CompletionPort;
    HANDLE             m_Thread;
    bool               m_ThreadExitRequested;

    static DWORD WINAPI ThreadProc(void *threadParam);
#endif

    void processChangeFile(const WatchedDirectory *dir, const FILE_NOTIFY_INFORMATION* notify );

    void processChangeRecord( const std::wstring& path, ChangeActions action );

public:

    bool start(const std::vector<std::string> &directories, NotifyFlags notifyFlags );
    void stop();

    Delegate<void(const ChangeRecord &record)> onFileChange;

};


}// namespace AN
