//
// Created by aojoie on 7/1/2023.
//

#ifndef OJOIE_EXPORT_H
#define OJOIE_EXPORT_H

#include <ojoie/Configuration/platform.h>

#ifdef AN_STATIC_DEFINE
#  define AN_API
#  define AN_NO_EXPORT
#else
#  ifndef AN_API
#    ifdef AN_BUILD_OJOIE
/* We are building this library */
#      if AN_WIN
#        define AN_API __declspec(dllexport)
#      else
#        define AN_API __attribute__((visibility("default")))
#      endif
#    else
/* We are using this library */
#      if AN_WIN
#          define AN_API __declspec(dllimport)
#      else
#          define AN_API
#      endif
#    endif
#  endif
#endif

#ifndef AN_DEPRECATED
#  define AN_DEPRECATED __declspec(deprecated)
#endif

#ifndef AN_DEPRECATED_EXPORT
#  define AN_DEPRECATED_EXPORT AN_API AN_DEPRECATED
#endif

#ifndef AN_DEPRECATED_NO_EXPORT
#  define AN_DEPRECATED_NO_EXPORT AN_NO_EXPORT AN_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef AN_NO_DEPRECATED
#    define AN_NO_DEPRECATED
#  endif
#endif

#endif//OJOIE_EXPORT_H
