// -*- Aleudillonam -*-
//===--------------------------- Aleudillonam ---------------------------------===//
//
// Log.cpp
// lib/Aleudillonam
// Created by Molybdenum on 2/13/22.
//===----------------------------------------------------------------------===//

#include "Core/Log.h"
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <cstring>
#include <codecvt>
#include <Windows.h>
#include <malloc.h>
#include <processthreadsapi.h>
#else
#include <unistd.h>
#include <alloca.h>
#endif

namespace AN {

Log &Log::GetSharedLog() {
    static Log log;
    return log;
}

int Log::MaxMaintainLogNum  = 49;

void Log::__log(const char * msg) {
    if (logs.size() > MaxMaintainLogNum) {
        logs.erase(logs.begin(), logs.begin() + MaxMaintainLogNum / 2);
    }
    logs.emplace_back(msg);
}

void Log::setFile(FILE * aFile) noexcept {
    file = aFile;
}

const std::vector<std::string> &Log::getLogs() const {
    return logs;
}

FILE *Log::getFile() const {
    return file;
}

Log::Log() noexcept : file(stderr) {

}
Log::Log(const char *path) noexcept : file(fopen(path, "w+")) {

}

Log::~Log() {
    if (file != stderr && file != stdout && file != stdin) {
        fclose(file);
    }
}

void Log::setPath(const char *path) noexcept {
    file = fopen(path, "w+");
}

void Log::log(const char *format, ...) {
    va_list list;
    va_start(list, format);
    logv(format, list);
    va_end(list);
}

void Log::logv(const char *format, va_list args) {
    static int pid = 0;

    if (0 == pid) {
#ifdef _WIN32
        pid = static_cast<int>(_getpid());
#else
        pid = static_cast<int>(getpid());
#endif
    }

    /// get thread id
#ifdef _WIN32
    thread_local DWORD tid = GetCurrentThreadId();
#else
    thread_local __uint64_t tid = 0;
    if (!tid) {
        pthread_threadid_np(pthread_self(), &tid);
    }

#endif

    /// get thread name
#ifdef _WIN32
    thread_local char threadName[128] = { 0 };
    if (!threadName[0]) {
        wchar_t *description;
        GetThreadDescription(GetCurrentThread(), &description);
        if (description) {
            WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    description,
                    (int)wcslen(description),
                    threadName,
                    sizeof threadName, nullptr, nullptr
                    );
        }
    }

#else
    thread_local char threadName[128] = { 0 };
    if (!threadName[0]) {
        pthread_getname_np(pthread_self(), threadName, sizeof threadName);
    }

#endif


    /// get process name
#ifdef _WIN32

    static char appname[256] = { 0 };
    if (!appname[0] && GetModuleFileNameA(nullptr, appname, sizeof appname)) {
        strcpy(appname, strrchr(appname, '\\') + 1);
    }

#else
    static const char * appname = getprogname();
#endif


    va_list args2;
    va_copy(args2, args);
    int len = 1 + vsnprintf(nullptr, 0, format, args2);
    va_end(args2);
    char *buf = (char*)alloca(len);
    vsnprintf(buf, len, format, args);


    {

        std::lock_guard<SpinLock> lock(mutex);

        std::timespec ts;
        std::timespec_get(&ts, TIME_UTC);
        char timeStr[24];
        std::strftime(timeStr, sizeof timeStr, "%Y-%m-%d %T", std::localtime(&ts.tv_sec));


        char nsec_str[16];
        snprintf(nsec_str, sizeof nsec_str, "%ld", ts.tv_nsec);
        nsec_str[3] = 0; // truncate by 3


        int output_len;


        static const char * output_format =

#ifdef _WIN32
                "%s.%s %s [%d|%s|%lu] %s\n";
#else
                "%s.%s %s [%d|%s|%llu] %s\n";
#endif

        static const char * n_thr_name_output_format =
#ifdef _WIN32
                "%s.%s %s [%d|%lu] %s\n";
#else
                "%s.%s %s [%d|%llu] %s\n"
#endif


        if (*threadName) {

            output_len = 1 + snprintf(nullptr, 0, output_format, timeStr, nsec_str, appname, pid, threadName, tid, buf);

        } else {

            output_len = 1 + snprintf(nullptr, 0, n_thr_name_output_format, timeStr, nsec_str, appname, pid, tid, buf);
        }

        // msvc don't support vla anyway
        char *output_buf = (char*)alloca(output_len);

        if (*threadName) {

            snprintf(output_buf, output_len, output_format, timeStr, nsec_str, appname, pid, threadName, tid, buf);

        } else {

            snprintf(output_buf, output_len, n_thr_name_output_format, timeStr, nsec_str, appname, pid, tid, buf);
        }


        this->__log(output_buf);
        fwrite(output_buf, output_len - 1, 1, getFile());
    }



}

const std::string &Log::getLastLog() const {
    if (logs.empty()) {
        static const std::string noLogMsg("no logs available");
        return noLogMsg;
    }
    return logs.back();
}




}

void ANLog(int val) {
    ANLog("%d", val);
}
void ANLog(long val) {
    ANLog("%ld", val);
}
void ANLog(char val) {
    ANLog("%c", val);
}

void ANLog(unsigned int val) {
    ANLog("%u", val);
}

void ANLog(unsigned long val) {
    ANLog("%lu", val);
}

void ANLog(unsigned char val) {
    ANLog("%uc", val);
}

void ANLog(float val) {
    ANLog("%f", val);
}

void ANLog(double val) {
    ANLog("%lf", val);
}


const char * ANLogGetLast(void) {
    return AN::Log::GetSharedLog().getLastLog().c_str();
}




void ANLog(const char * format, ...) {
    va_list args;
    va_start(args, format);
    AN::Log::GetSharedLog().logv(format, args);
    va_end(args);
}


void ANLogSetFile(FILE * file) {
    AN::Log::GetSharedLog().setFile(file);
}

void ANLogSetPath(const char * path) {
    AN::Log::GetSharedLog().setPath(path);
}

void ANLogv(const char * format, va_list args) {
    AN::Log::GetSharedLog().logv(format, args);
}
