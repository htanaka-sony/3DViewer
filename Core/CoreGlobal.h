#ifndef CORE_GLOBAL_H
#define CORE_GLOBAL_H

#ifdef CORE_LIBRARY
    #define CORE_EXPORT __declspec(dllexport)
#else
    #define CORE_EXPORT __declspec(dllimport)
    #ifndef USE_QT
        #define USE_QT
    #endif
#endif

#define CORE_NAMESPACE_BEGIN namespace Core {
#define CORE_NAMESPACE_END }

#include "CoreDebug.h"

#endif    // CORE_GLOBAL_H
