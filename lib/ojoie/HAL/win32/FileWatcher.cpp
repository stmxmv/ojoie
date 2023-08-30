//
// Created by Aleudillonam on 8/2/2023.
//

#include "HAL/FileWatcher.hpp"
#include "Utility/win32/Path.hpp"

namespace AN {

DWORD WINAPI FileWatcher::ThreadProc(void *threadParam) {
    FileWatcher *watcher = (FileWatcher *) threadParam;
    // copy out completionPort
    HANDLE completionPort = 0;
    {
        std::lock_guard lock(watcher->m_PendingMutex);
        completionPort = watcher->m_CompletionPort;
    }

    for (;;) {
        DWORD             numBytes   = 0;
        WatchedDirectory *di         = NULL;
        OVERLAPPED       *overlapped = NULL;

        // Fetch results for this directory through the completion port.
        // This will stall until something is available.
        BOOL ret = GetQueuedCompletionStatus(completionPort, &numBytes, (PULONG_PTR) &di, &overlapped, INFINITE);

        // Maybe we have to exit now?
        {
            std::lock_guard lock(watcher->m_PendingMutex);
            if (watcher->m_ThreadExitRequested)
                break;
        }

        if (ret == 0) {
            continue;
        }

        // If we have directory and actually got some bytes, process them
        if (di && numBytes > 0) {
            FILE_NOTIFY_INFORMATION *notify = (FILE_NOTIFY_INFORMATION *) di->buffer;

            DWORD byteOffset;
            do {
                watcher->processChangeFile(di, notify);
                byteOffset = notify->NextEntryOffset;
                notify     = (FILE_NOTIFY_INFORMATION *) ((BYTE *) notify + byteOffset);
            } while (byteOffset);

            // reissue directory watch
            if (!ReadDirectoryChangesW(di->dirHandle,
                                       di->buffer,
                                       kMaxNotifyBuffer,
                                       TRUE,
                                       watcher->m_NotifyFlags,
                                       &di->bufferLength,
                                       &di->overlapped,
                                       NULL)) {
                // don't log errors, we're on a thread
            }
        }
    }

    return 0;
}

void FileWatcher::processChangeFile(const WatchedDirectory *dir, const FILE_NOTIFY_INFORMATION *notify) {
    // file name length is in bytes!
    int nameLength = notify->FileNameLength / sizeof(wchar_t);
    ANAssert((notify->FileNameLength & 1) == 0);

    // copy the file name
    std::wstring wideName;
    wideName.resize(nameLength);
    wideName.assign(notify->FileName, nameLength);

    // construct full file name
    std::wstring fullWideName = dir->name;
    fullWideName += wideName;

    // add change record
    std::lock_guard lock(m_PendingMutex);
    processChangeRecord(fullWideName, static_cast<ChangeActions>(notify->Action));
}

void FileWatcher::processChangeRecord(const std::wstring &path, ChangeActions action) {
    ChangeRecord rec;

    rec.action = action;
    rec.path   = ConvertWindowsPathName(path);

    if (onFileChange) {
        onFileChange(rec);
    }
}

bool FileWatcher::start(const std::vector<std::string> &directories, NotifyFlags notifyFlags) {
    m_NotifyFlags = notifyFlags;

    // Create IO completion	ports for each directory
    m_Dirs.reserve(directories.size());
    for (int i = 0; i < directories.size(); ++i) {
        m_Dirs.push_back(new WatchedDirectory());
        WatchedDirectory &dir = *m_Dirs.back();

        // Open directory
        dir.name      = Utf8ToWide(directories[i]);
        dir.dirHandle = CreateFileW(dir.name.c_str(),
                                    FILE_LIST_DIRECTORY,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                    NULL);

        if (dir.dirHandle == INVALID_HANDLE_VALUE) {
            AN_LOG(Error, "DirectoryWatcher: can't open directory: %s", directories[i].c_str());
            m_Dirs.pop_back();
            continue;
        }

        // add ending path separator if does not exist yet
        if (!dir.name.empty()) {
            wchar_t last = dir.name.back();
            if (last != '\\' && last != '/') {
                dir.name.push_back(L'\\');
            }
        }

        // create completion port
        m_CompletionPort = CreateIoCompletionPort(dir.dirHandle, m_CompletionPort, (ULONG_PTR) &dir, 0);
        if (m_CompletionPort == INVALID_HANDLE_VALUE) {
            AN_LOG(Error, "DirectoryWatcher: can't create IO completion port");
            return false;
        }
    }

    if (m_Dirs.empty()) {
        AN_LOG(Error, "DirectoryWatcher: no valid directories");
        if (m_CompletionPort)
            CloseHandle(m_CompletionPort);
        m_CompletionPort = 0;
        return false;
    }

    // start watching directories
    for (int i = 0; i < m_Dirs.size(); ++i) {
        if (!ReadDirectoryChangesW(
                    m_Dirs[i]->dirHandle,
                    m_Dirs[i]->buffer,
                    kMaxNotifyBuffer,
                    TRUE,
                    m_NotifyFlags,
                    &m_Dirs[i]->bufferLength,
                    &m_Dirs[i]->overlapped,
                    NULL)) {
            AN_LOG(Error, "DirectoryWatcher: ReadDirectoryChangesW failed");
        }
    }

    // spawn a thread to manage the changes
    DWORD threadID;
    m_Thread = CreateThread(NULL, 0, ThreadProc, (LPVOID) this, 0, &threadID);
    if (m_Thread == INVALID_HANDLE_VALUE) {
        AN_LOG(Error, "DirectoryWatcher: failed to create thread");
        return false;
    }

    return true;
}

void FileWatcher::stop() {
    // tell the thread to exit
    {
        std::lock_guard lock(m_PendingMutex);
        m_ThreadExitRequested = true;
    }

    PostQueuedCompletionStatus(m_CompletionPort, 0, 0, NULL);

    // wait for thread to exit
    if (WaitForSingleObject(m_Thread, 5000) != WAIT_OBJECT_0) {
        AN_LOG(Error, "DirectoryWatcher: watcher thread did not exit nicely!");
    }

    CloseHandle(m_Thread);
    m_Thread = 0;

    CloseHandle(m_CompletionPort);
    m_CompletionPort = 0;

    for (int i = 0; i < m_Dirs.size(); ++i) {
        CloseHandle(m_Dirs[i]->dirHandle);
        delete m_Dirs[i];
    }
    m_Dirs.clear();
}


}// namespace AN