#ifndef RENDER_GLOBAL_H
#define RENDER_GLOBAL_H

#ifdef RENDER_LIBRARY
    #define RENDER_EXPORT __declspec(dllexport)
#else
    #define RENDER_EXPORT __declspec(dllimport)
    #ifndef USE_QT
        #define USE_QT
    #endif
#endif

// #define MEASURE_TIME 1

#define RENDER_NAMESPACE_BEGIN namespace Render {
#define RENDER_NAMESPACE_END }

#endif    // RENDER_GLOBAL_H
