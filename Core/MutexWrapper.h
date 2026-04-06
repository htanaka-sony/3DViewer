#ifndef MUTEXWRAPPER_H
#define MUTEXWRAPPER_H

#include "CoreGlobal.h"

#include <mutex>

#ifdef _WIN32
    #include <windows.h>
    #define USE_CRITICAL_SECTION
#endif

CORE_NAMESPACE_BEGIN

/// 排他制御クラス
class MutexWrapper {
private:
#ifdef USE_CRITICAL_SECTION
    CRITICAL_SECTION criticalSection;    /// Windowsのクリティカルセクション
#else
    std::mutex stdMutex;    // std::mutex
#endif

public:
    MutexWrapper()
    {
#ifdef USE_CRITICAL_SECTION
        InitializeCriticalSection(&criticalSection);
#endif
    }

    ~MutexWrapper()
    {
#ifdef USE_CRITICAL_SECTION
        DeleteCriticalSection(&criticalSection);
#endif
    }

    void lock()
    {
#ifdef USE_CRITICAL_SECTION
        EnterCriticalSection(&criticalSection);
#else
        stdMutex.lock();
#endif
    }

    void unlock()
    {
#ifdef USE_CRITICAL_SECTION
        LeaveCriticalSection(&criticalSection);
#else
        stdMutex.unlock();
#endif
    }
};

CORE_NAMESPACE_END

#endif    // MUTEXWRAPPER_H
