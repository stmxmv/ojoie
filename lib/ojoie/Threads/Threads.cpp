//
// Created by aojoie on 4/18/2023.
//

#include "Threads/Threads.hpp"

#include <mutex>

#ifdef _WIN32
#include "Utility/win32/Unicode.hpp"
#include <WinNls.h>
#include <Windows.h>
#include <processthreadsapi.h>
#include "detours.h"
#endif//_WIN32

namespace AN {

#ifdef _WIN32
typedef HRESULT (WINAPI *PFN_SetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);

static PFN_SetThreadDescription oldSetThreadDescription;

static PVOID Detour(PVOID func, PVOID jmp, bool attach) {
    if (!func)
        return nullptr;

    PVOID call = func;
    DetourTransactionBegin();
    DetourUpdateThread((HANDLE)-2); // all threads
    if (attach)
        DetourAttach(&call, jmp);
    else
        DetourDetach(&call, jmp);
    DetourTransactionCommit();

    return call;
}

static HRESULT WINAPI HookSetThreadDescription(HANDLE, PCWSTR) {
    return S_OK;
}

/// sometime graphic driver changes the thread name, so we hook this function
struct replace_SetThreadDescription_global {
    replace_SetThreadDescription_global() {
        oldSetThreadDescription = (PFN_SetThreadDescription)Detour(::SetThreadDescription, HookSetThreadDescription, true);
    }
} gReplace_SetThreadDescription_global;

#endif//_WIN32


ThreadID GetCurrentThreadID() {
#ifdef _WIN32
    return ::GetCurrentThreadId();
#else
#error "not implement"
#endif//_WIN32
}

void SetCurrentThreadName(const char *name) {
#ifdef _WIN32
    oldSetThreadDescription(::GetCurrentThread(), Utf8ToWide(name).c_str());
#else
#error "not implement"
#endif
}

std::string GetCurrentThreadName() {
    wchar_t *description;
    ::GetThreadDescription(::GetCurrentThread(), &description);
    std::string ret = WideToUtf8(description);
    LocalFree(description);
    return ret;
}

}