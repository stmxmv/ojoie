// -*- Aleudillonam -*-
//===--------------------------- Aleudillonam ---------------------------------===//
//
// Log.cpp
// lib/Aleudillonam
// Created by Molybdenum on 2/13/22.
//===----------------------------------------------------------------------===//

#include "Utility/Log.h"
#include "Utility/Assert.h"
#include "Threads/Threads.hpp"
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

inline const char* LogTypeToString (ANLogType type) {
    switch (type) {
        case ANLogType_Assert:    return "Assert";
        case ANLogType_Debug:     return "Debug";
        case ANLogType_Exception: return "Exception";
        case ANLogType_Error:     return "Error";
        case ANLogType_Log:       return "Log";
        case ANLogType_Warning:   return "Warning";
        case ANLogType_Info:      return "Info";
        default:                return "";
    }
}

namespace AN {


void Log::ANLogFileLogCallback(const char *log, size_t size, void *userdata) {
    Log *self = (Log *)userdata;
    fwrite(log, 1, size - 1, self->file);
    fflush(self->file);
}

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


Log::Log(FILE *file) noexcept : file(file), callback(ANLogFileLogCallback), userdata(this) {}
Log::Log() noexcept : Log(stderr) {}
Log::Log(const char *path) noexcept : Log(fopen(path, "w+")) {}

Log::~Log() {
    if (file != stderr && file != stdout && file != stdin) {
        fclose(file);
    }
}

void Log::setPath(const char *path) noexcept {
    file = fopen(path, "w+");
}

void Log::log(ANLogType type, FORMAT_STRING(const char *format), ...) {
    va_list list;
    va_start(list, format);
    logv(type, format, list);
    va_end(list);
}

void Log::logv(ANLogType type, const char *format, va_list args) {
    int pid;

#ifdef _WIN32
    pid = static_cast<int>(GetCurrentProcessId());
#else
    pid = static_cast<int>(getpid());
#endif//_WIN32

    /// get thread id
    ThreadID tid = GetCurrentThreadID();

    /// get thread name
    std::string threadName = GetCurrentThreadName();

    /// get process name
#ifdef _WIN32

    char appname[256] = { 0 };
    strcpy(appname, strrchr(_pgmptr, '\\') + 1);

#else
    static const char * appname = getprogname();
#endif//_WIN32


    va_list args2;
    va_copy(args2, args);
    int len = 1 + vsnprintf(nullptr, 0, format, args2);
    va_end(args2);
    std::unique_ptr<char []> buf = std::make_unique<char[]>(len);
    vsnprintf(buf.get(), len, format, args);


    {

        std::lock_guard<SpinLock> lock(mutex);

        std::timespec ts;
        ANAssert(std::timespec_get(&ts, TIME_UTC));
        char timeStr[24];
        std::strftime(timeStr, sizeof timeStr, "%Y-%m-%d %T", std::localtime(&ts.tv_sec));


        char nsec_str[16];
        snprintf(nsec_str, sizeof nsec_str, "%ld", ts.tv_nsec);
        nsec_str[3] = 0; // truncate by 3


        int output_len;


        static const char * output_format =

#ifdef _WIN32
                "%s.%s %s [%d|%s|%lu] [%s] %s\n";
#else
                "%s.%s %s [%d|%s|%llu] [%s] %s\n";
#endif//_WIN32

        static const char * n_thr_name_output_format =
#ifdef _WIN32
                "%s.%s %s [%d|%lu] [%s] %s\n";
#else
                "%s.%s %s [%d|%llu] [%s] %s\n"
#endif//_WIN32

        const char *logType = LogTypeToString(type);

        if (!threadName.empty()) {

            output_len = 1 + snprintf(nullptr, 0, output_format, timeStr, nsec_str, appname, pid, threadName.c_str(), tid, logType, buf.get());

        } else {

            output_len = 1 + snprintf(nullptr, 0, n_thr_name_output_format, timeStr, nsec_str, appname, pid, tid, logType, buf.get());
        }

        // msvc don't support vla anyway
        std::unique_ptr<char []> output_buf = std::make_unique<char []>(output_len);

        if (!threadName.empty()) {

            snprintf(output_buf.get(), output_len, output_format, timeStr, nsec_str, appname, pid, threadName.c_str(), tid, logType, buf.get());

        } else {

            snprintf(output_buf.get(), output_len, n_thr_name_output_format, timeStr, nsec_str, appname, pid, tid, logType, buf.get());
        }

        /// if log is same as last one, skip it
        if (logs.empty() || logs.back() != output_buf.get()) {
            this->__log(output_buf.get());
            callback(output_buf.get(), output_len, userdata);
        }
    }



}

const char *Log::getLastLog() const {
    if (logs.empty()) {
        static const char *noLogMsg("no logs available");
        return noLogMsg;
    }
    return logs.back().c_str();
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
    return AN::Log::GetSharedLog().getLastLog();
}


void ANLog(FORMAT_STRING(const char *fmt), ...) {
    va_list args;
    va_start(args, fmt);
    AN::Log::GetSharedLog().logv(ANLogType_Log, fmt, args);
    va_end(args);
}

void ANLogv(FORMAT_STRING(const char *fmt), va_list args) {
    AN::Log::GetSharedLog().logv(ANLogType_Log, fmt, args);
}

void AN_Log(ANLogType type, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    AN::Log::GetSharedLog().logv(type, fmt, args);
    va_end(args);
}

void AN_Logv(ANLogType type, const char *fmt, va_list args) {
    AN::Log::GetSharedLog().logv(type, fmt, args);
}

void ANLogSetFile(FILE * file) {
    AN::Log::GetSharedLog().setFile(file);
}

void ANLogSetPath(const char * path) {
    AN::Log::GetSharedLog().setPath(path);
}

void ANLogSetCallback(ANLogCallback callback, void *userdata) {
    AN::Log::GetSharedLog().setLogCallback(callback, userdata);
}

void ANLogResetCallback(void) {
    AN::Log::GetSharedLog().resetLogCallback();
}
