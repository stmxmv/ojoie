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


void ANLog(FORMAT_STRING(const char * fmt), ...) AN_FORMAT_FUNCTION(1,2);

void ANLogv(FORMAT_STRING(const char * fmt), va_list args);

void ANLogSetFile(FILE * file);

void ANLogSetPath(const char * path);

const char * ANLogGetLast(void);


#ifdef __cplusplus
}

#include <ojoie/Core/SpinLock.hpp>

void ANLog(int val);
void ANLog(unsigned int val);
void ANLog(long val);
void ANLog(unsigned long val);
void ANLog(char val);
void ANLog(unsigned char val);
void ANLog(float val);
void ANLog(double val);



namespace AN {
class Log {

    std::vector<std::string> logs;
    FILE * file;
    SpinLock mutex;

public:
    Log(const Log &) = delete;
    Log &operator=(const Log &) = delete;


    Log() noexcept;
    explicit Log(const char * path) noexcept;

    static int MaxMaintainLogNum;

    ~Log();

    static Log &GetSharedLog();

    /*!
     * @brief should not call this, this is used with global ANLog function.
     */
    void __log(const char * msg);

    void log(FORMAT_STRING(const char * format), ...) AN_FORMAT_FUNCTION(2,3);

    void logv(const char * format,va_list list);

    [[nodiscard]] const std::string &getLastLog() const ;

    void setFile(FILE * file) noexcept;

    void setPath(const char * path) noexcept;

    [[nodiscard]] const std::vector<std::string> &getLogs() const;

    [[nodiscard]] FILE *getFile() const;

};


}

#endif





#endif//ALEUDILLONAM_LOG_H
