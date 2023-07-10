//
// Created by aojoie on 4/18/2023.
//

#ifndef OJOIE_THREADS_HPP
#define OJOIE_THREADS_HPP

#include <ojoie/Configuration/typedef.h>

namespace AN {

typedef UInt32 ThreadID;

AN_API ThreadID GetCurrentThreadID();

AN_API void SetCurrentThreadName(const char *name);

AN_API std::string GetCurrentThreadName();

}


#endif//OJOIE_THREADS_HPP
