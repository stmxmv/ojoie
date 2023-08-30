// -*- Aleudillonam -*-
//===--------------------------- Aleudillonam -----------------------------===//
//
// Log.h
// include/Aleudillonam
// Created by Molybdenum on 2/13/22.
//===----------------------------------------------------------------------===//

#ifndef ALEUDILLONAM_LOG_H
#define ALEUDILLONAM_LOG_H



#ifdef __cplusplus

#include <ojoie/Threads/SpinLock.hpp>
#include <vector>
#include <string>
#include <cstdarg>
#include <cstdio>
#include <mutex>

extern "C" {

#else
#include <stdarg.h>
#include <stdio.h>

#endif

#if defined(__MINGW32__) && !defined(__clang__)
#define AN_FORMAT_FUNCTION(F,A) __attribute__((format(gnu_printf, F, A)))
#elif defined(__clang__) || defined(__GNUC__)
#define AN_FORMAT_FUNCTION(F,A) __attribute__((format(printf, F, A)))
#else
#define AN_FORMAT_FUNCTION(F,A)
#endif

#ifdef _MSC_VER
# include <sal.h>
#  define FORMAT_STRING(p) _Printf_format_string_ p
#else
# define FORMAT_STRING(p) p
#endif


#ifdef AN_DEBUG
#define ANDebugLog(...) ANLog(__VA_ARGS__)
#else
#define ANDebugLog(...)
#endif

enum ANLogType {
    /// LogType used for Errors.
    ANLogType_Error = 0,
    /// LogType used for Asserts. (These indicate an error inside Unity itself.)
    ANLogType_Assert = 1,
    /// LogType used for Warnings.
    ANLogType_Warning = 2,
    /// LogType used for regular log messages.
    ANLogType_Log = 3,
    /// LogType used for Exceptions.
    ANLogType_Exception = 4,
    /// LogType used for Debug.
    ANLogType_Debug = 5,
    ANLogType_Info = 6,
    ///
    ANLogType_NumLevels
};

typedef UInt32 ThreadID;

/// the size including the terminating zero character
typedef void (*ANLogCallback)(const char *log, size_t size, void *userdata);

AN_API void AN_Log(ANLogType type, FORMAT_STRING(const char * fmt), ...) AN_FORMAT_FUNCTION(2,3);

AN_API void AN_Logv(ANLogType type, FORMAT_STRING(const char * fmt), va_list args);

AN_API void ANLog(FORMAT_STRING(const char * fmt), ...) AN_FORMAT_FUNCTION(1,2);

AN_API void ANLogv(FORMAT_STRING(const char * fmt), va_list args);

AN_API void ANLogSetCallback(ANLogCallback callback, void *userdata);

AN_API void ANLogResetCallback(void);

AN_API void ANLogSetFile(FILE * file);

AN_API void ANLogSetPath(const char * path);

#define AN_LOG(type, format, ...) AN_Log(ANLogType_##type, format, __VA_ARGS__)

#ifdef __cplusplus
}

AN_API void ANLog(int val);
AN_API void ANLog(unsigned int val);
AN_API void ANLog(long val);
AN_API void ANLog(unsigned long val);
AN_API void ANLog(char val);
AN_API void ANLog(unsigned char val);
AN_API void ANLog(float val);
AN_API void ANLog(double val);



namespace AN {

struct LogInfo {
    std::string   message;
    ANLogType     type;
    std::timespec timeSpan;
    ThreadID      threadID;
};

class AN_API Log {

    std::vector<LogInfo> logs;
    FILE * file;
    AN::SpinLock mutex;

    ANLogCallback callback;
    void *userdata;

    void __log(const LogInfo &info);

    static void ANLogFileLogCallback(const char *log, size_t size, void *userdata);

public:
    Log(const Log &) = delete;
    Log &operator=(const Log &) = delete;


    Log() noexcept;
    explicit Log(FILE *file) noexcept;
    explicit Log(const char * path) noexcept;

    static int MaxMaintainLogNum;

    ~Log();

    static Log &GetSharedLog();

    void log(ANLogType type, FORMAT_STRING(const char * format), ...) AN_FORMAT_FUNCTION(3,4);

    void logv(ANLogType type, const char * format,va_list list);

    void setLogCallback(ANLogCallback aCallback, void *aUserdata) {
        callback = aCallback;
        userdata = aUserdata;
    }

    void resetLogCallback() {
        callback = ANLogFileLogCallback;
        userdata = this;
    }

    void setFile(FILE * file) noexcept;

    void setPath(const char * path) noexcept;

    const std::vector<LogInfo> &getLogs() const;

    FILE *getFile() const;

};


}

#endif





#endif//ALEUDILLONAM_LOG_H
