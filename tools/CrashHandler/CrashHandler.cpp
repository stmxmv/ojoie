//
// Created by aojoie on 6/1/2023.
//

#include "CrashHandler.hpp"
#include <ojoie/IO/FileOutputStream.hpp>
#include <ojoie/Serialize/Coder/YamlEncoder.hpp>
#include <ojoie/Utility/Log.h>
#include <ojoie/Utility/win32/Unicode.hpp>

#include <filesystem>
#include <ShlObj_core.h>
#include <minidumpapiset.h>

#include "detours.h"

#pragma comment(lib, "Dbghelp.lib")

#define DEBUGTRACE(x, ...) AN_LOG(Debug, "CrashHandler: " x "", ##__VA_ARGS__)


extern "C"  __declspec(dllexport) AN::CrashHandlerInterface *ANCreateCrashHandler(const char *crashReportAppPath,
                                                const char *appName,
                                                const char *version,
                                                const char *crashReportFolder) {
    return new AN::CrashHandler(crashReportAppPath, appName, version, crashReportFolder);
}

extern "C" __declspec(dllexport) void ANDeleteCrashHandler(AN::CrashHandlerInterface *handler) {
    delete (AN::CrashHandler *) handler;
}

namespace AN {

PVOID Detour(PVOID func, PVOID jmp, bool attach) {
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

static CrashHandlerImpl *gGlobalHandler = NULL;

static bool __cdecl InternalProcessCrash(PEXCEPTION_POINTERS pExceptPtrs, CrashHandlerImpl *handler);

static LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExInfo) {
    DEBUGTRACE("intercepted exception");
    if (EXCEPTION_BREAKPOINT == pExInfo->ExceptionRecord->ExceptionCode) {
        // Breakpoint. Don't treat this as a normal crash.
        DEBUGTRACE("Breakpoint exception, skipping");
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (!gGlobalHandler)
        return EXCEPTION_CONTINUE_SEARCH;
    if (IsDebuggerPresent())// let the debugger catch this
        return EXCEPTION_CONTINUE_SEARCH;

    bool ok = InternalProcessCrash(pExInfo, gGlobalHandler);

    // If we're in a debugger, return EXCEPTION_CONTINUE_SEARCH to cause the debugger to stop;
    // or if GenerateErrorReport returned FALSE (i.e. drop into debugger).
    return (!ok) ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER;
}

static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI HookedSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter) {
    DEBUGTRACE("hooked SetUnhandledExceptionFilter called");
    return nullptr;
}


class CrashHandlerImpl {
public:

    TCrashCallback callback;

    LPTOP_LEVEL_EXCEPTION_FILTER oldExFilter;// previous exception filter

    std::string crashReportAppPath;
    std::string appName;
    std::string version;
    std::string crashReportFolder;

    CrashHandlerImpl(const char *aCrashReportAppPath,
                     const char *anAppName,
                     const char *aVersion, const char *aCrashReportFolder)
        : callback(),
          crashReportAppPath(aCrashReportAppPath),
          appName(anAppName),
          version(aVersion),
          crashReportFolder(aCrashReportFolder) {}

    bool isInstalled() const {
        return this == gGlobalHandler;
    }

    bool install() {
        if (NULL == gGlobalHandler) {
            // add ourselves to the exception callback chain
            DEBUGTRACE("installing handler");
            oldExFilter = ::SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
            ::SetErrorMode(SEM_FAILCRITICALERRORS);
            gGlobalHandler = this;

            /// we hooked this function to avoid anyone else calling it
            Detour(SetUnhandledExceptionFilter, HookedSetUnhandledExceptionFilter, true);

            return true;
        } else {
            return isInstalled();
        }
    }

    void uninstall() {
        if (isInstalled()) {

            Detour(SetUnhandledExceptionFilter, HookedSetUnhandledExceptionFilter, false);

            // reset exception callback to previous filter (which can be NULL)
            DEBUGTRACE("uninstalling handler");
            ::SetUnhandledExceptionFilter(oldExFilter);
            gGlobalHandler = NULL;
        }
    }
};


CrashHandler::CrashHandler(const char *crashRepAppPath,
                           const char *appName,
                           const char *appInfo,
                           const char *baseCrashRepFolder)
    : impl(new CrashHandlerImpl{ crashRepAppPath, appName, appInfo, baseCrashRepFolder }) {}

CrashHandler::~CrashHandler() {
    if (impl) {
        delete impl;
        impl = nullptr;
    }
}

bool CrashHandler::install() {
    return impl->install();
}

void CrashHandler::uninstall() {
    impl->uninstall();
}

int CrashHandler::processCrash(EXCEPTION_POINTERS *ep) {
    DEBUGTRACE("ProcessCrash: processing");

    if (IsDebuggerPresent())             // let the debugger catch this
        return EXCEPTION_CONTINUE_SEARCH;// ???

    bool ok = InternalProcessCrash(ep, impl);

    // Do a continue search if crash processor told us we should ignore the exception
    return (!ok) ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER;
}

bool CrashHandler::isInstalled() const {
    return impl->isInstalled();
}

void CrashHandler::setCrashCallback(TCrashCallback cb) {
    impl->callback = cb;
}

const char *CrashHandler::getCrashReportFolder() {
    return impl->crashReportFolder.c_str();
}


static bool alreadyProcessedCrash = false;

static BOOL CALLBACK miniDumpCallback(PVOID pParam, const PMINIDUMP_CALLBACK_INPUT pInput, PMINIDUMP_CALLBACK_OUTPUT pOutput) {
    if (pInput == 0 || pOutput == 0) {
        return FALSE;
    }
    switch (pInput->CallbackType) {
        case ModuleCallback:
        case IncludeModuleCallback:
        case IncludeThreadCallback:
        case ThreadCallback:
        case ThreadExCallback:
            return TRUE;
        default:
            break;
    }

    return FALSE;
}

static bool __cdecl InternalProcessCrash(PEXCEPTION_POINTERS pExceptPtrs, CrashHandlerImpl *handler) {
    //only ever spawn the crashreporter once per session, so we don't bring up 200 crashreporters.
    if (alreadyProcessedCrash) return false;
    alreadyProcessedCrash = true;

    static bool inRecursion = false;
    if (inRecursion)// Going recursive! That must mean this routine crashed!
        return false;
    inRecursion = true;

    DEBUGTRACE("processing crash");

    char TIME[64] = {0};
    char DAY[64] = {0};

    SYSTEMTIME sys;
    GetLocalTime(&sys);
    sprintf(TIME, "%02d%02d%02d", sys.wHour, sys.wMinute, sys.wSecond);

    time_t curTm;
    time(&curTm);
    struct tm pTm = *localtime(&curTm);
    sprintf(DAY, "%04d%02d%02d", pTm.tm_year + 1900, pTm.tm_mon + 1, pTm.tm_mday);

    char exeName[256] = {0};
    sprintf(exeName, "%s-%s-%s.dmp", handler->appName.c_str(), DAY, TIME);

    std::string dumpFilePath = handler->crashReportFolder;
    std::string fileName = exeName;
    std::string exeDumpFilePath = dumpFilePath + "/" + fileName;

    std::filesystem::create_directories(handler->crashReportFolder);


    if (handler->callback)
        handler->callback(handler->crashReportFolder.c_str());

    HANDLE hFile = NULL;
    DEBUGTRACE("creating dump file at %s", exeDumpFilePath.c_str());
    hFile = CreateFileW(Utf8ToWide(exeDumpFilePath).c_str(),
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_WRITE,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

    if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        MINIDUMP_CALLBACK_INFORMATION mci;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pExceptPtrs;
        mdei.ClientPointers = FALSE;
        mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)miniDumpCallback;
        mci.CallbackParam = NULL;
        if (!MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, NULL, &mci)) {
            DEBUGTRACE("MiniDumpWriteDump return false");
        }
        CloseHandle(hFile);


        /// create crash app process
        STARTUPINFOW si;
        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;

        PROCESS_INFORMATION pi;
        ZeroMemory( &pi, sizeof(pi) );

        TCHAR szModuleName[MAX_PATH * 4];
        ZeroMemory(szModuleName, sizeof(szModuleName));
        if (GetModuleFileName(0, szModuleName, std::size(szModuleName)-2) <= 0)
            lstrcpy(szModuleName, L"Unknown");

        wchar_t wideCmdLine[2048];
        std::filesystem::path dumpFileFullPath(exeDumpFilePath);
        dumpFileFullPath = std::filesystem::absolute(dumpFileFullPath);
        std::string cmdLine = std::format("{} {} {}", handler->crashReportAppPath,
                                          std::filesystem::path(szModuleName).filename().string(),
                                          dumpFileFullPath.string());

        MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), -1, wideCmdLine, 2048 );

        if (CreateProcessW(
                    NULL,       // name of executable module
                    wideCmdLine,// command line string
                    NULL,       // process attributes
                    NULL,       // thread attributes
                    FALSE,      // handle inheritance option
                    0,          // creation flags
                    NULL,       // new environment block
                    NULL,       // current directory name
                    &si,        // startup information
                    &pi))       // process information
        {
            // CrashReportApp was successfully started
            inRecursion = false;

            WaitForInputIdle(pi.hProcess, INFINITE);
            // Set focus to the main window of the new process
            // Get the main window handle of the new process
            HWND mainWindowHandle = nullptr;
            EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
                            DWORD processId;
                            GetWindowThreadProcessId(hWnd, &processId);
                            if (processId == lParam) {
                                *reinterpret_cast<HWND*>(lParam) = hWnd;
                                return FALSE; // Stop enumerating windows
                            }
                            return TRUE; // Continue enumerating windows
                        }, reinterpret_cast<LPARAM>(&mainWindowHandle));

            // Set focus to the main window of the new process
            if (mainWindowHandle != nullptr)
                SetForegroundWindow(mainWindowHandle);

            return true;
        } else {
            DEBUGTRACE("Fail to create crash reporter app process");
        }


        return true;
    } else {
        DEBUGTRACE("create dump file error");
    }

    return true;
}

}// namespace AN
